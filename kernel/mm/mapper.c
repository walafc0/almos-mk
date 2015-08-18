/*
 * mm/mapper.c - mapping object used to map memory, file or device in
 * process virtual address space.
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

#include <mapper.h>
#include <radix.h>
#include <thread.h>
#include <cpu.h>
#include <task.h>
#include <kmem.h>
#include <kcm.h>
#include <page.h>
#include <blkio.h>
#include <cluster.h>
#include <vfs.h>
#include <ppn.h>
#include <vm_region.h>

static void mapper_ctor(struct kcm_s *kcm, void *ptr) 
{
	struct mapper_s *mapper;

	mapper = (struct mapper_s *)ptr;
	mapper->m_home = mapper;
	mapper->m_home_cid = current_cid;
}

KMEM_OBJATTR_INIT(mapper_kmem_init)
{
	attr->type   = KMEM_MAPPER;
	attr->name   = "KCM Mapper";
	attr->size   = sizeof(struct mapper_s);
	attr->aligne = 0;
	attr->min    = CONFIG_MAPPER_MIN;
	attr->max    = CONFIG_MAPPER_MAX;
	attr->ctor   = mapper_ctor;
	attr->dtor   = NULL;
	return 0;
}

error_t mapper_init(struct mapper_s *mapper, 
		    const struct mapper_op_s *ops, 
		    struct vfs_inode_s *inode,
		    void *data)
{
	
	kmem_req_t req;
	req.type  = KMEM_GENERIC;//do a type a part
	req.flags = AF_KERNEL;
	req.size  = sizeof(struct mapper_cnt_s);

	mapper->m_cnt = kmem_alloc(&req);

	if(!mapper->m_cnt) return ENOMEM;

	atomic_init(&mapper->m_refcount, 1);
	mapper->m_ops  = ops;
	mapper->m_inode = inode;
	mapper->m_data = data;

	radix_tree_init(&mapper->m_radix);
	spinlock_init(&mapper->m_cache_lock, "Mapper cache");
	mcs_lock_init(&mapper->m_lock, "Mapper Object");
	rwlock_init(&mapper->m_reg_lock);
	list_root_init(&mapper->m_reg_root);
	return 0;
}

//TODO: All open file should use the same mapper per cluster
error_t mapper_replicate(struct mapper_s* mapper,
			struct mapper_s *mapper_home, 
			cid_t mapper_home_cid)
{
	mapper->m_cnt = NULL;
	mapper->m_home = mapper_home;
	mapper->m_home_cid = mapper_home_cid;
	return 0;
}

static inline void __mapper_remove_page(struct mapper_s *mapper, struct page_s *page);

void mapper_destroy(struct mapper_s *mapper, bool_t doSync)
{
	kmem_req_t req;
	struct page_s* pages[10];
	uint_t count, j;

	req.type  = KMEM_PAGE;
	req.flags = AF_USER;

	if(doSync == true)
	{
		do   // writing && freeing all dirty pages
		{
			count = radix_tree_gang_lookup_tag(&mapper->m_radix,
							   (void**)pages,
							   0,
							   10,
							   TAG_PG_DIRTY);

			for (j=0; (j < count) && (pages[j] != NULL); ++j) 
			{
				page_lock(pages[j]);
				mapper->m_ops->sync_page(pages[j]);
				page_unlock(pages[j]);

				__mapper_remove_page(mapper,pages[j]);
				req.ptr = pages[j];
				kmem_free(&req);
			}
		}while(count != 0);
	}

	do   // freeing all pages
	{
		count = radix_tree_gang_lookup(&mapper->m_radix, (void**)pages, 0, 10);
 
		for (j=0; (j < count) && (pages[j] != NULL); ++j) 
		{
			if(PAGE_IS(pages[j], PG_DIRTY))
			{
				page_lock(pages[j]);
				mapper->m_ops->sync_page(pages[j]);
				page_unlock(pages[j]);

				page_lock(pages[j]);
				page_clear_dirty(pages[j]);
				page_unlock(pages[j]);
			}

			__mapper_remove_page(mapper,pages[j]);
			req.ptr = pages[j];
			kmem_free(&req);
		}
	}while(count != 0);
}

/* FIXME: check dummy pages inserted by mapper_get */
struct page_s* mapper_find_page(struct mapper_s* mapper, uint_t index) 
{
	struct page_s *page;
	uint_t irq_state;

