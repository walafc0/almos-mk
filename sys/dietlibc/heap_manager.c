/*
  This file is part of MutekP.
  
  MutekP is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  MutekP is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with MutekP; if not, write to the Free Software Foundation,
  Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
  UPMC / LIP6 / SOC (c) 2009
  Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#include <sys/types.h>
#include <sys/syscall.h>
#include <cpu-syscall.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#ifndef CONFIG_LIBC_MALLOC_DEBUG
#define CONFIG_LIBC_MALLOC_DEBUG  0
#endif

#undef dmsg_r
#define dmsg_r(...)


typedef struct heap_manager_s
{
	pthread_spinlock_t lock;

	volatile uint_t limit   __CACHELINE;
	volatile uint_t current __CACHELINE;
	volatile uint_t next    __CACHELINE;
	volatile uint_t last    __CACHELINE;

	uint_t start;
	uint_t sbrk_size;
}heap_manager_t;

struct block_info_s
{
	uint_t busy:1;
	uint_t size:31;
	struct block_info_s *ptr;
} __attribute__ ((packed));

typedef struct block_info_s block_info_t;

static heap_manager_t heap_mgr;
extern uint_t __bss_end;
static int cacheline_size;
static int page_size;

#define ARROUND_UP(val, size) ((val) & ((size) -1)) ? ((val) & ~((size)-1)) + (size) : (val)

void __heap_manager_init(void)
{
	block_info_t *info;
	uint_t initial_size;

	initial_size = (uint_t) cpu_syscall(NULL, NULL, NULL, NULL, SYS_SBRK);

	if(initial_size < 4096)
		initial_size = 0x100000; /* subject to segfault */

	pthread_spin_init(&heap_mgr.lock, 0);
	heap_mgr.start = (uint_t)&__bss_end;
	heap_mgr.limit = heap_mgr.start + initial_size;

	info = (block_info_t*)heap_mgr.start;
	info->busy = 0;
	info->size = heap_mgr.limit - heap_mgr.start;
	info->ptr = NULL;

#if CONFIG_LIBC_MALLOC_DEBUG
	printf("%s: start %x, info %x, size %d\n",
	       __FUNCTION__,
	       (unsigned)heap_mgr.start,
	       (unsigned)info,
	       ((block_info_t*)heap_mgr.start)->size);
#endif

	heap_mgr.current = heap_mgr.start;
	heap_mgr.next = heap_mgr.start;
	heap_mgr.last = heap_mgr.start;
	cacheline_size = sysconf(_SC_CACHE_LINE_SIZE);
	cacheline_size = (cacheline_size <= 0) ? 64 : cacheline_size;
	page_size = sysconf(_SC_PAGE_SIZE);
	page_size = (page_size <= 0) ? 4096 : page_size;

	heap_mgr.sbrk_size = 2 * (initial_size / page_size);
}

void* do_malloc(size_t, int);

void* malloc(size_t size)
{
	block_info_t *info;
	void *ptr;
	int  err;
	int tries_nr;
	uint_t blksz;

	int pid = getpid();

#if 1
	dmsg_r(stderr, "libc: %s: pid %d, tid %d, 0x%x started\n",
	       __FUNCTION__, pid, (int)pthread_self(), (unsigned)&heap_mgr.lock);
#endif

#undef  MALLOC_THRESHOLD
#define MALLOC_THRESHOLD  5

#undef  MALLOC_LIMIT
#define MALLOC_LIMIT      20

	tries_nr = 0;
	ptr      = NULL;
	err      = 0;

	while((ptr == NULL) && (tries_nr < MALLOC_LIMIT) && (err != ENOMEM))
	{
		ptr = do_malloc(size, tries_nr);

		if(ptr == NULL)
		{
			blksz = size / page_size;
			blksz = (blksz < heap_mgr.sbrk_size) ? heap_mgr.sbrk_size : (blksz + heap_mgr.sbrk_size); 

			err   = (int) cpu_syscall((void*)heap_mgr.limit,
						  (void*)blksz,
						  NULL, NULL, SYS_SBRK);
			if(err != 0)
				continue;

			blksz = blksz * page_size;

			pthread_spin_lock(&heap_mgr.lock);

			heap_mgr.limit += blksz;

			info = (block_info_t*)heap_mgr.last;

			if(info->busy)
			{
				info          = (block_info_t*)(heap_mgr.last + info->size);
				info->busy    = 0;
				info->size    = blksz;
				info->ptr     = NULL;
				heap_mgr.last = (uint_t)info;
			}
			else
				info->size += blksz;

			pthread_spin_unlock(&heap_mgr.lock);

			if(++tries_nr > MALLOC_THRESHOLD)
				pthread_yield();
		}
	}

#if 1
	dmsg_r(stderr, "libc: %s: pid %d, tid %d, 0x%x return 0x%x\n",
	       __FUNCTION__, pid, (unsigned)pthread_self(), (unsigned)&heap_mgr.lock, (unsigned)ptr);
#endif	

	return ptr;
}

