/*
 * mm/pmm.h - physical memory-mangement interface
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

#ifndef _PMM_H_
#define _PMM_H_

#include <types.h>

/** Page Related Macros */
#define PMM_PAGE_MASK 
#define PMM_PAGE_SHIFT
#define PMM_PAGE_SIZE
#define PPM_HUGE_PAGE_MASK
#define PPM_HUGE_PAGE_SHIFT
#define PMM_HUGE_PAGE_SIZE

/** Generic Page Attributes */
#define PMM_READ  
#define PMM_WRITE  
#define PMM_EXECUTE
#define PMM_USER
#define PMM_PRESENT
#define PMM_DIRTY
#define PMM_ACCESSED
#define PMM_CACHED
#define PMM_GLOBAL
#define PMM_HUGE
#define PMM_COW
#define PMM_MIGRATE
#define PMM_SWAP
#define PMM_LOCKED
#define PMM_CLEAR

/** Page flags */
#define PMM_TEXT
#define PMM_DATA
#define PMM_UNKNOWN
#define PMM_OTHER_TASK
#define PMM_LAZY

/** Page Fault Report */
#define pmm_except_isWrite(flags)
#define pmm_except_isExecute(flags)
#define pmm_except_isPresent(flags)
#define pmm_except_isRights(flags) 
#define pmm_except_isInKernelMode(flags)

/** 
 * Structure provided 
 * by this interface 
 */

struct cluster_s;

/* Page Information */
struct pmm_page_info_s
{
	uint_t attr;
	uint_t ppn;
	struct cluster_s *cluster;
	bool_t isAtomic;
	void *data;
};

typedef struct pmm_page_info_s pmm_page_info_t;

/** 
 * Structure & Operations must be implemented 
 * by hardware-specific low-level code 
 */

/* Physical Memory Manager */
struct pmm_s;			

/* Init-task, initialization function ( to be called once) */
error_t pmm_bootstrap_init(struct pmm_s *pmm, uint_t arg);

/* Physical Memory Manager Initialization/Duplication/Release/Destruction */
error_t pmm_init(struct pmm_s *pmm, struct cluster_s *cluster);
error_t pmm_dup(struct pmm_s *dst, struct pmm_s *src);
error_t pmm_release(struct pmm_s *pmm);
error_t pmm_destroy(struct pmm_s *pmm);

/* Page related operations*/
error_t pmm_lock_page(struct pmm_s *pmm, vma_t vaddr, pmm_page_info_t *info);
error_t pmm_unlock_page(struct pmm_s *pmm, vma_t vaddr, pmm_page_info_t *info);
error_t pmm_set_page(struct pmm_s *pmm, vma_t vaddr, pmm_page_info_t *info);
error_t pmm_get_page(struct pmm_s *pmm, vma_t vaddr, pmm_page_info_t *info);
struct ppm_s* pmm_ppn2ppm(ppn_t ppn);
inline cid_t pmm_ppn2cid(ppn_t ppn);
inline vma_t pmm_ppn2vma(ppn_t ppn);

/* Region related operations */
error_t pmm_region_dup(struct pmm_s *dst, struct pmm_s *src, vma_t vaddr, vma_t limit, uint_t flags);
error_t pmm_region_map(struct pmm_s *pmm, vma_t vaddr, uint_t pages_nr, pmm_page_info_t *info);
error_t pmm_region_unmap(struct pmm_s *pmm, vma_t vaddr, uint_t pages_nr, uint_t flags);
error_t pmm_region_attr_set(struct pmm_s *pmm, vma_t vaddr, uint_t pages_nr, uint_t attr);

/* TLB enable/disable */
void pmm_tlb_enable(uint_t flags);
void pmm_tlb_disable(uint_t flags);
void pmm_tlb_flush_vaddr(vma_t vaddr, uint_t flags);

/* CPU cache flush */
void pmm_cache_flush_vaddr(vma_t vaddr, uint_t flags);
void pmm_cache_flush_raddr(vma_t vaddr, uint_t cid, uint_t flags);
void pmm_cache_flush(uint_t flags);

/* Debug only */
void pmm_print(struct pmm_s *pmm);

/* Interface Implementation */
#include <arch-pmm.h>

#endif	/* _PMM_H_ */