	mcs_lock(&mapper->m_lock, &irq_state);
	page = radix_tree_lookup(&(mapper->m_radix), index);
	mcs_unlock(&mapper->m_lock, irq_state);

	return page;
}

/* FIXME: check dummy pages inserted by mapper_get */
uint_t mapper_find_pages(struct mapper_s* mapper, 
			 uint_t start, 
			 uint_t nr_pages, 
			 struct page_s** pages)
{
	uint_t pages_nr;
	uint_t irq_state;

	mcs_lock(&mapper->m_lock, &irq_state);

	pages_nr = radix_tree_gang_lookup(&(mapper->m_radix),
					  (void**)pages,
					  start,
					  nr_pages);

	mcs_unlock(&mapper->m_lock, irq_state);
	return pages_nr;
}

uint_t mapper_set_auto_migrate(struct mapper_s* mapper)
{
	uint_t count;
	uint_t pages_nr;
	uint_t irq_state;
	uint_t index, i;
	struct page_s *pgtbl[10];

	index    = 0;
	pages_nr = 0;

	mcs_lock(&mapper->m_lock, &irq_state);

	do
	{
		count  = radix_tree_gang_lookup(&mapper->m_radix, (void**)pgtbl, index, 10);
		index += count;

		for(i = 0; ((i < count) && (pgtbl[i] != NULL)); i++)
		{
			PAGE_SET(pgtbl[i], PG_MIGRATE);
			pages_nr ++;
		}

	}while(count > 0);

	mcs_unlock(&mapper->m_lock, irq_state);
	return pages_nr;
}


/* FIXME: check dummy pages inserted by mapper_get */
uint_t mapper_find_pages_contig(struct mapper_s* mapper, 
				uint_t start, 
				uint_t nr_pages, 
				struct page_s** pages)
{
	uint_t pages_nr;
	uint_t irq_state;
	uint_t i;

	mcs_lock(&mapper->m_lock, &irq_state);
	pages_nr = radix_tree_gang_lookup(&(mapper->m_radix),
					  (void**)pages,
					  start,
					  nr_pages);
	mcs_unlock(&mapper->m_lock, irq_state);

	for(i = 0; i < pages_nr; i++) 
	{
		if(pages[i]->index != (nr_pages+i))
			break;
	}

	pages[i] = NULL;
	return i;
}

/* FIXME: check dummy pages inserted by mapper_get */
uint_t mapper_find_pages_by_tag(struct mapper_s* mapper, 
				uint_t start, 
				uint_t tag, 
				uint_t nr_pages, 
				struct page_s** pages)
{
	uint_t pages_nr;
	uint_t irq_state;

	mcs_lock(&mapper->m_lock,&irq_state);

	pages_nr = radix_tree_gang_lookup_tag(&(mapper->m_radix),
					      (void**)pages,
					      start,
					      nr_pages,
					      tag);

	mcs_unlock(&mapper->m_lock, irq_state);
  
	return pages_nr;
}

error_t mapper_add_page(struct mapper_s *mapper, struct page_s* page, uint_t index)
{
	uint_t irq_state;
	error_t err;

	mcs_lock(&mapper->m_lock, &irq_state);
  
	err = radix_tree_insert(&(mapper->m_radix), index, page);
  
	if (err != 0) 
		goto ADD_PAGE_ERROR;

	page->mapper = mapper;
	page->index  = index;

ADD_PAGE_ERROR:
	mcs_unlock(&mapper->m_lock, irq_state);
	return err;
}

static inline void __mapper_remove_page(struct mapper_s *mapper, struct page_s *page)
{
	radix_tree_delete(&mapper->m_radix, page->index);
  
	PAGE_CLEAR(page, PG_DIRTY);

	if(PAGE_IS(page, PG_BUFFER))
		blkio_destroy(page);

	page->mapper = NULL;
}

