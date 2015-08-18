/*
 * mm/kcm.h - Per-cluster Kernel Cache Manager Interface
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

#ifndef _KCM_H_
#define _KCM_H_

#include <list.h>
#include <types.h>
#include <spinlock.h>

struct kcm_s;
struct page_s;

typedef struct page_s* (kcm_page_alloc_t)   (struct kcm_s *kcm);
typedef void           (kcm_page_free_t)    (struct kcm_s *kcm, struct page_s *page);
typedef void           (kcm_init_destroy_t) (struct kcm_s *kcm, void *ptr);

/** Kernel Cache Manager descriptor */
struct kcm_s                      
{
	spinlock_t lock;

	/* Blocks per page information */
	size_t size;
	uint_t blocks_nr;
	uint_t aligne;

	/* Active/Standby/free lists roots */
	struct list_entry activelist;
	struct list_entry busylist;
	struct list_entry freelist;

	/* freelist watermarks */
	uint_t free_pages_nr;
	uint_t free_pages_min;
	uint_t free_pages_max;

	/* Number of busy/active pages */
	uint_t busy_pages_nr;
	uint_t active_pages_nr;

	/* One-Time object constructor/destructor */
	kcm_init_destroy_t *obj_init;
	kcm_init_destroy_t *obj_destroy;

	/* Allocation/liberation interface */
	kcm_page_alloc_t *page_alloc;
	kcm_page_free_t  *page_free;
  
	/* Main Allocator chain list */
	struct list_entry list;

	/* Cache Name (Debug/state) */
	char *name;
};

/** Initialize a kernel Cache manager */
error_t kcm_init(struct kcm_s *kcm,
		 char *name,
		 size_t size,
		 uint_t aligne,
		 uint_t free_pages_min,
		 uint_t free_pages_max,
		 kcm_init_destroy_t *obj_init,
		 kcm_init_destroy_t *obj_destroy,
		 kcm_page_alloc_t *page_alloc_func,
		 kcm_page_free_t  *page_free_func);

/** Allocate any size but less than a page size */
void* kcm_alloc(struct kcm_s *kcm, uint_t flags);

/** Free previous allocated block */
void  kcm_free (void *ptr);

/** Shrink buffered pages if any, it should be called by the main allocator */
void kcm_release(struct kcm_s *kcm);

/** Default page alloc */
struct page_s* kcm_page_alloc(struct kcm_s *kcm);

/** Default page free */
void kcm_page_free(struct kcm_s *kcm, struct page_s *page);

/** Print KCM, for debug only !*/
void kcm_print(struct kcm_s *kcm);

#endif	/* _KCM_H_ */
