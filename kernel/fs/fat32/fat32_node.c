/*
 * fat32/fat32_node.c - fat32 node related operations
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
#include <string.h>
#include <thread.h>
#include <cluster.h>
#include <ppm.h>
#include <pmm.h>
#include <page.h>
#include <mapper.h>
#include <vfs.h>

#include <fat32.h>
#include <fat32-private.h>


KMEM_OBJATTR_INIT(vfat_kmem_inode_init)
{
	attr->type   = KMEM_VFAT_NODE;
	attr->name   = "KCM VFAT Node";
	attr->size   = sizeof(struct vfat_inode_s);
	attr->aligne = 0;
	attr->min    = CONFIG_VFAT_NODE_MIN;
	attr->max    = CONFIG_VFAT_NODE_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}


inline void vfat_getshortname(char *from, char *to) {
	char *p = to;
	char *q = from;
	uint_fast8_t d;

	for(d=0; (d < 8 && *q != ' '); d++)
		*p++ = *q++;

	if (*(q = &from[8]) != ' ')
	{
		*p++ = '.';
		d = 0;
		while (d++ < 3 && *q != ' ')
			*p++ = *q++;
	}

	*p = 0;
}

static inline void vfat_convert_name(char *str1, char *str2) {
	uint_fast8_t i;
	char *extention_ptr;

	extention_ptr = str2 + 8;
	memset(str2,' ',11);

	for(i=0; ((*str1) && (*str1 != '.') && (i<11)) ; i++)
		*str2++ = *str1++;

	if((i<=8) && (*str1 == '.')) {
		str1++;
		memcpy(extention_ptr,str1,3);
	}
}


VFS_INIT_NODE(vfat_init_node)
{
	struct vfat_inode_s *inode_info;
	kmem_req_t req;
	error_t err;

	if(inode->i_type != VFS_VFAT_TYPE)
		return VFS_EINVAL;

	req.type       = KMEM_MAPPER;
	req.size       = sizeof(*inode->i_mapper);
	req.flags      = AF_KERNEL;
	inode->i_mapper = kmem_alloc(&req);
  
	if(inode->i_mapper == NULL)
		return VFS_ENOMEM;

	err = mapper_init(inode->i_mapper, NULL, inode, NULL);

	if(err)
		return err;

	if(inode->i_pv != NULL)
		inode_info = (struct vfat_inode_s*)inode->i_pv;
	else 
	{
		req.type = KMEM_VFAT_NODE;
		req.size = sizeof(*inode_info);
    
		if((inode_info = kmem_alloc(&req)) == NULL)
		{
			req.type = KMEM_MAPPER;
			req.ptr  = inode->i_mapper;
			kmem_free(&req);
			return VFS_ENOMEM;
		}
     
		inode->i_pv = (void *) inode_info;
	}

	memset(inode_info, 0, sizeof(*inode_info));    
	return 0;
}


VFS_RELEASE_NODE(vfat_release_node)
{
	kmem_req_t req;
  
	if(inode->i_mapper != NULL)
	{
		mapper_destroy(inode->i_mapper, true);
		req.type = KMEM_MAPPER;
		req.ptr  = inode->i_mapper;
		kmem_free(&req);
	}

	if(inode->i_pv != NULL)
	{
		req.type = KMEM_VFAT_NODE;
		req.ptr  = inode->i_pv;
		kmem_free(&req);
		inode->i_pv = NULL;
	}
  
	return 0;
}

struct vfat_inode_create_s
{
	vfat_cluster_t first_cluster;
	//TODO: use vfs_inode_ref_s
	struct vfs_inode_s *parent; gc_t pgc; cid_t pcid;
	struct vfs_context_s *ctx;
	struct vfs_dirent_s *dirent;
	uint32_t fat_attr;
	uint32_t attr;
	uint32_t ino;
	size_t size;
	char new;
};

struct vfat_inode_create_resp_s
{
	struct vfs_inode_s *inode;
	gc_t igc;
};

RPC_DECLARE(vfat_inode_create, RPC_RET(RPC_RET_PTR(error_t, err), 
		RPC_RET_PTR(struct vfat_inode_create_resp_s, resp)),
		RPC_ARG(RPC_ARG_PTR(struct vfat_inode_create_s, ic)))
{
	struct vfat_context_s *ctx;
	struct vfs_inode_s *inode;
	struct vfat_inode_s *inode_info;
	struct page_s *tmp_page;
	kmem_req_t req;

	
	inode				= vfs_inode_new(ic->ctx);//allocate inode
	if(!inode)
	{
		*err = ENOMEM;
		return;
	}
	ctx				= &ic->ctx->ctx_vfat;
	inode_info			= inode->i_pv;
	assert(inode->i_mapper != NULL);
	inode->i_mapper->m_inode	= inode;
	inode->i_mapper->m_ops		= &vfat_file_mapper_op;
	inode->i_links			= 1;
	inode->i_size			= ic->size;
	inode->i_attr			= ic->attr;
	inode->i_number			= ic->ino;
	inode->i_dirent			= ic->dirent;
	inode->i_parent			= ic->parent;
	inode->i_pgc			= ic->pgc;
	inode->i_pcid			= ic->pcid;
		
	inode_info->flags		= ic->fat_attr;
	inode_info->first_cluster	= ic->first_cluster;

	vfat_dmsg(1, "Creating new inode with flags %x\n", inode_info->flags);

	resp->inode			= inode;
	resp->igc			= inode->i_gc;

	vfs_icache_add(inode);

	if(ic->attr & VFS_DIR) 
	{
		inode->i_mapper->m_ops = &vfat_inode_mapper_op;

		if(ic->new)
		{
			req.type  = KMEM_PAGE;
			req.size  = 0;
			req.flags = AF_USER | AF_ZERO;
			tmp_page  = kmem_alloc(&req);
	      
			if(tmp_page != NULL)
				*err = mapper_add_page(inode->i_mapper, tmp_page, 0);
			else 
				*err = ENOMEM;

			if(*err)
			{
				if(tmp_page != NULL)
				{
					req.ptr = tmp_page;
					kmem_free(&req);
				}
				return;
			}
			/* FIXME: we should also create dot & dotdot entries */
		}
 
	}

	*err = 0;
}