void mapper_remove_page(struct page_s *page)
{
	struct mapper_s *mapper;
	kmem_req_t req;
	uint_t irq_state;

	mapper    = page->mapper;
	req.type  = KMEM_PAGE;
	req.ptr   = page;

	mcs_lock(&mapper->m_lock, &irq_state);
	__mapper_remove_page(mapper, page);
	mcs_unlock(&mapper->m_lock, irq_state);
	kmem_free(&req);
}

MAPPER_RELEASE_PAGE(mapper_default_release_page)
{
#if 0
	struct mapper_s *mapper;
	uint_t irq_state;

	mapper = page->mapper;

	mcs_lock(&mapper->m_lock, &irq_state);
	__mapper_remove_page(mapper, page);
	mcs_unlock(&mapper->m_lock, irq_state);
#endif
	return 0;
}

struct page_s* mapper_get_page(struct mapper_s*	mapper, uint_t index, uint_t flags)
{
	kmem_req_t req;
	struct page_s dummy;
	struct page_s *page;
	struct page_s *new;
	struct vm_region_s *reg;
	radix_item_info_t info;
	uint_t irq_state;
	bool_t found;
	error_t err;
	error_t err2;

	req.type  = KMEM_PAGE;
	req.size  = 0;
	req.flags = AF_USER;

	while (1) 
	{
		mcs_lock(&mapper->m_lock, &irq_state);
		found = radix_item_info_lookup(&mapper->m_radix, index, &info);

		if ((found == false) || (info.item == NULL))   // page not in mapper, creating it 
		{
			page_init(&dummy, CLUSTER_NR);
			PAGE_CLEAR(&dummy, PG_INIT);
			PAGE_SET(&dummy, PG_INLOAD);
			page_refcount_up(&dummy);
			dummy.mapper = mapper;
			dummy.index  = index;
			err          = 0;

			if(found == true)
			{
				err = radix_item_info_apply(&mapper->m_radix, 
							    &info, 
							    RADIX_INFO_INSERT, 
							    &dummy);
			}
			else
			{
				err = radix_tree_insert(&mapper->m_radix, 
							index, 
							&dummy);
			}

			if(err) __mapper_remove_page(mapper, &dummy);

			mcs_unlock(&mapper->m_lock, irq_state);

			if(err) goto fail_radix;

			page = kmem_alloc(&req);
      
			if(page != NULL)
			{
				PAGE_SET(page, PG_INLOAD);
				page->mapper = mapper;
				page->index  = index;

				err = mapper->m_ops->readpage(page, flags);
			}

			if((page == NULL) || (err != 0)) 
			{ 
				mcs_lock(&mapper->m_lock, &irq_state);
				__mapper_remove_page(mapper, &dummy);
				wakeup_all(&dummy.wait_queue);
				mcs_unlock(&mapper->m_lock, irq_state);
	
				if(page != NULL)
				{
					if(PAGE_IS(page, PG_BUFFER))
						blkio_destroy(page);
	  
					page->mapper = NULL;
					req.ptr = page;
					kmem_free(&req);
				}

				goto fail_alloc_load;
			}
      
			mcs_lock(&mapper->m_lock, &irq_state);
			found = radix_item_info_lookup(&mapper->m_radix, index, &info);
			assert((found == true) && (info.item == &dummy));
			err = radix_item_info_apply(&mapper->m_radix, &info, RADIX_INFO_SET, page);
			assert(err == 0);
			PAGE_CLEAR(page, PG_INLOAD);
			wakeup_all(&dummy.wait_queue);
			mcs_unlock(&mapper->m_lock,irq_state);
			return page;
		}

		page = (struct page_s*)info.item;

		if(PAGE_IS(page, PG_INLOAD))
		{
			wait_on(&page->wait_queue, WAIT_LAST);
			mcs_unlock(&mapper->m_lock, irq_state);
			sched_sleep(current_thread);
			continue;
		}

		if(PAGE_IS(page, PG_MIGRATE))
		{
			assert(MAPPER_IS_MIGRATABLE(mapper));

			PAGE_CLEAR(page, PG_MIGRATE);

			if((mapper->m_inode == NULL) && (current_cluster->id != page->cid))
			{
				page_init(&dummy, CLUSTER_NR);
				PAGE_CLEAR(&dummy, PG_INIT);
				PAGE_SET(&dummy, PG_INLOAD);
				page_refcount_up(&dummy);
				dummy.mapper = mapper;
				dummy.index  = index;

				err = radix_item_info_apply(&mapper->m_radix, &info, RADIX_INFO_SET, &dummy);

				if(err)
					continue;
			}
			else
			{
				mcs_unlock(&mapper->m_lock, irq_state);
				return page;
			}

			mcs_unlock(&mapper->m_lock, irq_state);

			new = NULL;
			err = 0;
			reg = NULL;

			rwlock_rdlock(&mapper->m_reg_lock);

			if(!(list_empty(&mapper->m_reg_root)))
			{
				reg = list_first(&mapper->m_reg_root, struct vm_region_s, vm_shared_list);

				page_lock(page);

				//err = vmm_broadcast_inval(reg, page, &new);
				err = vmm_migrate_shared_page_seq(reg, page, &new);

				page_unlock(page);
			}

			rwlock_unlock(&mapper->m_reg_lock);

			err2 = 0;

			mcs_lock(&mapper->m_lock, &irq_state);

			if(new == NULL)
				(void) radix_item_info_apply(&mapper->m_radix, &info, RADIX_INFO_SET, page);

			if((err == 0) && (new != NULL))
				err2 = radix_item_info_apply(&mapper->m_radix, &info, RADIX_INFO_SET, new);

			wakeup_all(&dummy.wait_queue);

			mcs_unlock(&mapper->m_lock, irq_state);

			if(new == NULL)
				return page;

			if((err != 0) || (err2 != 0))
			{
				/* TODO: differs this kmem_free */
				req.ptr = new;
				kmem_free(&req);
				return page;
			}

			req.ptr = page;
			kmem_free(&req);
			return new;
		}

		mcs_unlock(&mapper->m_lock, irq_state);
		return page;
	}

fail_radix:
	printk(ERROR, "ERROR: %s: cpu %d, pid %d, tid %d, failed to insert/modify node, err %d [%u]\n",
	       __FUNCTION__,
	       cpu_get_id(),
	       current_task->pid,
	       current_thread->info.order,
	       err);

