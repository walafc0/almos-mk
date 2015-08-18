/*
 * mm/heap_manager.c - kernel heap manager
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
#include <spinlock.h>
#include <bits.h>
#include <config.h>
#include <kdmsg.h>
#include <thread.h>
#include <cluster.h>
#include <page.h>
#include <ppm.h>
#include <pmm.h>
#include <heap_manager.h>

#define CACHE_LINE_SIZE CONFIG_CACHE_LINE_SIZE

struct block_info_s
{
	uint_t busy:1;
	uint_t size:31;
	struct heap_manager_s *heap_mgr;
} __attribute__ ((packed));

typedef struct block_info_s block_info_t;

error_t heap_manager_init(struct heap_manager_s *heap_mgr,
			  uint_t heap_base,
			  uint_t heap_start, 
			  uint_t heap_limit)
{
	block_info_t *info;

	spinlock_init(&heap_mgr->lock, "Heap Mgr");  
	heap_mgr->flags   = 0;
	info              = (block_info_t*)heap_start;
	info->size        = (char*)heap_limit - (char*)info;
	info->busy        = 0;
	heap_mgr->base    = heap_base;
	heap_mgr->start   = heap_start;
	heap_mgr->limit   = heap_limit;
	heap_mgr->current = heap_start;
	heap_mgr->next    = heap_start;
	return 0;
}

void* heap_manager_malloc(struct heap_manager_s *heap_mgr, size_t size, uint_t flags)
{
	block_info_t *current;
	block_info_t *next;
	register size_t effective_size;

	effective_size = size + sizeof(*current);
	effective_size = ARROUND_UP(effective_size, CACHE_LINE_SIZE);

	spinlock_lock(&heap_mgr->lock);
  
	if((uint_t)heap_mgr->next > ((uint_t)heap_mgr->limit - effective_size))
		current = (block_info_t*)heap_mgr->start;
	else
		current = (block_info_t*)heap_mgr->next;
    
	while(current->busy || (current->size < effective_size)) 
	{
		assert((current->size != 0) && "Corrupted memory block descriptor");
       
		current = (block_info_t*) ((char*)current + current->size);
    
		if((uint_t)current >= heap_mgr->limit)
		{
			spinlock_unlock(&heap_mgr->lock);

			printk(WARNING, "WARNING: heap_manager_malloc: NO MORE AVAILABLE MEMORY FOR SIZE %d\n", 
			       effective_size);

			return NULL;
		}
	}

	if((current->size - effective_size) >= CACHE_LINE_SIZE)
	{
		next           = (block_info_t*) ((char*)current + effective_size);
		next->size     = current->size - effective_size;
		next->busy     = 0;
		heap_mgr->next = (uint_t) next;
		current->size  = effective_size;
		current->busy  = 1;
	}
	else
		current->busy  = 1;

	spinlock_unlock(&heap_mgr->lock);

	//current->signature = MEM_CHECK_VAL;
	current->heap_mgr = heap_mgr;

	return (char*)current + sizeof(*current);
}

void heap_manager_free(void *ptr)
{
	struct heap_manager_s *heap_mgr;
	block_info_t *current;
	block_info_t *next;
  
	if(ptr == NULL) return;
  
	current  = (block_info_t*) ((char*)ptr - sizeof(*current));
	heap_mgr = current->heap_mgr;
 
	spinlock_lock(&heap_mgr->lock);
	current->busy = 0;
  
	while ((next = (block_info_t*) ((char*) current + current->size)))
	{ 
		assert((next->size != 0) && "Corrupted memory block descriptor");
		if (((uint_t)next >= heap_mgr->limit) || (next->busy == 1))
			break;

		current->size += next->size;
	}

	if((uint_t)current < heap_mgr->next)
		heap_mgr->next = (uint_t) current;
  
	spinlock_unlock(&heap_mgr->lock);
}