//parent must be refcounted!
//dirent must be set as INLOAD: no other thread can
//deleted or insert it's entry!
VFS_CREATE_NODE(vfat_create) 
{
	struct vfat_dirent_s *dirent_info;
	struct vfat_inode_s *parent_info;
	struct vfat_context_s *ctx;
	struct page_s *page;
	struct mapper_s *mapper;
	struct vfat_DirEntry_s *dir;
	struct page_s *tmp_page;
	kmem_req_t req;
	uint_t current_page;
	uint_t entries_nr;
	uint_t entry;
	vfat_cluster_t new_cluster;
	size_t current_offset;
	error_t err;

	ctx         = &parent->i_ctx->ctx_vfat;
	entries_nr  = PMM_PAGE_SIZE / sizeof(struct vfat_DirEntry_s);
	mapper      = parent->i_mapper;
	dir         = NULL;
	parent_info = NULL;
	new_cluster = 0;
	err         = 0;
	entry       = 0;
	page        = NULL;

	vfat_dmsg(1,"vfat_create started, dirent/node to be created %s, its parent %s\n",
		  dirent->d_name, parent->i_number);

	if(dirent->d_ctx->ctx_type != VFS_VFAT_TYPE)
		return VFS_EINVAL;

	// find a first cluster for the new file
	if(!(dirent->d_attr & (VFS_FIFO | VFS_DEV)))
	{
		if(vfat_alloc_fat_entry(ctx, &new_cluster))
			return VFS_IO_ERR;
    
		if(new_cluster == 0)
			return -VFS_ENOSPC;
	}

	dirent_info    = &dirent->d_vfat;
	parent_info    = (struct vfat_inode_s*) parent->i_pv;
	current_page   = 0;
	current_offset = 0;

	while(1) 
	{
		//TODO: lock the page in read_only to read it and then lock it back to lock to modify it or 
		//use CAS to allocate entrys ?
		if ((page = mapper_get_page(mapper, current_page, MAPPER_SYNC_OP)) == NULL)
		{
			err = VFS_IO_ERR;
			goto VFAT_CREATE_NODE_ERR;
		}

		dir = (struct vfat_DirEntry_s*) ppm_page2addr(page);
    
		page_lock(page);
    
		for(entry=0; entry < entries_nr; entry ++)
		{
			if((dir[entry].DIR_Name[0] == 0x00) || (dir[entry].DIR_Name[0] == 0xE5))
			{
				vfat_dmsg(1,"%s: found entry %d in page %d, name[0] %d\n",
					  __FUNCTION__,
					  entry, 
					  current_page, 
					  dir[entry].DIR_Name[0]);
				goto FREE_ENTRY_FOUND;
			}
			current_offset += sizeof (struct vfat_DirEntry_s);
		}
    
		page_unlock(page);
		current_page ++;
	}

FREE_ENTRY_FOUND:
	current_offset += sizeof (struct vfat_DirEntry_s);
	// we need to set the next entry to 0 if we got the last one
	if (dir[entry].DIR_Name[0] == 0x00) 
	{
		if(entry == entries_nr - 1) 
		{
			req.type  = KMEM_PAGE;
			req.size  = 0;
			req.flags = AF_USER | AF_ZERO;
			tmp_page  = kmem_alloc(&req);
      
			if(tmp_page != NULL)
				err = mapper_add_page(mapper, tmp_page, current_page + 1);
			else 
				err = ENOMEM;

			if(err)
			{
				page_unlock(page);

				if(tmp_page != NULL)
				{
					req.ptr = tmp_page;
					kmem_free(&req);
				}

				goto VFAT_CREATE_NODE_ERR;
			}
      		
			mapper->m_ops->set_page_dirty(tmp_page);

			if (current_offset == parent->i_size)
				parent->i_size += ctx->bytes_per_cluster;
		}
		else
			dir[entry+1].DIR_Name[0] = 0x00;
	}

	dir[entry].DIR_FstClusHI = new_cluster >> 16;
	dir[entry].DIR_FstClusLO = new_cluster & 0xFFFF;
	dir[entry].DIR_FileSize  = 0;
	dir[entry].DIR_Attr      = 0;

	if((dirent->d_attr & (VFS_SYS | VFS_FIFO | VFS_DEV)))
		dir[entry].DIR_Attr |= VFAT_ATTR_SYSTEM;

	if(dirent->d_attr & VFS_ARCHIVE)
		dir[entry].DIR_Attr |= VFAT_ATTR_ARCHIVE;

	if((dirent->d_attr & (VFS_RD_ONLY | VFS_FIFO | VFS_DEV)))
		dir[entry].DIR_Attr |= VFAT_ATTR_READ_ONLY;

	if(dirent->d_attr & VFS_DIR)
		dir[entry].DIR_Attr  |= VFAT_ATTR_DIRECTORY;

	vfat_convert_name(dirent->d_name,(char *)dir[entry].DIR_Name);  /* FIXME: name may be long */


#if VFAT_INSTRUMENT
	wr_count ++;
#endif

	dirent_info->entry_index    = current_page*entries_nr + entry;
	dirent_info->first_cluster  = new_cluster;
	page_unlock(page);

	error_t ret_err;
	{
		uint32_t ino = vfs_inum_new(parent->i_ctx);
		struct vfat_inode_create_s ic = {.first_cluster = new_cluster, 
						.ctx = parent->i_ctx, 
						.dirent = dirent, 
						.parent = parent, 
						.pgc = parent->i_gc, 
						.pcid = current_cid, 
						.fat_attr = dir[entry].DIR_Attr,
						.ino = ino,
						.size = ctx->bytes_per_cluster,
						.attr = dirent->d_attr};
		struct vfat_inode_create_resp_s resp;

		cid_t inode_cid = vfs_inode_get_cid(ino);
		err = RCPC(inode_cid, 	RPC_PRIO_FS, 
					vfat_inode_create,
					RPC_RECV(RPC_RECV_OBJ(ret_err),
						RPC_RECV_OBJ(resp)), 
					RPC_SEND(RPC_SEND_OBJ(ic)));

		dirent->d_inode.ptr = resp.inode;
		dirent->d_inode.cid = inode_cid;
		dirent->d_inode.gc = resp.igc;
		dirent->d_inum = ino;
	}

	if(!(err || ret_err))
		return 0;

	//find the page of dir, lock it and remove the entry!
	page_lock(page); //FIXME: is the page still valid ?
	dir->DIR_Name[0] = (dir[1].DIR_Name[0] == 0x00) ? 0x00 : 0xE5;
	page_unlock(page);

	if(!err) err = ret_err;

VFAT_CREATE_NODE_ERR:
	vfat_free_fat_entry(ctx, new_cluster);
	return err;
}