	current_thread->info.errno = (err == ENOMEM) ? err : EIO;
	return NULL;

fail_alloc_load:
	printk(ERROR, "ERROR: %s: cpu %d, pid %d, tid %d, failed to alloc/load page, index %d, err %d [%u]\n",
	       __FUNCTION__,
	       cpu_get_id(),
	       current_task->pid,
	       current_thread->info.order,
	       index,
	       err,
	       cpu_time_stamp());

	current_thread->info.errno = (err == 0) ? ENOMEM : EIO;
	return NULL;
}

RPC_DECLARE( __mapper_get_ppn, 
		RPC_RET(RPC_RET_PTR(ppn_t, ppn)), 
		RPC_ARG(RPC_ARG_VAL(struct mapper_s*, mapper),
		RPC_ARG_VAL(uint_t, index),
		RPC_ARG_VAL(uint_t, flags)))
{
	struct page_s* page;
	page = mapper_get_page(mapper, index, flags);
	page_refcount_up(page);
	*ppn = ppm_page2ppn(page);
}

//similar to mapper_get_page but also hold the refcount!
ppn_t mapper_get_ppn(struct mapper_s* mapper, uint_t index, uint_t flags)
{
	ppn_t ret;

	RCPC(mapper->m_home_cid, 
		RPC_PRIO_MAPPER,
		__mapper_get_ppn,
		RPC_RECV(RPC_RECV_OBJ(ret)), 
		RPC_SEND(RPC_SEND_OBJ(mapper->m_home),
			RPC_SEND_OBJ(index),
			RPC_SEND_OBJ(flags))
		);

	return ret;
}

