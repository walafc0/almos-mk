/*
 * vm_region.c: virtual memory region related operations
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

#ifndef _VM_REGION_H_
#define _VM_REGION_H_

#include <types.h>
#include <rbtree.h>
//#include <spinlock.h>
#include <mcs_sync.h>
#include <atomic.h>
#include <mapper.h>
#include <vfs.h>

struct vmm_s;
struct vm_region_s;

#define VM_REG_SHARED   0x0001
#define VM_REG_PRIVATE  0x0002
#define VM_REG_PVSH     0x0003
#define VM_REG_ANON     0x0004
#define VM_REG_STACK    0x0008
#define VM_REG_LOCKED   0x0010
#define VM_REG_HUGETLB  0x0020
#define VM_REG_FIXED    0x0040
#define VM_REG_DEV      0x0080
#define VM_REG_FILE     0x0100
#define VM_REG_LAZY     0x0200
#define VM_REG_INIT     0x0400
#define VM_REG_HEAP     0x0800
#define VM_REG_INST     0x1000

#define VM_REG_NON      0x00
#define VM_REG_RD       0x01
#define VM_REG_WR       0x02
#define VM_REG_EX       0x04

#define VM_REG_CAP_RD   0x08
#define VM_REG_CAP_WR   0x10
#define VM_REG_CAP_EX   0x20

#define VM_FAILED      ((void *) -1)

/* Virtual Memory Region */
struct vm_region_s
{
	struct rb_node vm_node;
//	spinlock_t vm_lock;
	mcs_lock_t vm_lock;
	atomic_t vm_refcount;
	struct list_entry vm_list;
	struct vmm_s *vmm;
	uint_t vm_begin;
	uint_t vm_end;
	uint_t vm_start;
	volatile uint_t vm_limit;
	uint_t vm_prot;
	uint_t vm_pgprot;
	uint_t vm_flags;
	uint_t vm_offset;
	struct vm_region_op_s *vm_op;
	struct mapper_s vm_mapper;
	struct vfs_file_s vm_file;
	struct list_entry vm_shared_list;
	void *vm_data;
};

/**
 *  Methodes:
 * - page_in         : when page must be swaped in to memory
 * - page_out        : when page must be swaped out of memory
 * - page_lookup     : when a specific page descriptor is needed
 * - page_fault      : when a memory access generate a page fault
 **/
#define VM_REGION_PAGE_IN(n)     error_t (n) (struct vm_region_s *region, struct page_s *pgptr)
#define VM_REGION_PAGE_OUT(n)    error_t (n) (struct vm_region_s *region, struct page_s *pgptr)
#define VM_REGION_PAGE_LOOKUP(n) error_t (n) (struct vm_region_s *region, uint_t vaddr, struct page_s **pgptr)
#define VM_REGION_PAGE_FAULT(n)  error_t (n) (struct vm_region_s *region, uint_t vaddr, uint_t flags)

typedef VM_REGION_PAGE_IN(vm_reg_pagein_t);
typedef VM_REGION_PAGE_OUT(vm_reg_pageout_t);
typedef VM_REGION_PAGE_LOOKUP(vm_reg_lookup_t);
typedef VM_REGION_PAGE_FAULT(vm_reg_pagefault_t);

struct vm_region_op_s
{
	vm_reg_pagein_t *page_in;
	vm_reg_pageout_t *page_out;
	vm_reg_lookup_t *page_lookup;
	vm_reg_pagefault_t *page_fault;
};

extern const struct vm_region_op_s vm_region_default_op;

KMEM_OBJATTR_INIT(vm_region_kmem_init);

error_t vm_region_init(struct vm_region_s *region,
		       uint_t vma_start, 
		       uint_t vma_end, 
		       uint_t proto,
		       uint_t offset,
		       uint_t flags);

error_t vm_region_destroy(struct vm_region_s *region);

struct vm_region_s* vm_region_find(struct vmm_s *vmm, uint_t vaddr);

error_t vm_region_attach(struct vmm_s *vmm, struct vm_region_s *region);

error_t vm_region_detach(struct vmm_s *vmm, struct vm_region_s *region);

error_t vm_region_grow(struct vm_region_s *region, uint_t pages_nr);

error_t vm_region_split(struct vmm_s *vmm, 
			struct vm_region_s *region, 
			uint_t start_addr, 
			uint_t length);

error_t vm_region_resize(struct vmm_s *vmm, 
			 struct vm_region_s *region, 
			 uint_t start, 
			 uint_t end);

error_t vm_region_dup(struct vm_region_s *dst, struct vm_region_s *src);

error_t vm_region_update(struct vm_region_s *region, uint_t vaddr, uint_t flags);

#endif /* _VM_REGION_H_ */
