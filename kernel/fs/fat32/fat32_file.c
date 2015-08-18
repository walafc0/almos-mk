/*
 * fat32/fat32_file.c - fat32 file related operations
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

#include <stdint.h>
#include <kmem.h>
#include <string.h>
#include <ppm.h>
#include <pmm.h>
#include <page.h>
#include <mapper.h>
#include <vfs.h>
#include <ppn.h>
#include <thread.h>
#include <sys-vfs.h>

#include <fat32.h>
#include <fat32-private.h>

#if 0
KMEM_OBJATTR_INIT(vfat_kmem_file_init)
{
	attr->type   = KMEM_VFAT_FILE;
	attr->name   = "KCM VFAT File";
	attr->size   = sizeof(struct vfat_file_s);
	attr->aligne = 0;
	attr->min    = CONFIG_VFAT_FILE_MIN;
	attr->max    = CONFIG_VFAT_FILE_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}
#endif

VFS_OPEN_FILE(vfat_open) 
{
#if 0
	struct vfat_context_s *ctx;
	struct vfat_inode_s *node_info;
	//struct vfat_file_s *file_info;
	kmem_req_t req;
  
	ctx       = node->n_ctx->ctx_pv;
	node_info = node->n_pv;
	file_info = file->f_pv;

	if(file_info == NULL)
	{
		req.type  = KMEM_VFAT_FILE;
		req.size  = sizeof(*file_info);
		req.flags = AF_KERNEL;

		if((file_info = kmem_alloc(&req)) == NULL)
			return VFS_ENOMEM;
	}

	file_info->ctx                = ctx;
	file_info->node_cluster       = node_info->node_cluster;
	file_info->pg_current_cluster = node_info->node_cluster;
	file_info->pg_current_rank    = 0;
	file->f_pv                    = file_info;
#endif
	return 0;
}

VFS_LSEEK_FILE(vfat_lseek)
{
#if 0
	struct vfat_context_s *ctx;
	struct vfat_file_s *file_info;
	vfat_cluster_t cluster_rank;
	vfat_cluster_t next_cluster;
	vfat_cluster_t effective_rank;
	uint_t isExtanded;
	error_t err;

	file_info = file->f_pv;
	ctx       = file_info->ctx;
  
	if(file->f_offset == 0)
	{
		file_info->pg_current_cluster = file_info->node_cluster;
		file_info->pg_current_rank    = 0;
		return 0;
	}

	next_cluster   = file_info->pg_current_cluster;
	cluster_rank   = (vfat_offset_t)file->f_offset / ctx->bytes_per_cluster;
	effective_rank = cluster_rank;

	if(cluster_rank != file_info->pg_current_rank)
	{
		if(cluster_rank > file_info->pg_current_rank)
			effective_rank -= file_info->pg_current_rank;
		else
			next_cluster = file_info->node_cluster;
  
		if(effective_rank != 0)
		{
			if((err = vfat_cluster_lookup(ctx, 
						      next_cluster, 
						      effective_rank, 
						      &next_cluster, 
						      &isExtanded)))
				return err;
		}
	}
 
	file_info->pg_current_cluster = next_cluster;
	file_info->pg_current_rank    = cluster_rank;
#endif
	//file->f_pv = NULL;
	return 0;
}

VFS_CLOSE_FILE(vfat_close) 
{
	return 0;
}

VFS_RELEASE_FILE(vfat_release) 
{
	assert(file->fr_pv == NULL);
	return 0;
}


#if 0
RPC_DECLARE(__fat_get_ppn_next, RPC_RET(RPC_RET_PTR(ppn_t, ppn), RPC_RET_PTR(size_t, offset)),
		RPC_ARG(RPC_ARG_VAL(struct vfs_file_remote_s*, file))
		)
{
	rwlock_wrlock(&file->fr_rwlock);
	*ppn = mapper_get_ppn(file->fr_inode->i_mapper, 
				file->fr_offset >> PMM_PAGE_SHIFT, 
				MAPPER_SYNC_OP);
	*offset = file->fr_offset;
	file->fr_offset += sizeof(struct vfat_DirEntry_s);
	rwlock_unlock(&file->fr_rwlock);
}

ppn_t fat_get_ppn_next(struct vfs_file_s *file, uint32_t *offset)
{
	ppn_t ppn;
	struct mapper_s *mapper;

	mapper    = &file->f_mapper;

	RCPC(mapper->m_home_cid, RPC_PRIO_FS, __fat_get_ppn_next, 
				RPC_RECV(RPC_RECV_OBJ(ppn), RPC_RECV_OBJ(*offset)), 
				RPC_SEND(RPC_SEND_OBJ(file->f_remote)));
	return ppn;
}


//TODO: do the search in remote and then copy the result to local buff
VFS_READ_DIR(vfat_readdir) 
{
	vfat_cluster_t node_cluster;
	struct vfat_DirEntry_s dir;
	struct vfs_usp_dirent_s vdir;
	uint32_t offset;
	uint32_t found;
	uint8_t *buff;
	ppn_t ppn;
	cid_t cid;

	found     = 0;

	/* TODO: dont call mapper every time, as a ppn can be reused */
	while(!found) 
	{
		//FIXME: synchro : either use a lock or force the writer to
		//write the first byte of the name as the last thing he do
		//when adding an entry
		if ((ppn = fat_get_ppn_next(file, &offset)) == 0)
			return VFS_IO_ERR;

		cid   = ppn_ppn2cid(ppn);
		buff  = (void*)ppn_ppn2vma(ppn);
		buff += offset % PMM_PAGE_SIZE;

		remote_memcpy(&dir, current_cid, buff, cid, sizeof(dir));

		if(dir.DIR_Name[0] == 0x00) {
			vfat_dmsg(3,"vfat_readdir: entries termination found (0x00)\n");
			ppn_refcount_down(ppn);
			goto VFS_READ_DIR_EODIR;
		}

		if(dir.DIR_Name[0] == 0xE5) {
			vfat_dmsg(3,"entry was freeed previously\n");
			vfat_getshortname((char*)dir.DIR_Name, (char*)vdir.u_name);
			vfat_dmsg(3,"it was %s\n",vdir.u_name);
			goto VFS_READ_DIR_NEXT;
		}

		if(dir.DIR_Attr == 0x0F) {
			vfat_dmsg(3,"this entry is a long one\n");
			vfat_getshortname((char*)dir.DIR_Name, (char*)vdir.u_name);
			vfat_dmsg(3,"trying to read its name %s\n",vdir.u_name);
			goto VFS_READ_DIR_NEXT;
		}

		if(dir.DIR_Name[0] == '.')
			goto VFS_READ_DIR_NEXT;

		found = 1;
		vfat_getshortname((char *)dir.DIR_Name, (char*)vdir.u_name);

		//vdir.u_size = dir.DIR_FileSize;
		vdir.u_attr = 0;

		if(dir.DIR_Attr & VFAT_ATTR_DIRECTORY)
			vdir.u_attr = VFS_DIR;

		if(dir.DIR_Attr & VFAT_ATTR_SYSTEM)
			vdir.u_attr |= VFS_SYS;

		if(dir.DIR_Attr & VFAT_ATTR_ARCHIVE)
			vdir.u_attr |= VFS_ARCHIVE;

		if(dir.DIR_Attr & VFAT_ATTR_READ_ONLY)
			vdir.u_attr |= VFS_RD_ONLY;

		node_cluster  = dir.DIR_FstClusHI << 16;
		node_cluster |= (0x0000FFFF & dir.DIR_FstClusLO);

		if((!node_cluster) && (vdir.u_attr & VFS_SYS)
		   && (vdir.u_attr & VFS_RD_ONLY) && (vdir.u_attr & VFS_DIR)) {
			vdir.u_attr |= VFS_DEV;
			vdir.u_attr &= ~(VFS_SYS | VFS_RD_ONLY | VFS_DIR);
		}
		else
			if((!node_cluster) && (vdir.u_attr & VFS_SYS) && (vdir.u_attr & VFS_RD_ONLY)) {
				vdir.u_attr |= VFS_FIFO;
				vdir.u_attr &= ~(VFS_SYS | VFS_RD_ONLY);
			}

	VFS_READ_DIR_NEXT:
		ppn_refcount_down(ppn);
	}