//Synchro: since the corresponding dirent is set INLOAD
//no one can delete the dirent
VFS_LOOKUP_NODE(vfat_lookup)
{
	struct vfat_context_s *ctx;
	struct vfat_inode_s *parent_info;
	struct vfat_dirent_s *dirent_info;
	struct vfat_entry_request_s rq;
	struct vfat_DirEntry_s dir;
	size_t isize;
	vfat_cluster_t first_cluster;
	uint_t entry_index;
	error_t err;

	ctx         = &parent->i_ctx->ctx_vfat;
	parent_info = parent->i_pv;
	dirent_info = &dirent->d_vfat;
	err         = 0;
	isize	    = 0;

	if(!(parent_info->flags & VFAT_ATTR_DIRECTORY))
		return VFS_ENOTDIR;

	rq.ctx         = ctx;
	rq.parent      = parent;
	rq.entry       = &dir;
	rq.entry_name  = dirent->d_name;
	rq.entry_index = &entry_index;

	if((err=vfat_locate_entry(&rq)))
		return err;

#if 0
	if(((node->i_attr & VFS_DIR) && 1) ^ ((dir.DIR_Attr & VFAT_ATTR_DIRECTORY) && 1))
		return VFS_ENOTDIR;
#else
	if((dirent->d_attr & VFS_DIR) && !(dir.DIR_Attr & VFAT_ATTR_DIRECTORY))
		return VFS_ENOTDIR;
#endif

	if(dir.DIR_Attr & VFAT_ATTR_DIRECTORY)
		dirent->d_attr |= VFS_DIR;
	else
		isize = dir.DIR_FileSize;

	if(dir.DIR_Attr & VFAT_ATTR_SYSTEM)    dirent->d_attr |= VFS_SYS;
	if(dir.DIR_Attr & VFAT_ATTR_ARCHIVE)   dirent->d_attr |= VFS_ARCHIVE;
	if(dir.DIR_Attr & VFAT_ATTR_READ_ONLY) dirent->d_attr |= VFS_RD_ONLY;
 
	first_cluster			= dir.DIR_FstClusHI << 16;
	first_cluster			|= (0x0000FFFF & dir.DIR_FstClusLO);

	if((!first_cluster)   && 
	   (dirent->d_attr & VFS_SYS)     && 
	   (dirent->d_attr & VFS_RD_ONLY) && 
	   (dirent->d_attr & VFS_DIR)) 
	{
		dirent->d_attr |= VFS_DEV;
		dirent->d_attr &= ~(VFS_SYS | VFS_RD_ONLY | VFS_DIR);
	} 
	else
	{
		if((!first_cluster) && 
		   (dirent->d_attr & VFS_SYS)   && 
		   (dirent->d_attr & VFS_RD_ONLY)) 
		{
			dirent->d_attr |= VFS_FIFO;
			dirent->d_attr &= ~(VFS_SYS | VFS_RD_ONLY);
		}
	}

	if((first_cluster) && (dirent->d_attr & VFS_DIR))
		isize = vfat_cluster_count(ctx, first_cluster) * ctx->bytes_per_cluster;

	dirent_info->entry_index	= entry_index;
	dirent_info->first_cluster	= first_cluster;

	error_t ret_err;
	{
		uint32_t ino = vfs_inum_new(parent->i_ctx);
		struct vfat_inode_create_s ic = {.first_cluster = first_cluster, 
						.ctx = parent->i_ctx, 
						.dirent = dirent, 
						.parent = parent, 
						.pgc = parent->i_gc, 
						.pcid = current_cid, 
						.fat_attr = dir.DIR_Attr,
						.ino = ino,
						.size = isize,
						.attr = dirent->d_attr};
		struct vfat_inode_create_resp_s resp;


		cid_t inode_cid = vfs_inode_get_cid(ino);
		err = RCPC(inode_cid, 	RPC_PRIO_FS, 
					vfat_inode_create,
					RPC_RECV(RPC_RECV_OBJ(ret_err),
						RPC_RECV_OBJ(resp)), 
					RPC_SEND(RPC_SEND_OBJ(ic)));

		dirent->d_inode.ptr = resp.inode;
		dirent->d_inode.cid = inode_cid;
		dirent->d_inode.gc = resp.igc;
		dirent->d_inum = ino;
	}

	//drop page ?
	if(err)	return err;
	if(ret_err) return ret_err;

	return VFS_FOUND;
}

