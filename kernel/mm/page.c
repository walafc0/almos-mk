/*
 * mm/page.c - physical page descriptor and related operations
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

#include <atomic.h>
#include <types.h>
#include <list.h>
#include <thread.h>
#include <scheduler.h>
#include <cluster.h>
#include <ppm.h>
#include <pmm.h>
#include <page.h>
#include <mapper.h>
#include <kdmsg.h>
#include <vfs.h>
#include <task.h>

struct dirty_pages_s 
{
	struct list_entry	root;
	spinlock_t		lock;
};

static struct dirty_pages_s dirty_pages;

void dirty_pages_init()
{
	list_root_init(&dirty_pages.root);
	spinlock_init(&dirty_pages.lock, "Dirty Pages");
}

/* TODO: we should use on dirty list per ppm & one kvfsd per cluster */
bool_t page_set_dirty(struct page_s *page)
{
	bool_t isDirty = false;

	spinlock_lock(&dirty_pages.lock);

	if(!(PAGE_IS(page, PG_DIRTY)))
	{
		PAGE_SET(page, PG_DIRTY);
  
		list_add_first(&dirty_pages.root, &page->private_list);
		isDirty = true;
	}

	spinlock_unlock(&dirty_pages.lock);
  
	if(isDirty)
	{
		vfs_dmsg(1, "%s: called for page #%d (@%x), of node %s\n",
			 __FUNCTION__,
			 page->index,
			 page,
			 (page->mapper == NULL) ? 
			 "Unknown page" : ((page->mapper->m_inode == NULL) ? 
					   "Unknwon Mapper" : "File mapper"));
	}

	return isDirty;
}

bool_t page_clear_dirty(struct page_s *page)
{
	bool_t isDirty = false;

	spinlock_lock(&dirty_pages.lock);

	if(PAGE_IS(page, PG_DIRTY))
	{
		PAGE_CLEAR(page, PG_DIRTY);
		list_unlink(&page->private_list);
		isDirty = true;
	}

	spinlock_unlock(&dirty_pages.lock);

	return isDirty;
}

void sync_all_pages(void) 
{
	struct page_s *page;
	struct mapper_s *mapper;

	spinlock_lock(&dirty_pages.lock);

	while(!list_empty(&dirty_pages.root)) 
	{
		page = list_first(&dirty_pages.root, struct page_s, private_list);
		spinlock_unlock(&dirty_pages.lock);
		mapper = page->mapper;
 
		page_lock(page);
		mapper->m_ops->sync_page(page);
		page_unlock(page);

		printk(INFO, "INFO: Node %s, Page #%d (@%x) Has Been Flushed To Disk\n", 
		       (page->mapper) ? ((page->mapper->m_inode) ? "File mapper" 
					 : "Uknown Mapper") : "Unknown Page", page->index, page); 
    
		spinlock_lock(&dirty_pages.lock);
	}

	spinlock_unlock(&dirty_pages.lock);
}

inline struct ppm_s* page_get_ppm(struct page_s *page)
{
	//FIXME
	//return &clusters_tbl[page->cid].cluster->ppm;
	return &current_cluster->ppm;
}

void page_lock(struct page_s *page)
{
	spin_lock(&page->lock);

	if(PAGE_IS(page, PG_LOCKED))
	{
		wait_on(&page->wait_queue, WAIT_LAST);
		spin_unlock_nosched(&page->lock);
		sched_sleep(current_thread);
	}
	else
	{
		PAGE_SET(page, PG_LOCKED);
		spin_unlock(&page->lock);
	}
}

void page_unlock(struct page_s *page)
{
	register bool_t isEmpty;
 
	spin_lock(&page->lock);
  
	isEmpty = wait_queue_isEmpty(&page->wait_queue);

	if(isEmpty == false)
		wakeup_one(&page->wait_queue, WAIT_FIRST);
	else
		PAGE_CLEAR(page, PG_LOCKED);

	spin_unlock(&page->lock);
}


static void page_to_free(struct page_s *page)
{
	assert((page->state == PGINVALID) || 
	       (page->state == PGVALID)   ||
	       (page->state == PGINIT));

	if(page_refcount_get(page) != 0)
	{
		printk(ERROR, "ERROR: %s: cpu %d, pid %d, tid %x, page %x, state %x, count %d [%d]\n",
		       __FUNCTION__,
		       cpu_get_id(),
		       current_task->pid,
		       current_thread,
		       page,
		       page->state,
		       page_refcount_get(page),
		       cpu_time_stamp());
	}
	page->state = PGFREE;
}

static void page_to_invalid(struct page_s *page)
{
	assert((page->state == PGFREE)  || 
	       (page->state == PGVALID) ||
	       (page->state == PGLOCKEDIO));
  
	if(page->state == PGLOCKEDIO)
	{
		page->state = PGINVALID; 
		page_unlock(page);
		return;
	}
 
	page->state = PGINVALID;
}

static void page_to_valid(struct page_s *page)
{
	assert((page->state == PGINVALID) || 
	       (page->state == PGLOCKED)  ||
	       (page->state == PGLOCKEDIO));
  
	if(page->state == PGINVALID)
	{
		page->state = PGVALID;
		return;
	}

	page->state = PGVALID;
	page_unlock(page);
}

static void page_to_locked_io(struct page_s *page)
{
	assert((page->state == PGINVALID) || 
	       (page->state == PGVALID));

	page_lock(page);
	page->state = PGLOCKEDIO;
}

static void page_to_locked(struct page_s *page)
{
	assert(page->state == PGVALID);
	page_lock(page);
	page->state = PGLOCKED;
}

void page_state_set(struct page_s *page, page_state_t new_state)
{
	switch(new_state)
	{
	case PGFREE:
		page_to_free(page);
		return;

	case PGINVALID:
		page_to_invalid(page);
		return;

	case PGVALID:
		page_to_valid(page);
		return;

	case PGLOCKEDIO:
		page_to_locked_io(page);
		return;

	case PGLOCKED:
		page_to_locked(page);
		return;

	case PGRESERVED:
		refcount_set(&page->count,1);
	case PGINIT:
		page->state = new_state;
		return;

	default:
		printk(ERROR, "ERROR: %s: Unknown Asked State %d\n", new_state);
		return;
	}
}

void page_copy(struct page_s *dst, struct page_s *src)
{
	register uint_t size;
	void *src_addr;
	void *dst_addr;
  
	assert(dst->order == src->order);
	size = (1 << dst->order) * PMM_PAGE_SIZE;

	src_addr = ppm_page2addr(src);
	dst_addr = ppm_page2addr(dst);

	memcpy(dst_addr,src_addr,size);		/* TODO: OPTIMIZATION */
	//page_state_set(dst,PGVALID);
}

void page_zero(struct page_s *page)
{
	register uint_t size;
	void *paddr;
	size = (1 << page->order) * PMM_PAGE_SIZE;
	paddr = ppm_page2addr(page);

	memset(paddr,0,size);        /* TODO: OPTIMIZATION */
	//page_state_set(page,PGVALID);
}


void page_print(struct page_s *page)
{
	printk(DEBUG,"Page (%x,%x) {state %d, flags %x, cid %d, order %d, count %d, index %x, mapper %x, private %x}\n",
	       page,
	       ppm_page2addr(page),
	       page->state,
	       page->flags, 
	       page->cid, 
	       page->order, 
	       refcount_get(&page->count), 
	       page->index, 
	       page->mapper, 
	       page->private);
}
