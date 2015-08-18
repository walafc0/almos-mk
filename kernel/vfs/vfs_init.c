/*
 * vfs/vfs_init.c - Virtual File System initialization
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

#include <kmem.h>
#include <task.h>
#include <string.h>
#include <device.h>
#include <page.h>
#include <metafs.h>
#include <thread.h>
#include <cluster.h>

#include <vfs.h>
#include <sysfs.h>
#include <devfs.h>
//#include <ext2.h>
#include <fat32.h>


#ifdef CONFIG_DRIVER_FS_PIPE
#include <pipe.h>
#endif

#define MAX_FS 8
struct vfs_context_s vfs_contexts[MAX_FS];

KMEM_OBJATTR_INIT(vfs_kmem_context_init)
{
	attr->type   = KMEM_VFS_CTX;
	attr->name   = "KCM VFS CTX";
	attr->size   = sizeof(struct vfs_context_s);
	attr->aligne = 0;
	attr->min    = CONFIG_VFS_CTX_MIN;
	attr->max    = CONFIG_VFS_CTX_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}

typedef struct fs_type_s
{
	uint_t type;
	bool_t isRoot;
	const char *name;
	const struct vfs_context_op_s *ops;
}fs_type_t;


static fs_type_t fs_tbl[VFS_TYPES_NR] = 
{ 
	//{.type = VFS_EXT2_TYPE , .isRoot = true , .name = "Ext2FS", .ops = &ext2_ctx_op},
	{.type = VFS_SYSFS_TYPE, .isRoot = false, .name = "SysFS" , .ops = &sysfs_ctx_op},
	{.type = VFS_DEVFS_TYPE, .isRoot = false, .name = "DevFS" , .ops = &devfs_ctx_op},
	{.type = VFS_VFAT_TYPE , .isRoot = true , .name = "VfatFS", .ops = &vfat_ctx_op},
	{.type = VFS_PIPE_TYPE , .isRoot = false, .name = "FifoFS", .ops = NULL}
};


//Initialise contexts ...
void vfs_init()
{
	uint_t i;
	error_t err;

	for(i=0; i<MAX_FS; i++)
	{
		atomic_init(&vfs_contexts[i].ctx_count, 0);
		vfs_contexts[i].ctx_flags = 0;
	}
	
	vfs_dmsg(1, "%s: Init dirty pages_list\n", __FUNCTION__);

	dirty_pages_init();

	vfs_dmsg(1, "%s: Init caches\n", __FUNCTION__);

	if((err=vfs_cache_init()))
		PANIC("Failed to INITIALISE Inode cache, err %d\n", err);
	
}

RPC_DECLARE(alloc_context, RPC_RET(RPC_RET_PTR(struct vfs_context_s*, ctx)), RPC_ARG())
{
	uint_t i;
	*ctx=NULL;
	for(i=0; i<MAX_FS; i++)
	{
		if(atomic_test_set(&vfs_contexts[i].ctx_count, 1))
		{
			*ctx = &vfs_contexts[i]; 
			return;
		}
	}
}

struct root_info
{
	struct vfs_dirent_s *i_dirent; 
	struct vfs_inode_s *i_parent; gc_t i_pgc; cid_t i_pcid;
	struct vfs_context_s *i_ctx;
	struct vfs_context_s *i_mount_ctx;
};

RPC_DECLARE(replicate_root, RPC_RET(RPC_RET_PTR(error_t, err)), 
			RPC_ARG(RPC_ARG_PTR(struct root_info, info)))
{
	struct vfs_context_s *ctx;
	struct vfs_inode_s *root;

	*err = 0;
	root = NULL;
	ctx = info->i_ctx;
	root = vfs_inode_new(ctx);

	if(!root) 
	{
		vfs_dmsg(1, "%s: could not replicate root (no mem)\n", __FUNCTION__);
		*err = ENOMEM;
		return;
	}

	root->i_mount_ctx = info->i_mount_ctx;
	root->i_dirent = info->i_dirent;
	root->i_parent = info->i_parent;
	root->i_pcid = info->i_pcid;
	root->i_pgc = info->i_pgc; 
	root->i_ctx = info->i_ctx; 

	if(ctx->ctx_op->repli_root)
		if((*err=ctx->ctx_op->repli_root(ctx, root)))
			return;
	
	vfs_icache_add(root);//TODO: check error ?

	vfs_dmsg(1, "%s: root has been read\n", __FUNCTION__);

	VFS_INODE_REF_SET(ctx->ctx_root, root, current_cid);
}


error_t update_remote_contexts(struct vfs_inode_s *root)
{
	uint_t i;
	uint_t lcid;
	error_t err;
	uint_t clstr_nr;
	struct vfs_context_s *ctx;
	struct root_info info;

	err = 0;
	ctx = root->i_ctx;
	lcid = current_cid;
	clstr_nr = current_cluster->clstr_wram_nr;

	info.i_mount_ctx = root->i_mount_ctx;
	info.i_dirent = root->i_dirent;
	info.i_parent = root->i_parent;
	info.i_pcid = root->i_pcid;
	info.i_pgc = root->i_pgc;
	info.i_ctx = root->i_ctx;

	for(i=0; i < clstr_nr; i++)
	{
		if(i == lcid) continue;
		remote_memcpy(ctx, i, ctx, lcid, sizeof(*ctx));
		if(VFS_IS(ctx->ctx_flags, VFS_FS_LOCAL))
		{
			RCPC(i, RPC_PRIO_FS, replicate_root,
				RPC_RECV(RPC_RECV_OBJ(err)), 
				RPC_SEND(RPC_SEND_OBJ(info)));
			if(err) break;
		}
			
	}
	return err;
}

void update_task0_root_cwd(struct task_s *task0)
{
	uint_t i;
	uint_t lcid;
	uint_t clstr_nr;

	lcid = current_cid;
	clstr_nr = current_cluster->clstr_wram_nr;

	task0->vfs_cwd = task0->vfs_root;
	for(i=0; i < clstr_nr; i++)
	{
		if(i == lcid) continue;
		remote_memcpy(&task0->vfs_root, i, &task0->vfs_root, lcid, 
						sizeof(struct vfs_file_s));
		remote_memcpy(&task0->vfs_cwd, i, &task0->vfs_root, lcid, 
						sizeof(struct vfs_file_s));
	}
}


error_t vfs_mount_prepare(struct device_s *dev,
		       const struct vfs_context_op_s *ctx_op,
		       struct vfs_context_s **vfs_ctx,
		       struct vfs_inode_s **vfs_inode)
{
	struct vfs_context_s *ctx = NULL;
	struct vfs_inode_s *root   = NULL;
	//kmem_req_t req;
	error_t err;

	vfs_dmsg(1, "%s: started\n", __FUNCTION__);
	
	//FIXME: replace 0 by  get_boot_gid()
	if((err = RPPC(0, RPC_PRIO_FS, alloc_context, 
			RPC_RECV(RPC_RECV_OBJ(ctx)), RPC_SEND())))
		return err;

	if(ctx == NULL) return -VFS_ENOMEM;//?

	ctx->ctx_dev = dev;
	//atomic_init(&ctx->ctx_count, 1); done in alloc_context
	atomic_init(&ctx->ctx_inum_count, 0);

	if((err=ctx_op->create(ctx)))
		return err;
	
	vfs_dmsg(1, "%s: root context has been allocated\n", __FUNCTION__);

	//TODO: place (allocate) root acording to it's inode number ?
	if(vfs_inode)
	{
		root = vfs_inode_new(ctx);
		if(!root) 
		{
			return -VFS_ENOMEM;//TODO: deallocate ctx
		}

		vfs_dmsg(1, "%s: root has been allocated\n", __FUNCTION__);

		if((err=ctx_op->read_root(ctx, root)))
			return err;
		
		vfs_icache_add(root);//TODO: check error ?

		vfs_dmsg(1, "%s: root has been read\n", __FUNCTION__);

		VFS_INODE_REF_SET(ctx->ctx_root, root, current_cid);

		*vfs_inode = root;			
	}
		
	if(vfs_ctx)
		*vfs_ctx = ctx;

	return 0;
}

	
//FIXME: check for dynamique mount
void vfs_mount_link(struct vfs_inode_s *mount_point,
			struct vfs_inode_s *root, 
			char* name)
{
	struct vfs_dirent_s *dirent;

	dirent = vfs_dirent_new(mount_point->i_ctx, name);

	dirent->d_inode.ptr = root;
	dirent->d_inode.gc = root->i_gc;
	dirent->d_inum = root->i_number;
	dirent->d_inode.cid = current_cid;
	dirent->d_attr = VFS_DIR;
	dirent->d_parent = mount_point;//a root has a parent ?
	dirent->d_mounted = root->i_ctx;

	root->i_dirent = dirent;
	root->i_pcid = current_cid;
	root->i_parent = mount_point;
	root->i_pgc = mount_point->i_gc;
	root->i_mount_ctx = mount_point->i_ctx;

	vfs_dirent_up(dirent);//pin
	//vfs_inode_lock(mount_point);
	metafs_register(&mount_point->i_meta, &dirent->d_meta);
	//vfs_inode_unlock(mount_point);
}


error_t vfs_mount_fs_root(struct device_s *device,
			 uint_t fs_type,
			 struct task_s *task0)
{
	error_t err;
	uint_t clstr_nr;
	struct vfs_inode_s *fs_root;
	struct vfs_inode_s *devfs_root;
	struct vfs_inode_s *sysfs_root;

	vfs_dmsg(1, "%s: started\n", __FUNCTION__);
  
	assert(device != NULL);
	assert(device->type == DEV_BLK);

	if(fs_type >= VFS_TYPES_NR)
	{
		printk(ERROR, "ERROR: %s: invalid fs_type value %d\n",
		       __FUNCTION__, 
		       fs_type);

		return EINVAL;
	}

	if(fs_tbl[fs_type].isRoot == false)
	{
		printk(ERROR, "ERROR: %s: fs_type (%s) is not can not be mounted as a root\n",
		       __FUNCTION__,
		       fs_tbl[fs_type].name);

		return EINVAL;
	}

	vfs_dmsg(1, "%s: going to mount root node\n", __FUNCTION__);
  
	if((err=vfs_mount_prepare(device, fs_tbl[fs_type].ops, NULL, &fs_root)))
		return err;

	if((err=vfs_mount_prepare(NULL, fs_tbl[VFS_DEVFS_TYPE].ops, NULL, &devfs_root)))
		return err;

	if((err=vfs_mount_prepare(NULL, fs_tbl[VFS_SYSFS_TYPE].ops, NULL, &sysfs_root)))
		return err;

#if 0
	if((err=vfs_mount_prepare(NULL, &pipe_ctx_op, &vfs_pipe_ctx, NULL)))
		return err;
#endif

	vfs_dmsg(1, "%s: going to link root node\n", __FUNCTION__);

	vfs_inode_up(fs_root);
	vfs_inode_up(devfs_root);
	vfs_inode_up(sysfs_root);


	/* Link devfs and sysfs roots to the main root */
	vfs_mount_link(fs_root, devfs_root, "DEV");
	vfs_mount_link(fs_root, sysfs_root, "SYS");

	err = update_remote_contexts(fs_root);
	assert(!err);//TODO
	err = update_remote_contexts(devfs_root);
	assert(!err);
	err = update_remote_contexts(sysfs_root);
	assert(!err);

	/* Update task0->vfs_{root, cwd} fields */
	/* Note: task0 must be statically allocated */
	struct vfs_inode_ref_s ref;
	VFS_INODE_REF_SET(ref, fs_root, current_cid);
	vfs_file_get(fs_root->i_ctx, 
				&task0->vfs_root,
				NULL,
				fs_root->i_mapper, 
				&ref, 
				NULL,
				current_cid);
	
	clstr_nr = current_cluster->clstr_wram_nr;
	atomic_add(&task0->vfs_root.f_remote->fr_count, (clstr_nr*2)-1);
	//-1 because file_get already hold one rc
	update_task0_root_cwd(task0);

	return err;
}