VFS_STAT_NODE(vfat_stat_node)
{
	struct vfat_context_s *ctx;
	struct vfat_inode_s *inode_info;
	uint_t mode;

	inode_info = inode->i_pv;
	ctx       = &inode->i_ctx->ctx_vfat;

	mode                    = 0;
	inode->i_stat.st_dev     = (uint_t)ctx->dev;
	inode->i_stat.st_ino     = inode_info->first_cluster;
	inode->i_stat.st_nlink   = inode->i_links;
	inode->i_stat.st_uid     = 0;
	inode->i_stat.st_gid     = 0;
	inode->i_stat.st_rdev    = VFS_VFAT_TYPE;
	inode->i_stat.st_size    = inode->i_size;
	inode->i_stat.st_blksize = ctx->bytes_per_sector;
	inode->i_stat.st_blocks  = (uint32_t)inode->i_size / ctx->bytes_per_sector;
	inode->i_stat.st_atime   = 0;
	inode->i_stat.st_mtime   = 0;
	inode->i_stat.st_ctime   = 0;

	if(inode->i_attr & VFS_DIR)
	{
		VFS_SET(mode, VFS_IFDIR);
	}
	else if(inode->i_attr & VFS_FIFO)
	{
		VFS_SET(mode, VFS_IFIFO);
	}
	else if(inode->i_attr & VFS_PIPE)
	{
		VFS_SET(mode, VFS_IFIFO);
	}
	else if(inode->i_attr & VFS_DEV_CHR)
	{
		VFS_SET(mode, VFS_IFCHR);
	}
	else if(inode->i_attr & VFS_DEV_BLK)
	{
		VFS_SET(mode, VFS_IFBLK);
	}
	else
		VFS_SET(mode, VFS_IFREG);

	inode->i_stat.st_mode = mode;
	return 0;
}

