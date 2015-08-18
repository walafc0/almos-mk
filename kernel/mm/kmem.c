/*
 * mm/kmem.c - kernel unified memory allocator
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

#include <types.h>
#include <errno.h>
#include <spinlock.h>
#include <kmem.h>
#include <heap_manager.h>
#include <ppm.h>
#include <page.h>
#include <cluster.h>
#include <thread.h>
#include <dma.h>
#include <blkio.h>
#include <task.h>
#include <devfs.h>
#include <fat32.h>
#include <sysfs.h>
#include <vfs.h>
#include <semaphore.h>
#include <cond_var.h>
#include <barrier.h>
#include <rwlock.h>
#include <mapper.h>
#include <radix.h>
#include <vm_region.h>
//#include <ext2.h>

typedef KMEM_OBJATTR_INIT(kmem_init_t);

static kmem_init_t * const kmem_init_tbl[KMEM_TYPES_NR] = {
	NULL, /* RESERVED */
	NULL, /* RESERVED */
	blkio_kmem_init,
	radix_node_kmem_init,
	dma_kmem_request_init, 
	mapper_kmem_init,
	task_kmem_init,
	task_fdinfo_kmem_init,
	devfs_kmem_context_init,
	devfs_kmem_file_init,
	vfat_kmem_context_init,
	vfat_kmem_inode_init,
	sysfs_kmem_node_init,
	sysfs_kmem_file_init,
	vfs_kmem_context_init,
	vfs_kmem_inode_init,
	vfs_kmem_dirent_init,
	vfs_kmem_file_remote_init,
	vm_region_kmem_init,
	sem_kmem_init,
	cv_kmem_init,
	barrier_kmem_init,
	rwlock_kmem_init,
	wqdb_kmem_init,
	keysrec_kmem_init//,
	//ext2_kmem_context_init,
	//ext2_kmem_node_init,
	//ext2_kmem_file_init
	};

static void* kmem_kcm_alloc(struct cluster_s *cluster, struct kmem_req_s *req);

void* kmem_alloc(struct kmem_req_s *req)
{
	register struct cluster_s *cluster;
	register struct kcm_s *kcm;
	register uint_t type;
	register uint_t size;
	register uint_t flags;
	register uint_t i;
	register void *ptr;

	type  = req->type;
	size  = req->size;
	flags = req->flags;
  
	kmem_dmsg("%s: cpu %d, pid %d, tid %x, type %d, size %d, flags %x [ STARTED ] [ %u ]\n",
		  __FUNCTION__, 
		  cpu_get_id(), 
		  current_task->pid,
		  current_thread,
		  type, 
		  size, 
		  flags,
		  cpu_time_stamp());

	assert(type < KMEM_TYPES_NR);
  
	if(flags & AF_REMOTE)
	{
		flags &= ~(AF_REMOTE);
		cluster = req->ptr; 
	}
	else
		cluster = current_cluster;

	switch(type)
	{
	case KMEM_PAGE:
		ptr = (void*) ppm_alloc_pages(&cluster->ppm, size, flags);
		if((flags & AF_ZERO) && (ptr != NULL)) page_zero(ptr);
    
		if(cluster->ppm.free_pages_nr < cluster->ppm.kprio_pages_min)
		{      
#if 0
			printk(INFO, "INFO: %s: cpu %d, cid %d, memory shortage found [%u]\n", 
			       __FUNCTION__,
			       cpu_get_id(),
			       cluster->id, 
			       cpu_time_stamp());
#endif
			for(i = 0; i < CLUSTER_TOTAL_KEYS_NR; i++)
			{
				kcm = cluster->keys_tbl[i];
	
				if(kcm != NULL) 
					kcm_release(kcm);
			}
		}
		break;

	case KMEM_GENERIC:
		ptr = heap_manager_malloc(&cluster->khm, size, flags);
		if((flags & AF_ZERO) && (ptr != NULL)) memset(ptr, 0, size);
		break;
    
	default:
		ptr = kmem_kcm_alloc(cluster, req);
	}

	kmem_dmsg("%s:    cpu %d, pid %d, tid %x, got ptr %x [ ENDED ] [ %u ]\n",
		  __FUNCTION__,
		  cpu_get_id(),
		  current_task->pid,
		  current_thread,
		  ptr,
		  cpu_time_stamp());
	    

	if(ptr != NULL) return ptr;

	printk(WARNING, "WARNING: %s: cpu %d, cid %d, pid %d, tid %d, failed to alloc type %d, size %d, flags %x\n", 
	       __FUNCTION__,
	       cpu_get_id(),
	       cluster->id,
	       current_task->pid,
	       current_thread->info.order,
	       type,
	       size,
	       flags);
 
	return NULL;
}


void kmem_free(struct kmem_req_s *req)
{
	assert(req->type < KMEM_TYPES_NR);
  
	switch(req->type)
	{
	case KMEM_PAGE:
		ppm_free_pages((struct page_s*) req->ptr);
		return;

	case KMEM_GENERIC:
		heap_manager_free(req->ptr);
		return;

	default:
		kcm_free(req->ptr);
		return;
	}
}

/* TODO: use req info to enlarge kcm allocatoin possiblities */
error_t kmem_get_kcm(struct cluster_s *cluster, kmem_req_t *req, struct kmem_objattr_s *attr)
{
	struct kcm_s *kcm;
	struct thread_s *this;
	error_t err;

#if CONFIG_SHOW_KMEM_INIT
	printk(DEBUG, "%s: cluster %d: object name: %s\n", 
	       __FUNCTION__, 
	       cluster->id, 
	       attr->name);
#endif
  
	if((attr->type != KMEM_TASK) && (attr->size > (PMM_PAGE_SIZE >> 2)))
	{
		this = current_thread;

		printk(ERROR, "%s: cpu %d: pid %d: tid %x: unexpected size %d for an object of type %s\n", 
		       __FUNCTION__,
		       cpu_get_id(),
		       this->task->pid,
		       this,
		       attr->size, 
		       attr->name);

		return ENOSYS;
	}

	assert(req->size == attr->size);

	if((kcm = kcm_alloc(&cluster->kcm, 0)) == NULL)
		return ENOMEM;

	err = kcm_init(kcm,
		       attr->name,
		       attr->size,
		       attr->aligne,
		       attr->min,
		       attr->max,
		       attr->ctor,
		       attr->dtor,
		       cluster->kcm.page_alloc, 
		       cluster->kcm.page_free);

	if(err)
	{
		printk(WARNING, "WARNING: %s: faild to create KCM for object name %s [err %d]\n", 
		       __FUNCTION__, 
		       attr->name, 
		       err);

		return err;
	}

	cluster->keys_tbl[attr->type] = kcm;
	cpu_wbflush();
	return 0;
}

void* kmem_kcm_alloc(struct cluster_s *cluster, kmem_req_t *req)
{
	void *ptr;
	uint_t type;
	error_t err;
	struct kmem_objattr_s attr;

	ptr  = NULL; 
	err  = 0;
	type = req->type;

	if(cluster->keys_tbl[type] == NULL)
	{ 
		spinlock_lock(&cluster->lock);
  
		if(cluster->keys_tbl[type] == NULL)
		{
			err = kmem_init_tbl[type](&attr);

			if(err == 0)
				err = kmem_get_kcm(cluster, req, &attr);
		}

		spinlock_unlock(&cluster->lock);
	}

	if(err == 0)
		ptr = kcm_alloc((struct kcm_s*)cluster->keys_tbl[type], 0);

	return ptr;
}