void* do_malloc(size_t size, int round)
{
	block_info_t *current;
	block_info_t *next;
	register size_t effective_size;

	effective_size = size + sizeof(*current);
	effective_size = ARROUND_UP(effective_size, cacheline_size);

	if((round == 0) && (heap_mgr.next > (heap_mgr.limit - effective_size)))
		return NULL;

	pthread_spin_lock(&heap_mgr.lock);

	if(heap_mgr.next > (heap_mgr.limit - effective_size))
		current = (block_info_t*)heap_mgr.start;
	else
		current = (block_info_t*)heap_mgr.next;

#if CONFIG_LIBC_MALLOC_DEBUG
	int cpu;
	pthread_attr_getcpuid_np(&cpu);

	fprintf(stderr,
		"%s: cpu %d Started [sz %d, esz %d, pg %d, cl %d, start %x, next %x,"
		"last %x, current %x, limit %x, busy %d, size %d]\n",
		__FUNCTION__,
		cpu,
		size, 
		effective_size,
		page_size,
		cacheline_size,
		(unsigned)heap_mgr.start,
		(unsigned)heap_mgr.next,
		(unsigned)heap_mgr.last,
		(unsigned)current,
		(unsigned)heap_mgr.limit,
		current->busy,
		current->size);
#endif

	while(current->busy || (current->size < effective_size)) 
	{
		if((current->busy) && (current->size == 0))
			fprintf(stderr, "Corrupted memory block descriptor: @0x%x\n", (unsigned int)current);

		current = (block_info_t*) ((char*)current + current->size);

    
		if((uint_t)current >= heap_mgr.limit)
		{
			pthread_spin_unlock(&heap_mgr.lock);

#if CONFIG_LIBC_MALLOC_DEBUG
			int cpu;
			pthread_attr_getcpuid_np(&cpu);

			fprintf(stderr, "[%s] cpu %d, pid %d, tid %d, limit 0x%x, ended with ENOMEM\n",
				__FUNCTION__,
				cpu,
				(unsigned)getpid(),
				(unsigned)pthread_self(),
				(unsigned)heap_mgr.limit);
#endif
			return NULL;
		}
		
		dmsg_r(stderr, "%s: current 0x%x, current->busy %d, current->size %d, limit %d [%d]\n",
		     __FUNCTION__,
		     (unsigned)current,
		     (unsigned)current->busy,
		     (unsigned)current->size,
		     (unsigned)(heap_mgr.limit - sizeof(*current)),
		     (unsigned)heap_mgr.limit);
	}

	if((current->size - effective_size) >= (uint_t)cacheline_size)
	{
		next          = (block_info_t*) ((uint_t)current + effective_size);
		next->size    = current->size - effective_size;
		next->busy    = 0;    
		current->size = effective_size;
		current->busy = 1;
	}
	else
	{
		current->busy = 1;
		next          = (block_info_t*)((uint_t)current + current->size);
	}

	heap_mgr.next = (uint_t)next;
	current->ptr  = NULL;

#if CONFIG_LIBC_MALLOC_DEBUG
	fprintf(stderr, "%s: cpu %d End [ 0x%x ]\n", __FUNCTION__, cpu, ((unsigned int)current) + sizeof(*current));
#endif

	if(((uint_t)next + next->size) >= heap_mgr.limit)
		heap_mgr.last = (uint_t)next;

	pthread_spin_unlock(&heap_mgr.lock);
	return (char*)current + sizeof(*current);
}

void free(void *ptr)
{
	block_info_t *current;
	block_info_t *next;
	size_t limit;

	if(ptr == NULL)
		return;
	
	current = (block_info_t*) ((char*)ptr - sizeof(*current));
	current = (current->ptr != NULL) ? current->ptr : current;

	pthread_spin_lock(&heap_mgr.lock);
	limit = heap_mgr.limit;
	current->busy = 0;

	while (1)
	{ 
		next = (block_info_t*) ((char*) current + current->size);

		if (((uint_t)next >= limit) || (next->busy == 1))
			break;

		current->size += next->size;
	}

	if((uint_t)current < heap_mgr.next)
		heap_mgr.next = (uint_t) current;
  
	pthread_spin_unlock(&heap_mgr.lock);
}


void* realloc(void *ptr, size_t size)
{
	block_info_t *info;
	uint_t old_size;
	void* new_zone;
	void* block;

	if(!ptr)
		return malloc(size);

	if(!size)
	{
		free(ptr);
		return NULL;
	}

	/* Try to reuse cache lines */
	info = (block_info_t*)((char*)ptr - sizeof(*info));
	old_size = info->size; 
 
	if(size <= old_size)    
		return ptr;

	/* Allocate new block */
	if(info->ptr == NULL)
	{
		new_zone = malloc(size);
	}
	else
	{
		info = info->ptr;
		new_zone = valloc(size);
	}

	if(new_zone == NULL)
		return NULL;

	memcpy(new_zone, ptr, old_size);

	block = (char*)info + sizeof(*info);

	free(block);
	return new_zone;
}

void* calloc(size_t nmemb, size_t size)
{
	void *ptr = malloc(nmemb * size);

	if(ptr == NULL)
		return NULL;

	memset(ptr, 0, (nmemb * size));
  
	return ptr;
}

void* valloc(size_t size)
{
	void *ptr;
	uintptr_t block;
	block_info_t *info;
	size_t asked_size;
  
	asked_size = size;
	size  = ARROUND_UP(size, page_size);
	block = (uintptr_t) malloc(size + page_size);
  
	if(block == 0) return NULL;
  
	ptr   = (void*)((block + page_size) - (block % page_size));
	info  = (block_info_t*) ((char*)ptr - sizeof(*info));

	info->busy = 1;
	info->size = asked_size;
	info->ptr  = (block_info_t*)(block - sizeof(*info));

	return ptr;
}