RPC_DECLARE(__vfat_write_inode, RPC_RET(RPC_RET_PTR(error_t, err)),
		RPC_ARG(RPC_ARG_VAL(struct vfs_dirent_s*, dirent), 
		RPC_ARG_VAL(uint32_t, attr), 
		RPC_ARG_VAL(size_t, size)))
{
	struct page_s *page;
	struct mapper_s *mapper;
	struct vfs_inode_s *parent;
	struct vfat_dirent_s *dirent_info;
	struct vfat_DirEntry_s *dir;
	uint_t entry_index_page;

	parent		= dirent->d_parent;
	mapper          = parent->i_mapper;
	dirent_info	= &dirent->d_vfat;
	entry_index_page = dirent_info->entry_index * sizeof(struct vfat_DirEntry_s*) >> PMM_PAGE_SHIFT;
	dirent->d_attr	= attr; 

	*err = VFS_EINVAL;
	if(dirent->d_ctx->ctx_type != VFS_VFAT_TYPE)
		return;	

	*err = VFS_IO_ERR;
	//rd_lock is enough since we are supposed to be holding the dentry above with the INLOAD flag
	if ((page = mapper_get_page(mapper, entry_index_page, MAPPER_SYNC_OP)) == NULL)
		return;	

	//do the locking of mapper_get_page, like : MAPPER_PAGE_RD_LOCK
	page_lock(page);

	dir  = (struct vfat_DirEntry_s*) ppm_page2addr(page);
	dir += dirent_info->entry_index;

	if(attr & VFS_DIR) dir->DIR_Attr |= VFAT_ATTR_DIRECTORY;
	if(attr & VFS_SYS) dir->DIR_Attr |= VFAT_ATTR_SYSTEM;
	if(attr & VFS_ARCHIVE) dir->DIR_Attr |= VFAT_ATTR_ARCHIVE;
	if(attr & VFS_RD_ONLY) dir->DIR_Attr |= VFAT_ATTR_READ_ONLY;

	dir->DIR_FileSize = size;

	mapper->m_ops->set_page_dirty(page);

	page_unlock(page);

	*err = 0;
}

