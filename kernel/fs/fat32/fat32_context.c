/*
 * fat32/fat32_context.c - fat32 context related operations
 *
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS-kernel.
 *
 * ALMOS-kernel is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS-kernel is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS-kernel; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <kdmsg.h>
#include <device.h>
#include <driver.h>
#include <kmem.h>
#include <rwlock.h>
#include <string.h>
#include <page.h>
#include <cluster.h>
#include <thread.h>
#include <mapper.h>
#include <atomic.h>
#include <vfs.h>
 
#include <fat32.h>
#include <fat32-private.h>

static error_t vfat_context_init(struct vfat_context_s *ctx)
{
	size_t blk_sz;
	dev_params_t params;
	struct vfat_bpb_s *bpb;
	struct page_s *page;
	struct mapper_s *mapper;
  
	if((ctx->dev->op.dev.get_params(ctx->dev, &params)))
		return -1;

	blk_sz                = params.sector_size;
	ctx->bytes_per_sector = blk_sz;
	mapper                = ctx->mapper;
       
	page = mapper_get_page(mapper, 0, MAPPER_SYNC_OP);

	if(page == NULL) 
	{
		printk(ERROR, "ERROR: VFAT context_init: I/O error, could not read bpb from the device.\n");
		return -1;
	}

	bpb = (struct vfat_bpb_s *) ppm_page2addr(page);

	ctx->bytes_per_sector = bpb->BPB_BytsPerSec;

	if (blk_sz != ctx->bytes_per_sector)
		PANIC("VFAT drv error: bpb/device block size mismatch bpb block %d, devBlk %d\n",
		      ctx->bytes_per_sector, blk_sz);

	ctx->fat_begin_lba         = bpb->BPB_RsvdSecCnt;
	ctx->fat_blk_count         = bpb->BPB_FATSz32;
	ctx->cluster_begin_lba     = ctx->fat_begin_lba + (bpb->BPB_NumFATs * bpb->BPB_FATSz32);
	ctx->sectors_per_cluster   = bpb->BPB_SecPerClus;
	ctx->rootdir_first_cluster = bpb->BPB_RootClus;
	ctx->bytes_per_cluster     = ctx->bytes_per_sector * ctx->sectors_per_cluster;
	ctx->last_allocated_sector = ctx->fat_begin_lba;
	ctx->last_allocated_index  = 2;

	vfat_dmsg(1, 
		  "%s:\n\tbegin_lba %d\n\tblk_count %d\n\tcluster_begin_lba %d\n\t"
		  "sectors_per_cluster %d\n\trootdir_first_cluster  %d\n\tbytes_per_cluster %d\n\t"
		  "MediaType %x (@%x)\n",
		  __FUNCTION__,
		  ctx->fat_begin_lba, 
		  ctx->fat_blk_count,
		  ctx->cluster_begin_lba, 
		  ctx->sectors_per_cluster,
		  ctx->rootdir_first_cluster, 
		  ctx->bytes_per_cluster,
		  bpb->BPB_Media,
		  &bpb->BPB_Media);

	vfat_dmsg(1, "DEBUG: context_init: last allocated sector %d, last allocated index %d\n",
		  ctx->last_allocated_sector, ctx->last_allocated_index);

	
	return 0;
}

VFS_CREATE_CONTEXT(vfat_create_context)
{
	kmem_req_t req;
	struct vfat_context_s *vfat_ctx;
	struct mapper_s *mapper;
	error_t err;

	vfat_dmsg(1, "%s: started\n", __FUNCTION__);
  
	vfat_ctx = &context->ctx_vfat;

	vfat_ctx->dev = context->ctx_dev;//necessary ?

	rwlock_init(&vfat_ctx->lock);

	req.type = KMEM_MAPPER;
	req.size = sizeof(*mapper);
	mapper   = kmem_alloc(&req);

	if(mapper == NULL)
	{
		err = ENOMEM;
		goto fail_mapper;
	}

	err = mapper_init(mapper, &vfat_inode_mapper_op, NULL, vfat_ctx);

	if(err)
	{
		printk(ERROR, "ERROR: %s: failed to init mapper, err %d\n", __FUNCTION__, err);
		goto fail_mapper_init;
	}

	vfat_ctx->mapper = mapper;
	vfat_ctx->fat_cid = current_cid;

	if ((err = vfat_context_init(vfat_ctx)))
	{
		printk(ERROR, "ERROR: vfat_create_context: INITIALIZING VFAT CONTEXT err %d\n",err);
		goto fail_ctx_init;
	}

	context->ctx_type    = VFS_VFAT_TYPE;
	context->ctx_op      = (struct vfs_context_op_s *) &vfat_ctx_op;
	context->ctx_inode_op = (struct vfs_inode_op_s *) &vfat_i_op;
	context->ctx_dirent_op = (struct vfs_dirent_op_s *) &vfat_d_op;
	context->ctx_file_op = (struct vfs_file_op_s *) &vfat_f_op;
	return 0;

fail_ctx_init:
fail_mapper_init:
	mapper_destroy(mapper, false);
	req.type = KMEM_MAPPER;
	req.ptr  = mapper;
	kmem_free(&req);

fail_mapper:
	req.type = KMEM_VFAT_CTX;
	req.ptr  = vfat_ctx;
	kmem_free(&req);

	rwlock_destroy(&vfat_ctx->lock);
	return err;
}

VFS_DESTROY_CONTEXT(vfat_destroy_context)
{
	struct vfat_context_s *ctx;

	ctx = &context->ctx_vfat;
	mapper_destroy(ctx->mapper, true);
	rwlock_destroy(&ctx->lock);
	return 0;
}


VFS_READ_ROOT(vfat_read_root)
{
	struct vfat_context_s *ctx;
	struct vfat_inode_s * i_info;
	
	ctx = &context->ctx_vfat;

	root->i_type |= VFS_DIR;//?
	root->i_links          = 1;
	root->i_attr          |= VFS_DIR;
	root->i_size           = vfat_cluster_count(ctx, ctx->rootdir_first_cluster);//does a dir has a size?
	root->i_mapper->m_inode = root;
	root->i_mapper->m_ops  = &vfat_inode_mapper_op;
	root->i_number = vfs_inum_new(context);

	assert(root->i_pv);
	i_info                 = root->i_pv;
	i_info->flags          = VFAT_ATTR_DIRECTORY;
	i_info->first_cluster  = ctx->rootdir_first_cluster;
	//i_info->parent_cluster = 0;
	//i_info->entry_index    = 0;

	return 0;
}


VFS_WRITE_ROOT(vfat_write_root)
{
	return 0;
}


KMEM_OBJATTR_INIT(vfat_kmem_context_init)
{
	attr->type   = KMEM_VFAT_CTX;
	attr->name   = "KCM VFAT CTX";
	attr->size   = sizeof(struct vfat_context_s);
	attr->aligne = 0;
	attr->min    = CONFIG_VFAT_CTX_MIN;
	attr->max    = CONFIG_VFAT_CTX_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}

const struct vfs_context_op_s vfat_ctx_op =
{
	.create     = vfat_create_context,
	.destroy    = vfat_destroy_context,
	.read_root  = vfat_read_root,
	.write_root = vfat_write_root
};