error_t __mapper_request_(struct mapper_s* mapper, struct mapper_buff_s *buff, uint_t flags, char read)
{
	struct vfs_inode_s *inode;
	struct vfs_file_remote_s *file;
	struct page_s *page;
	uint8_t *psrc;
	uint8_t *preq;
	size_t isize;//inode size
	size_t bsize;//buffer toatal size
	size_t csize;//slice to copy size
	size_t rsize;//slice request size
	size_t ssize;//slice source size
	uint_t current_offset;
	uint_t buff_offset;
	cid_t req_cid;
 
	bsize		= buff->size;
	file		= buff->file;
	if(bsize == 0) return 0;


	if(file)
	{
		rwlock_wrlock(&file->fr_rwlock);
		current_offset	= file->fr_offset;
	}else
		current_offset	= buff->data_offset;

	buff_offset	= buff->buff_offset;
	ssize		= 0;
	rsize		= 0;
	preq		= NULL;
	psrc		= NULL;
	req_cid		= 0;//CID_HOLE

	assert(mapper->m_inode);
	inode		= mapper->m_inode;


	if(read) isize	= inode->i_size;//synchro ?
	else isize = current_offset + bsize;

	while((bsize > 0) && (current_offset < isize)) 
	{
		assert(!(ssize && rsize));//they cannot be both full

		if(!ssize)
		{
			if ((page = mapper_get_page(mapper,
						    current_offset >> PMM_PAGE_SHIFT, 
						    MAPPER_SYNC_OP)) == NULL)
				return -VFS_IO_ERR;

			psrc  = (uint8_t*) ppm_page2addr(page);
			psrc += current_offset % PMM_PAGE_SIZE;
			ssize = ((isize - current_offset) > PMM_PAGE_SIZE) ? 
				PMM_PAGE_SIZE : (isize - current_offset);
		}

		if(!rsize)
		{
			preq		= (void*)ppn_ppn2vma(buff->buff_ppns[buff_offset >> PMM_PAGE_SHIFT]);
			preq		+= buff_offset % PMM_PAGE_SIZE;
			req_cid		= ppn_ppn2cid(buff->buff_ppns[buff_offset >> PMM_PAGE_SHIFT]);
			rsize		= MIN(PMM_PAGE_SIZE - (buff_offset%PMM_PAGE_SIZE), bsize);
		}
    
		csize = MIN(rsize, ssize);
		
		//copy to preq
		//buffer->scpy_to_buff(buffer, psrc, csize);
		if(read)
			remote_memcpy(preq, req_cid, psrc, current_cid, csize);
		else
			remote_memcpy(psrc, current_cid, preq, req_cid, csize);

		rsize -= csize;
		ssize -= csize;
		bsize -= csize;
		current_offset	+= csize;
		buff_offset	+= csize;
		
		//if(thread_sched_isActivated(current_thread))
		//	sched_yield(current_thread);
	}

	//TODO: update atime ...
	if(current_offset > inode->i_size)
	{
		inode->i_size = current_offset;//synchro ?
		//TODO if flags & SYNC or ...
		cpu_wbflush();
		VFS_SET(inode->i_state, VFS_DIRTY);//synch ?
		cpu_wbflush();
	}
	
	if(file)
	{
		file->fr_offset = current_offset;
		rwlock_unlock(&file->fr_rwlock);
	}	

	return buff->size - bsize;
}
#define MAPPER_OP_FIRST 0x1
#define MAPPER_OP_LAST	0x2
#define MAPPER_OP_READ	0x4

//FIXME: tacking lock over multiple buffer!
RPC_DECLARE(__mapper_request, RPC_RET(RPC_RET_PTR(size_t, size)), 
		RPC_ARG(RPC_ARG_VAL(struct mapper_s*, mapper), 
			RPC_ARG_PTR(struct mapper_buff_s, mp_buffs), 
			RPC_ARG_PTR(ppn_t, ppns), 
			RPC_ARG_VAL(uint_t, flags),
			RPC_ARG_VAL(uint_t, op_flag)))
{
	mp_buffs->buff_ppns = ppns;

	if(op_flag & MAPPER_OP_FIRST)
		spinlock_lock(&mapper->m_cache_lock);

	*size = __mapper_request_(mapper, mp_buffs, flags, op_flag & MAPPER_OP_READ);
	
	if(op_flag & MAPPER_OP_LAST)
		spinlock_unlock(&mapper->m_cache_lock);
}