VFS_READ_DIR_EODIR:
	if(found) dirent->copy_to_buff(dirent, &vdir);
	return (found) ? 0 : VFS_EODIR;
}
#else

VFS_READ_DIR(vfat_readdir) 
{
	struct vfat_DirEntry_s dir;
	struct page_s *page;
	struct mapper_s *mapper;
	vfat_cluster_t node_cluster;
	uint8_t *buff;
	uint32_t found;

	mapper    = file->fr_inode->i_mapper;
	found     = 0;

	/* TODO: dont call mapper every time, as page can be reused */
	while(!found) 
	{
		//FIXME: synchro : either use a lock or force the writer to
		//write the first byte of the name as the last thing he do
		//when adding an entry
		//also lock file offset ?
		if ((page = mapper_get_page(mapper, 
					    file->fr_offset >> PMM_PAGE_SHIFT, 
					    MAPPER_SYNC_OP)) == NULL)
			return VFS_IO_ERR;

		buff  = ppm_page2addr(page);
		buff += file->fr_offset % PMM_PAGE_SIZE;

		memcpy(&dir, buff, sizeof(dir));

		if(dir.DIR_Name[0] == 0x00) {
			vfat_dmsg(3,"vfat_readdir: entries termination found (0x00)\n");
			goto VFS_READ_DIR_EODIR;
		}

		if(dir.DIR_Name[0] == 0xE5) {
			vfat_dmsg(3,"entry was freeed previously\n");
			vfat_getshortname((char*)dir.DIR_Name, (char*)dirent->u_name);
			vfat_dmsg(3,"it was %s\n",dirent->u_name);
			goto VFS_READ_DIR_NEXT;
		}

		if(dir.DIR_Attr == 0x0F) {
			vfat_dmsg(3,"this entry is a long one\n");
			vfat_getshortname((char*)dir.DIR_Name, (char*)dirent->u_name);
			vfat_dmsg(3,"trying to read its name %s\n",dirent->u_name);
			goto VFS_READ_DIR_NEXT;
		}

		if(dir.DIR_Name[0] == '.')
			goto VFS_READ_DIR_NEXT;

		found = 1;
		vfat_getshortname((char *)dir.DIR_Name, (char*)dirent->u_name);

		//dirent->d_size = dir.DIR_FileSize;
		dirent->u_attr = 0;

		if(dir.DIR_Attr & VFAT_ATTR_DIRECTORY)
			dirent->u_attr = VFS_DIR;

		if(dir.DIR_Attr & VFAT_ATTR_SYSTEM)
			dirent->u_attr |= VFS_SYS;

		if(dir.DIR_Attr & VFAT_ATTR_ARCHIVE)
			dirent->u_attr |= VFS_ARCHIVE;

		if(dir.DIR_Attr & VFAT_ATTR_READ_ONLY)
			dirent->u_attr |= VFS_RD_ONLY;

		node_cluster  = dir.DIR_FstClusHI << 16;
		node_cluster |= (0x0000FFFF & dir.DIR_FstClusLO);

		if((!node_cluster) && (dirent->u_attr & VFS_SYS)
		   && (dirent->u_attr & VFS_RD_ONLY) && (dirent->u_attr & VFS_DIR)) {
			dirent->u_attr |= VFS_DEV;
			dirent->u_attr &= ~(VFS_SYS | VFS_RD_ONLY | VFS_DIR);
		}
		else
			if((!node_cluster) && (dirent->u_attr & VFS_SYS) && (dirent->u_attr & VFS_RD_ONLY)) {
				dirent->u_attr |= VFS_FIFO;
				dirent->u_attr &= ~(VFS_SYS | VFS_RD_ONLY);
			}

	VFS_READ_DIR_NEXT:
		file->fr_offset += sizeof(struct vfat_DirEntry_s);
	}

VFS_READ_DIR_EODIR:
	return (found) ? 0 : VFS_EODIR;
}
#endif

const struct vfs_file_op_s vfat_f_op =
{
	.open    = vfat_open,
	.read    = vfs_default_read,
	.write   = vfs_default_write,
	.lseek   = vfat_lseek,
	.readdir = vfat_readdir,
	.close   = vfat_close,
	.release = vfat_release,
	.mmap    = vfs_default_mmap_file,
	.munmap  = vfs_default_munmap_file
};
