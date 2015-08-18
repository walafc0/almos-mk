/*
 * mm/heap_manager.h - kernel heap manager used as a low-level
 * allocator for generic types (See kmem.h).
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

#ifndef _HEAP_MANAGER_H_
#define _HEAP_MANAGER_H_

#include <config.h>
#include <types.h>
#include <spinlock.h>

#define  KMEM_BOOT_STAGE   1

struct heap_manager_s
{
	spinlock_t lock;
	uint_t flags;
	uint_t base;
	uint_t start;
	uint_t limit;
	uint_t current;
	uint_t next;
};

error_t heap_manager_init(struct heap_manager_s *heap_mgr,
			  uint_t heap_base,
			  uint_t heap_start, 
			  uint_t heap_limit);

void* heap_manager_malloc(struct heap_manager_s *heap_mgr, size_t size, uint_t flags);
void  heap_manager_free  (void *ptr);

#endif	/* _HEAP_MANAGER_H_ */