//update the dirent of this inode 
//FIXME: check syncro
VFS_WRITE_NODE(vfat_write_node)
{
	struct vfs_dirent_s *dirent;
	cid_t dirent_cid;
	error_t err;
	uint32_t attr;
	size_t isize;

	dirent          = inode->i_dirent;
	dirent_cid      = inode->i_pcid;
	isize		= inode->i_size;
	attr		= inode->i_attr;

	RCPC(dirent_cid, RPC_PRIO_FS_WRITE, __vfat_write_inode, RPC_RECV(RPC_RECV_OBJ(err)),
		RPC_SEND(RPC_SEND_OBJ(dirent), RPC_SEND_OBJ(isize), RPC_SEND_OBJ(attr)));

#if VFAT_INSTRUMENT
	wr_count ++;
#endif
	return err;
}

#if 1
/* Only check if dir is empty 	*
 * return 0 if it is empty
 * else return 1		*
 * The emptiness of files is	*
 * done in unlink		*
 * The deletion of the inode is *
 * no necesseray for the FAT fs */
VFS_DELETE_NODE(vfat_delete)
{
	struct vfat_DirEntry_s dir;
	struct page_s *page;
	struct mapper_s *mapper;
	uint8_t *buff;
	uint32_t found;
	uint32_t offset = 0; //64 bits?

	if(!(inode->i_attr & VFS_DIR) )
		return 0;

	mapper    = inode->i_mapper;
	found     = 0;

	/* TODO: dont call mapper every time, as page can be reused */
	while(!found) 
	{
		if ((page = mapper_get_page(mapper, 
					    offset >> PMM_PAGE_SHIFT, 
					    MAPPER_SYNC_OP)) == NULL)
			return VFS_IO_ERR;

		page_lock(page);

		buff  = ppm_page2addr(page);
		buff += offset % PMM_PAGE_SIZE;

		//is a memcpy necessary?
		memcpy(&dir, buff, sizeof(dir));

		if(dir.DIR_Name[0] == 0x00) {
			vfat_dmsg(3,"vfat_readdir: entries termination found (0x00)\n");
			page_unlock(page);
			break;
		}

		if(dir.DIR_Name[0] == 0xE5) {
			vfat_dmsg(3,"entry was freeed previously\n");
			goto VFS_READ_DIR_NEXT;
		}

		if(dir.DIR_Attr == 0x0F) {
			vfat_dmsg(3,"this entry is a long one\n");
			// FIXME should not delete dirs who contain long name ? 
			// knowink that we can't read them.
			//goto VFS_READ_DIR_NEXT;
			//found = 1;
			page_unlock(page);
			break;
		}

		if(dir.DIR_Name[0] == '.') // dot and dot_dot entries: since fat doesnot support hiden file!
			goto VFS_READ_DIR_NEXT;

		vfat_dmsg(1, "DIR NOT empty (%s), name: %s\n",__FUNCTION__, dir.DIR_Name);
		found = 1;

	VFS_READ_DIR_NEXT:
		page_unlock(page);
		if(found) break;
		offset += sizeof(struct vfat_DirEntry_s);
	}

	return found ? VFS_ENOTEMPTY : 0;
}

