/*
 * mm/page.h - physical page descriptor and related operations
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

#ifndef _PAGE_H_
#define _PAGE_H_

#include <atomic.h>
#include <spinlock.h>
#include <types.h>
#include <list.h>
#include <wait_queue.h>

#define PG_INIT		0x001
#define PG_RESERVED	0x002
#define PG_FREE		0x004
#define PG_INLOAD	0x008
#define PG_IO_ERR	0x010
#define PG_BUFFER	0x020
#define PG_BUSY		0x040
#define PG_DIRTY	0x080
#define PG_LOCKED       0x100
#define PG_PINNED       0x200
#define PG_MIGRATE      0x400

typedef enum
{
	PGINIT = 1,
	PGRESERVED,
	PGFREE,
	PGINVALID,
	PGVALID,
	PGLOCKEDIO,
	PGLOCKED
}page_state_t;

#define PAGE_SET(page,flag)    ((page)->flags) |= (flag)
#define PAGE_CLEAR(page,flag)  ((page)->flags) &= ~(flag)
#define PAGE_IS(page,flag)     ((page)->flags) & (flag)

struct ppm_s;

struct page_s
{
	/* flags */
	uint32_t state : 4;
	uint32_t flags : 12;
	uint32_t cid   : 12;
	uint32_t order : 4;

	/* waiting threads */
	struct wait_queue_s wait_queue;

	union
	{
		uint32_t index;
		uint32_t shared;
	};

	/* Associated mapper */
	struct mapper_s *mapper;

	/* Allocator, BlkIO attached objects */
	union 
	{
		uint_t private;
		void *data;
		struct slist_entry root;            
	};

	/* Delayed dirty list */
	struct list_entry private_list;

	/* Allocation/Replacement policies list */
	struct list_entry list;

	/* Reference Count & lock */
	refcount_t count;
	slock_t    lock;
};

/**
 * Initializes page descriptor
 * 
 * @page                Pointer to descriptor
 * @cid                 Cluster id home of this page  
 **/
static inline void page_init(struct page_s *page, uint_t cid);

/**
 *  
 **/
void dirty_pages_init(void);
void sync_all_pages(void);
bool_t page_set_dirty(struct page_s *page);
bool_t page_clear_dirty(struct page_s *page);


void page_copy(struct page_s *dst, struct page_s *src);
void page_zero(struct page_s *page);
void page_lock(struct page_s *page);
void page_unlock(struct page_s *page);

void page_state_set(struct page_s *page, page_state_t new_state);
static inline page_state_t page_state_get(struct page_s *page);

inline struct ppm_s* page_get_ppm(struct page_s *page);

static inline uint_t page_refcount_get(struct page_s *page);
static inline uint_t page_refcount_up(struct page_s *page);
static inline uint_t page_refcount_down(struct page_s *page);

static inline void page_shared_init(struct page_s *page);
static inline uint_t page_shared_get(struct page_s *page);
static inline uint_t page_shared_up(struct page_s *page);
static inline uint_t page_shared_down(struct page_s *page);

void page_print(struct page_s *page);

#define MCNTL_READ           0x0
#define MCNTL_L1_iFLUSH      0x1
#define MCNTL_MOVE           0x2 /* this operation is not implemented in this version */
#define MCNTL_OPS_NR         MCNTL_MOVE //(MCNTL_MOVE + 1)

struct minfo_s
{
	uint_t mi_cid;
	uint_t mi_cx;
	uint_t mi_cy;
	uint_t mi_cz;
};

typedef struct minfo_s minfo_t;

int sys_mcntl(int op, uint_t vaddr, size_t len, minfo_t *pinfo);

////////////////////////////////////////////////////////
//                  Private Section                   //
////////////////////////////////////////////////////////

static inline void page_init(struct page_s *page, uint_t cid)
{
	wait_queue_init(&page->wait_queue, "Page");
	page->state = PGINIT;
	page->flags = 0;
	page->order = 0;
	page->cid = cid;
	refcount_init(&page->count);
	spin_init(&page->lock);
	//refcount_up(&page->count);
	page->index = 0;
	page->mapper = NULL;
	page->private = 0;
}

static inline page_state_t page_state_get(struct page_s *page)
{
	return page->state;
}

static inline uint_t page_refcount_get(struct page_s *page)
{
	return refcount_get(&page->count);
}


static inline uint_t page_refcount_up(struct page_s *page)
{
	return refcount_up(&page->count);
}

static inline uint_t page_refcount_down(struct page_s *page)
{
	return refcount_down(&page->count);
}

static inline void page_shared_init(struct page_s *page)
{
	assert(page->mapper == NULL);
	page->shared = 1;
}

static inline uint_t page_shared_get(struct page_s *page)
{
	assert(page->mapper == NULL);
	return page->shared;
}

static inline uint_t page_shared_up(struct page_s *page)
{
	assert(page->mapper == NULL);
	return page->shared ++;
}

static inline uint_t page_shared_down(struct page_s *page)
{
	assert(page->mapper == NULL);
	return page->shared --;
}

#endif	/* _PAGE_H_ */
