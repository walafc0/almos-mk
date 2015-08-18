/*
 * mm/ppm.h - Per-cluster Physical Pages Manager Interface
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

#ifndef _PPM_H_
#define _PPM_H_

#include <types.h>
#include <list.h>
#include <spinlock.h>
#include <wait_queue.h>

#define PPM_MAX_ORDER     CONFIG_PPM_MAX_ORDER
#define PPM_MAX_WAIT      PPM_MAX_ORDER 
#define PPM_LAST_ORDER    PPM_MAX_ORDER -  1

struct ppm_s;
struct page_s;
struct boot_info_s;
struct ppm_dqdt_req_s;

/**
 * @ppm           Pointer to ppm object
 * @return        Total pages number
 **/
#define ppm_get_pages_nr(ppm)

/**
 * @ppm           Pointer to ppm object
 * @return        Pointer to ppm's cluster
 **/
#define ppm_get_cluster(ppm)


/**
 * Initializes the given PPM object
 * 
 * @ppm           PPM object to initialize
 * @begin         Begin physical address
 * @limit         Limit physical address
 * @vaddr_begin   Base address of its mapped virtual region
 * @vaddr_start   The first available address in this region
 * @info          Pointer to In-core Boot Information Block
 *
 * @retrun        Error code
 **/
error_t ppm_init(struct ppm_s *ppm, 
		 uint64_t begin, 
		 uint64_t limit,
		 uint_t vaddr_begin,
		 uint_t vaddr_start,
		 struct boot_info_s *info);

/**
 * Gets the kernel virtual address of given page
 * 
 * @page         Pointer to page descriptor
 * @return       Its kernel virtual address
 **/
inline void* ppm_page2addr(struct page_s *page);

/**
 * Gets the PPN of given page
 * 
 * @page         Pointer to page descriptor
 * @return       Its PPN
 **/
inline ppn_t ppm_page2ppn(struct page_s *page);

/**
 * Gets the page descriptor of given PPN
 * 
 * @ppm          The manager of the given ppn
 * @ppn          the physical page number
 * @return       Its page descriptor
 **/
inline struct page_s* ppm_ppn2page(struct ppm_s *ppm, ppn_t ppn);

/**
 * Gets the page descriptor of the given kernel virtual address
 *
 * @ppm          The manager of the given addr
 * @addr         A kernel virtual address
 * @return       Its page descriptor
 */
inline struct page_s* ppm_addr2page(struct ppm_s *ppm, void *addr);

/**
 * Gets the mapped kernel virtual address of the given ppn
 *
 * @ppm          The manager of the given ppn
 * @ppn          PPN to look for
 * @return       The kernel virtual address where ppn is mapped
 */
inline void* ppm_ppn2vma(struct ppm_s *ppm, ppn_t ppn);

/**
 * Gets the PPN of the given kernel virtual address
 *
 * @ppm          The manager of the given ppn
 * @vma          The virtual memory address to look for
 * @return       Its PPN
 */
inline ppn_t ppm_vma2ppn(struct ppm_s *ppm, void* vma);

/**
 * This is the low-level pages allocation function
 * In normal use, you do not need to call it directly
 *
 * The recommanded way to get physical pages is to 
 * call the generic allocator (see kmem.h)
 *
 * @ppm          PPM object to get pages from
 * @order        Power of 2 contiguous pages
 * @flags        Allocation flags (see kmem.h)
 *
 * @return       Pointer to the first page descriptor if success, NULL otherwise
 **/
struct page_s* ppm_alloc_pages(struct ppm_s *ppm, uint_t order, uint_t flags);

/**
 * This is the low-level pages free function
 * In normal use, you do not need to call it directly
 * 
 * The recommanded way to free previously allocated 
 * physical pages is to call the generic allocator (see kmem.h)
 *
 * @page         Pointer to the first page descriptor
 **/
void ppm_free_pages(struct page_s *page);

/////////////////////////////////////////////
///             Private Section           ///
/////////////////////////////////////////////
#undef ppm_get_pages_nr
#define ppm_get_pages_nr(_ppm)   ((_ppm)->pages_nr)

#undef ppm_get_cluster
#define ppm_get_cluster(_ppm) list_container((_ppm), struct cluster_s, ppm)

struct buddy_list_s
{
	struct list_entry root;
	uint_t pages_nr;
};

struct ppm_s
{
	uint_t signature;
	spinlock_t lock;
	uint_t ppn_start;
	uint_t vaddr_begin;
	uint_t vaddr_start;
	struct buddy_list_s free_pages[PPM_MAX_ORDER];
	struct page_s *pages_tbl;
	uint_t pages_nr;
	uint_t free_pages_nr;
	uint_t uprio_pages_min;
	uint_t kprio_pages_min;
	uint_t urgent_pages_min;
	uint64_t begin;
	spinlock_t wait_lock;
	struct wait_queue_s wait_tbl[PPM_MAX_WAIT];
};

struct ppm_dqdt_req_s
{
	uint_t threshold;
	uint_t order;
};

static inline error_t ppm_wait_on(struct ppm_s *ppm, struct page_s *page)
{
	register uint_t index;

	index = (uint_t)page << 16;
	index = index >> 18;
	index = (index >> 8) ^ (index << 8);
	index = index % PPM_MAX_WAIT;

	wait_on(&ppm->wait_tbl[index], WAIT_LAST);
	return 0;
}

static inline uint_t ppm_wakeup_all(struct ppm_s *ppm, struct page_s *page)
{
	register uint_t index;
	register uint_t count;

	index = (uint_t)page << 16;
	index = index >> 18;
	index = (index >> 8) ^ (index << 8);
	index = index % PPM_MAX_WAIT;

	count = wakeup_all(&ppm->wait_tbl[index]);
	return count;
}

void ppm_print(struct ppm_s *ppm);
void ppm_assert_order(struct ppm_s *ppm);
/////////////////////////////////////////////
 
#endif	/* _PPM_H_ */