VFS_UNLINK_NODE(vfat_unlink_node)
{
	struct vfat_entry_request_s rq;
	struct page_s *page;
	struct mapper_s *mapper;
	struct vfat_context_s *ctx;
	struct vfat_dirent_s *dirent_info;
	struct vfat_DirEntry_s *dir;
	uint_t entry_index;
	uint_t entry_index_page;
	error_t err;
	uint_t val;

	err         = 0;
	ctx         = &dirent->d_ctx->ctx_vfat;
	dirent_info = &dirent->d_vfat;
	mapper      = parent->i_mapper;


	/* If this dirent has an inode delete it.		*
	*  For dirs, this function also check that it is empty.	*/
	if(dirent->d_inode.cid != CID_NULL)
		err=vfs_delete_inode(dirent->d_inum, dirent->d_ctx, dirent->d_inode.cid);
	
	if(err) goto UNLINK_IOERR;

	//Free FAT name directory entry
	if((dirent->d_attr & VFS_FIFO) && (dirent->d_op != &vfat_d_op)) //?
	{
		rq.entry       = NULL;
		rq.ctx         = ctx;
		rq.parent      = parent;
		rq.entry_name  = dirent->d_name;
		rq.entry_index = &entry_index;

		if((err=vfat_locate_entry(&rq)))
			return err;

	} else 
	{
		entry_index = dirent_info->entry_index;
	}

	entry_index_page = (entry_index*sizeof(struct vfat_DirEntry_s)) >> PMM_PAGE_SHIFT;
	page = mapper_get_page(mapper, entry_index_page, MAPPER_SYNC_OP);

	if(page == NULL)
	{ 
		val = entry_index_page;
		err = VFS_IO_ERR;
		goto UNLINK_IOERR;
	}

	dir = (struct vfat_DirEntry_s*) ppm_page2addr(page);
	dir += entry_index % (PMM_PAGE_SIZE / sizeof(struct vfat_DirEntry_s));

	uint_t entries_nr = PMM_PAGE_SIZE / sizeof(struct vfat_DirEntry_s);
	val = 0;

	if (entry_index == (entries_nr - 1))
	{
		// last entry in the page, looking for the next page
		struct page_s *temp_page;
		struct vfat_DirEntry_s *temp_dir;

		temp_page = mapper_get_page(mapper, entry_index_page+1, MAPPER_SYNC_OP);

		if(temp_page == NULL)
		{
			val = entry_index_page + 1;
			err = VFS_IO_ERR;
			goto UNLINK_IOERR;
		}

		page_lock(temp_page);
		page_lock(page);

		temp_dir = (struct vfat_DirEntry_s*) ppm_page2addr(temp_page);

		if (temp_dir->DIR_Name[0] == 0x00)
			dir->DIR_Name[0] = 0x00;
		else
			dir->DIR_Name[0] = 0xE5;

		mapper->m_ops->set_page_dirty(page);
		page_unlock(page);
		page_unlock(temp_page);
	} 
	else 
	{
		// we can look for the next entry
		page_lock(page);
		dir->DIR_Name[0] = (dir[1].DIR_Name[0] == 0x00) ? 0x00 : 0xE5;
		mapper->m_ops->set_page_dirty(page);
		page_unlock(page);
	}

	//Free content if not fifo
	if(!(dirent->d_attr & VFS_FIFO))
		err=vfat_free_fat_entry(ctx, dirent_info->first_cluster);

	assert(!err);

#if VFAT_INSTRUMENT
	wr_count ++;
#endif

UNLINK_IOERR:

	if (err == VFS_IO_ERR)
	{
		printk(WARNING,"%s: unable to load page #%d of inode %d while removing node %s\n",
		       __FUNCTION__,
		       val,
		       parent->i_number,
		       dirent->d_name);
	}
	return err;
}
#endif

const struct vfs_inode_op_s vfat_i_op =
{
	.init    = vfat_init_node,
	.create  = vfat_create,
	.lookup  = vfat_lookup,
	.write   = vfat_write_node,
	.release = vfat_release_node,
	.unlink  = vfat_unlink_node,
	.delete  = vfat_delete,
	.stat    = vfat_stat_node,
	.trunc	 = NULL
};

VFS_COMPARE_DIRENT(vfat_compare)
{
	return strcasecmp(first, second);//TODO
}

const struct vfs_dirent_op_s vfat_d_op =
{
	.compare    = vfat_compare
};