//FIXME improve (for multiple buff)...
error_t mapper_request(struct mapper_s* mapper, struct mapper_buff_s *mp_buffs, size_t nb_buff, char read, uint_t flags)
{
	size_t size;
	uint_t op_flag;
	uint_t i;

	op_flag = MAPPER_OP_FIRST;
	if(read) op_flag |= MAPPER_OP_READ;
	for(i=0; i < nb_buff; i++)
	{
		if(i == (nb_buff-1))
			op_flag |= MAPPER_OP_LAST;
		RCPC(mapper->m_home_cid, RPC_PRIO_MAPPER, __mapper_request,
					RPC_RECV(RPC_RECV_OBJ(size)), 
					RPC_SEND(RPC_SEND_OBJ(mapper->m_home),
						RPC_SEND_MEM(mp_buffs, sizeof(struct mapper_buff_s)),
						RPC_SEND_MEM(mp_buffs->buff_ppns, sizeof(ppn_t)*mp_buffs->max_ppns),
						RPC_SEND_OBJ(flags), RPC_SEND_OBJ(op_flag)));
	}
	return size;
}

//Atomically read from the mapper content
size_t mapper_read(struct mapper_s* mapper, struct mapper_buff_s *mp_buffs, size_t nb_buff, uint_t flags)
{
	return mapper_request(mapper, mp_buffs, nb_buff, 1, flags);
}

//Atomically write to the mapper content
size_t mapper_write(struct mapper_s* mapper, struct mapper_buff_s *mp_buffs, size_t nb_buff, uint_t flags)
{
	return mapper_request(mapper, mp_buffs, nb_buff, 0, flags);
}

MAPPER_SET_PAGE_DIRTY(mapper_default_set_page_dirty)
{
	bool_t done;
	uint_t irq_state;

	if(PAGE_IS(page, PG_DIRTY))
		return 0;

	struct mapper_s *mapper;
  
	mapper = page->mapper;

	mcs_lock(&mapper->m_lock, &irq_state);
	radix_tree_tag_set(&mapper->m_radix, page->index, TAG_PG_DIRTY);
	mcs_unlock(&mapper->m_lock, irq_state);

	done = page_set_dirty(page);  

	assert(done == true);
	return 1;
}

MAPPER_CLEAR_PAGE_DIRTY(mapper_default_clear_page_dirty)
{
	uint_t irq_state;
	bool_t done;

	if(!(PAGE_IS(page, PG_DIRTY)))
		return 0;

	struct mapper_s *mapper;
  
	mapper = page->mapper;

	mcs_lock(&mapper->m_lock, &irq_state);
	radix_tree_tag_clear(&mapper->m_radix, page->index, TAG_PG_DIRTY);
	mcs_unlock(&mapper->m_lock, irq_state);

	done = page_clear_dirty(page);

	assert(done == true);

	return 1;
}

MAPPER_SYNC_PAGE(mapper_default_sync_page)
{
	struct mapper_s *mapper;
	uint_t irq_state;
	error_t err;
	bool_t done;

	mapper = page->mapper;
	err    = 0;
    
	if(PAGE_IS(page, PG_DIRTY)) 
	{
		err = mapper->m_ops->writepage(page, MAPPER_SYNC_OP);
    
		if(err) goto MAPPER_SYNC_ERR;
    
		mcs_lock(&mapper->m_lock, &irq_state);
		radix_tree_tag_clear(&mapper->m_radix, page->index, TAG_PG_DIRTY);
		mcs_unlock(&mapper->m_lock, irq_state);
    
		done = page_clear_dirty(page);
		assert(done == true);
	}
  
MAPPER_SYNC_ERR:
	return err;
}

MAPPER_READ_PAGE(mapper_default_read_page)
{
	page_zero(page);
	return 0;
}

MAPPER_WRITE_PAGE(mapper_default_write_page)
{
	return 0;
}
