/*
 * vfs/vfs_file.c - vfs file related operations
 *
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012,2013,2014,2015 UPMC Sorbonne Universites
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

#include <task.h>
#include <thread.h>
#include <cpu.h>
#include <string.h>
#include <kmem.h>
#include <vfs.h>
#include <page.h>
#include <ppm.h>
#include <pmm.h>
#include <vfs-private.h>
#include <spinlock.h>
#include <mapper.h>
#include <vm_region.h>

static void vfs_file_ctor(struct kcm_s *kcm, void *ptr)
{
        struct vfs_file_remote_s *file = ptr;

	rwlock_init(&file->fr_rwlock);
}

KMEM_OBJATTR_INIT(vfs_kmem_file_remote_init)
{
	attr->type   = KMEM_VFS_FILE_REMOTE;
	attr->name   = "KCM VFS File REMOTE";
	attr->size   = sizeof(struct vfs_file_remote_s);
	attr->aligne = 0;
	attr->min    = CONFIG_VFS_FILE_MIN;
	attr->max    = CONFIG_VFS_FILE_MAX;
	attr->ctor   = vfs_file_ctor;
	attr->dtor   = NULL;
	return 0;
}

void vfs_file_remote_put(struct vfs_file_remote_s* fremote)
{
	kmem_req_t req;
	req.ptr = fremote;
	req.type  = KMEM_VFS_FILE_REMOTE;
	req.size  = sizeof(struct vfs_file_remote_s);

	kmem_free(&req);
}

struct vfs_file_remote_s* 
vfs_file_remote_get(struct vfs_inode_s *inode)
{
	struct vfs_file_remote_s *fremote;
	kmem_req_t req;
	
	req.type  = KMEM_VFS_FILE_REMOTE;
	req.size  = sizeof(struct vfs_file_remote_s);
	req.flags = AF_USR;
	
	fremote = kmem_alloc(&req);
	if(!fremote) return NULL;

	atomic_init(&fremote->fr_count, 1);
	fremote->fr_offset  = 0;
	fremote->fr_version = 0;
	fremote->fr_pv      = NULL;
	fremote->fr_op      = inode->i_ctx->ctx_file_op;
	fremote->fr_inode   = inode;

	return fremote;
}

struct vfs_file_s* vfs_file_get(struct vfs_context_s *ctx,
				struct vfs_file_s *file,
				struct vfs_file_remote_s *fremote,
				struct mapper_s *mapper,
				struct vfs_inode_ref_s *inode, 
				union vfs_file_private_s *priv, 
					cid_t home_cid)
{
	if(mapper_replicate(&file->f_mapper, mapper, home_cid))
	{
		return NULL;
	}

	if(!fremote)
	{
		if((home_cid != current_cid) || 
		((fremote = vfs_file_remote_get(inode->ptr) ) == NULL))
			return NULL;	
	}
	
	file->f_version = 0;
	file->f_ctx     = ctx;
	file->f_op      = ctx->ctx_file_op;
	file->f_remote  = fremote;
	file->f_inode   = *inode;
	if(priv) 
		file->f_private = *priv;

	return file;
}

RPC_DECLARE(__vfs_file_up,
                RPC_RET( RPC_RET_PTR(sint_t, count) ),
                RPC_ARG( RPC_ARG_VAL(struct vfs_file_remote_s*, file) )
           )
{
	atomic_inc(&file->fr_count);
        return;
}

sint_t vfs_file_remote_down(struct vfs_file_remote_s *file)
{
	sint_t count;
	error_t err;

        count = atomic_dec(&file->fr_count);

	if(count > 1) return 0;

	vfs_dmsg(1, "%s: cpu %d, pid %u\n",
		 __FUNCTION__,
		 cpu_get_id(),
		 current_task->pid);//TODO: print file name

	if(file->fr_inode != NULL)
	{
		vfs_inode_down(file->fr_inode);
	}

	if((err=file->fr_op->close(file)))
		return err;//FIXME: ?

	file->fr_op->release(file);
  
	vfs_file_remote_put(file);

        return count;
}

RPC_DECLARE(__vfs_file_down,
                RPC_RET( RPC_RET_PTR(sint_t, count) ),
                RPC_ARG( RPC_ARG_VAL(struct vfs_file_remote_s*, file) )
           )
{
	*count = vfs_file_remote_down(file);
}

sint_t vfs_file_up(struct vfs_file_s *file)
{
        sint_t count;
	//TODO: add lazy flag
        RCPC(file->f_inode.cid, RPC_PRIO_FS, __vfs_file_up,
                        RPC_RECV( RPC_RECV_OBJ(count) ),
                        RPC_SEND( RPC_SEND_OBJ(file->f_remote) ) );
        return count;
}

sint_t vfs_file_down(struct vfs_file_s *file)
{
        sint_t count;
	//TODO: add lazy flag
        RCPC(file->f_inode.cid, RPC_PRIO_FS, __vfs_file_down,
                        RPC_RECV( RPC_RECV_OBJ(count) ),
                        RPC_SEND( RPC_SEND_OBJ(file->f_remote) ) );
	return count;
}


//TODO: the inode is remote, so is the mapper
//add the notion of replicated mappera and then
//create a replica of the remote mapper
VFS_MMAP_FILE(vfs_default_mmap_file)
{
	assert(file);

	region->vm_mapper = file->f_mapper;
	region->vm_file   = *file;
	//atomic_add(&file->f_count, 1); done in sys_mmap

	if(region->vm_flags & VM_REG_PRIVATE)
	{
		region->vm_pgprot &= ~(PMM_WRITE);
		region->vm_pgprot |= PMM_COW;
	}

#if CONFIG_USE_COA
	uint_t onln_clusters;

	onln_clusters = arch_onln_cluster_nr();

	if((region->vm_flags & VM_REG_SHARED) && (region->vm_flags & VM_REG_INST) && (onln_clusters > 1))
	{
		region->vm_pgprot &= ~(PMM_PRESENT);
		region->vm_pgprot |= PMM_MIGRATE;
	}
#endif
	region->vm_flags |= VM_REG_FILE;

#if 0
	if(region->vm_flags & VM_REG_SHARED)
	{
		rwlock_wrlock(&region->vm_mapper->m_reg_lock);
		list_add_last(&region->vm_mapper->m_reg_root, &region->vm_shared_list);
		rwlock_unlock(&region->vm_mapper->m_reg_lock);
		//(void)atomic_add(&region->vm_mapper->m_refcount, 1);
	}
#endif

	vfs_dmsg(1,
	       "Region [%x,%x] has been mapped on a file\n",
	       region->vm_start,
	       region->vm_limit);

	return 0;
}

VFS_MUNMAP_FILE(vfs_default_munmap_file)
{
#if 0
	if(region->vm_flags & VM_REG_SHARED)
	{
		(void)atomic_add(&region->vm_mapper->m_refcount, -1);
		rwlock_wrlock(&region->vm_mapper->m_reg_lock);
		list_unlink(&region->vm_shared_list);
		rwlock_unlock(&region->vm_mapper->m_reg_lock);
	}
#endif

	return 0;
}


VFS_READ_FILE(vfs_default_read) 
{  
	struct mapper_buff_s mp_buff;
	uint_t buff_offset;
	uint_t nb_ppn;
	size_t size;
 
	if(file->f_attr & VFS_FIFO)
		return -EINVAL;

	size = buffer->get_size(buffer);
	if(buffer->size == 0) return 0;

	nb_ppn = buffer->get_ppn_max(buffer);	

	if(nb_ppn > 64)
	{
		//TODO allocate on mem!
		printk(ERROR, "Error in file %s at func %s line %d\n", 
				__FILE__, __FUNCTION__, __LINE__);
		while(1);
	}

	ppn_t ppns[nb_ppn];

	buff_offset = buffer->get_ppn(buffer, ppns, nb_ppn);

	mp_buff.data_offset = 0;
	mp_buff.file = file->f_remote;
	mp_buff.buff_offset = buff_offset;
	mp_buff.buff_ppns = &ppns[0];
	mp_buff.max_ppns = nb_ppn;
	mp_buff.size = size;

	return mapper_read(&file->f_mapper, &mp_buff, 1, file->f_flags);
}


VFS_WRITE_FILE(vfs_default_write) 
{
	struct mapper_buff_s mp_buff;
	uint_t buff_offset;
	uint_t nb_ppn;
	size_t size;
 
	if(file->f_attr & VFS_FIFO)
		return -EINVAL;

	size = buffer->get_size(buffer);
	if(buffer->size == 0) return 0;

	nb_ppn = buffer->get_ppn_max(buffer);	

	if(nb_ppn > 64)
	{
		//TODO allocate on mem!
		printk(ERROR, "Error in file %s at func %s line %d\n", 
				__FILE__, __FUNCTION__, __LINE__);
		while(1);
	}

	ppn_t ppns[nb_ppn];

	buff_offset = buffer->get_ppn(buffer, ppns, nb_ppn);

	mp_buff.data_offset = 0;
	mp_buff.file = file->f_remote;
	mp_buff.buff_offset = buff_offset;
	mp_buff.buff_ppns = &ppns[0];
	mp_buff.max_ppns = nb_ppn;
	mp_buff.size = size;

	return mapper_write(&file->f_mapper, &mp_buff, 1, file->f_flags);
}
