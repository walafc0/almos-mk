/*
 * kern/kfifo.c - Lock-Free Flexible Buffer
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

#include <cpu.h>
#include <kmem.h>
#include <kdmsg.h>
#include <kfifo.h>


#if CONFIG_KFIFO_DEBUG
void kfifo_print(struct kfifo_s *kfifo)
{
	uint_t i;
  
	printk(DEBUG, "%s: size %d, rdidx %d, wridx %d : [", 
	       __FUNCTION__, 
	       kfifo->size,
	       kfifo->rdidx,
	       kfifo->wridx);

	for(i = 0; i < kfifo->size; i++)
		printk(DEBUG, "%d:%d, ", i, (int)kfifo->tbl[i]);
  
	printk(DEBUG, "\b\b]\n");
}
#endif


//FIXME: use a real read barrier ?
#define cpu_mbarrier cpu_wbflush()

KFIFO_PUT(single_event_put)
{
	size_t wridx;
	size_t rdidx;
	size_t size;

	size  = kfifo->size;
	wridx = kfifo->wridx;
	rdidx = kfifo->rdidx;

	cpu_invalid_dcache_line((void*)&kfifo->rdidx);

	if(((wridx + 1) % size) == rdidx)
		return EAGAIN;

	kfifo->tbl[wridx] = val;
	cpu_mbarrier;

	kfifo->wridx = (wridx + 1) % size;	
	cpu_wbflush();

	return 0;
}

KFIFO_PUT(multi_event_put)
{
	size_t wridx;
	size_t rdidx;
	size_t size;
	bool_t isAtomic;
	size_t threshold;
	volatile size_t cntr;

	size      = kfifo->size;
	cntr      = 0;
	threshold = 10000;

	do
	{
		wridx = kfifo->wridx;
		rdidx = kfifo->rdidx;
  
		if(((wridx + 1) % size) == rdidx)
			return EAGAIN;

		isAtomic = cpu_atomic_cas((void*)&kfifo->wridx, 
					  wridx,
					  (wridx + 1) % kfifo->size);
		
		if(cntr++ == threshold)
			return EBUSY;

	}while(isAtomic == false);

	kfifo->tbl[wridx] = val;
	cpu_wbflush();

	cpu_invalid_dcache_line((void*)&kfifo->rdidx);

	return 0;
}


KFIFO_GET(single_event_get)
{
	size_t rdidx;
	size_t wridx;

	rdidx = kfifo->rdidx;
	wridx = kfifo->wridx;

	cpu_invalid_dcache_line((void*)&kfifo->wridx);

	if(rdidx == wridx)
		return EAGAIN;
	
	*val = kfifo->tbl[rdidx];
	cpu_mbarrier; 

	kfifo->rdidx = (rdidx + 1) % kfifo->size;
	cpu_wbflush();

	return 0;
}

KFIFO_GET(multi_event_get)
{
	size_t rdidx;
	size_t wridx;
	size_t size;
	bool_t isAtomic;
	size_t threshold;
	volatile size_t cntr;

	size      = kfifo->size;
	cntr      = 0;
	threshold = 10000;

	do
	{
		rdidx = kfifo->rdidx;
		wridx = kfifo->wridx;
		
		if(rdidx == wridx)
			return EAGAIN;

		*val = kfifo->tbl[rdidx];

		isAtomic = cpu_atomic_cas((void*)&kfifo->rdidx, 
					  rdidx, 
					  (rdidx + 1) % size);
		if(cntr++ == threshold)
			return EBUSY;

	}while(isAtomic == false);

	cpu_invalid_dcache_line((void*)&kfifo->wridx);

	return 0;
}

error_t kfifo_init(struct kfifo_s *kfifo, size_t size, uint_t mode)
{
	kmem_req_t req;

	req.type  = KMEM_GENERIC;
	req.flags = AF_KERNEL | AF_ZERO;
	req.size  = sizeof(void*) * size;

	kfifo->tbl = kmem_alloc(&req);

	if(kfifo->tbl == NULL)
		return ENOMEM;
  
	kfifo->wridx = 0;
	kfifo->rdidx = 0;
	kfifo->size  = size;

	mode &= KFIFO_MASK;

	kfifo->ops.put = single_event_put;
	kfifo->ops.get = single_event_get;

	if(mode & KFIFO_MW)
		kfifo->ops.put = multi_event_put;

	if(mode & KFIFO_MR)
		kfifo->ops.get = multi_event_get;

	return 0;
}

void kfifo_destroy(struct kfifo_s *kfifo)
{
	kmem_req_t req;
  
	req.type  = KMEM_GENERIC;
	req.flags = AF_KERNEL;
	req.size  = kfifo->size;
	req.ptr   = kfifo->tbl;

	kmem_free(&req);
}
