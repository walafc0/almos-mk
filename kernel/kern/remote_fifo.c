/*
 * kern/remote_fifo.c
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
#include <bits.h>
#include <kmem.h>
#include <thread.h>
#include <cluster.h>
#include <kdmsg.h>
#include <remote_fifo.h>


void* item_get_base(struct remote_fifo_s *remote_fifo, cid_t cid, uint_t pos)
{
	size_t base = remote_lw((void*)&remote_fifo->tbl, cid);
	return (void*)( base + (remote_fifo->slot_size * pos));
}

//set an itemm at the already allocated pos slot 
void item_set(struct remote_fifo_s *remote_fifo, cid_t cid, void *item, uint_t pos)
{
	uint_t size;
	void *item_base;
	
	size = remote_lw(&remote_fifo->item_size, cid);
	item_base = item_get_base(remote_fifo, cid, pos);
	remote_memcpy(item_base, cid, item, current_cid, size);
}

#define cpu_mbarrier cpu_wbflush()

#if 0
EVENT_FIFO_PUT(single_put)
{
	size_t wridx;
	size_t hdidx;
	size_t size;
	size_t clnb;

	size = rpc_get_size(item);
	clnb = size/CACHELINE;
	wridx = remote_lw((void*)&remote_fifo->wridx, cid);
	hdidx = remote_lw((void*)&remote_fifo->hdidx, cid);

	size  = remote_lw((void*)&remote_fifo->slot_nbr, cid);

	cpu_invalid_dcache_line((void*)&remote_fifo->hdidx);

	if(((wridx + 1) % size) == hdidx)
		return EAGAIN;

	item_set(remote_fifo, cid, item, wridx);
	cpu_mbarrier;

	remote_sw((void*)&remote_fifo->wridx, cid, (wridx + 1) % size);	
	cpu_wbflush();

	return 0;
}

EVENT_FIFO_GET(multi_get)
{
	size_t rdidx;
	size_t wridx;
	size_t size;
	bool_t isAtomic;
	size_t threshold;
	volatile size_t cntr;

	size      = remote_lw((void*)&remote_fifo->slot_nbr, cid);
	cntr      = 0;
	threshold = 10000;

	do
	{
		rdidx = remote_lw((void*)&remote_fifo->rdidx, cid);
		wridx = remote_lw((void*)&remote_fifo->wridx, cid);
		
		if(rdidx == wridx)
			return EAGAIN;

		*item = item_get_base(remote_fifo, cid, rdidx);

		isAtomic = remote_atomic_cas((void*)&remote_fifo->rdidx, cid, 
					  rdidx, 
					  (rdidx + 1) % size);
		if(cntr++ == threshold)
			return EBUSY;

	}while(isAtomic == false);

	cpu_invalid_dcache_line((void*)&remote_fifo->wridx);//FIXME

	return 0;
}
EVENT_FIFO_GET(single_get)
{
	size_t size;
	size_t rdidx;
	size_t wridx;

	size  = remote_lw((void*)&remote_fifo->slot_nbr, cid);
	rdidx = remote_lw((void*)&remote_fifo->rdidx, cid);
	wridx = remote_lw((void*)&remote_fifo->wridx, cid);

	cpu_invalid_dcache_line((void*)&remote_fifo->wridx);//FIXME

	if(rdidx == wridx)
		return EAGAIN;
	
	*item = item_get_base(remote_fifo, cid, rdidx);
	cpu_mbarrier; 

	remote_sw((void*)&remote_fifo->rdidx, cid, (rdidx + 1) % size);	
	cpu_wbflush();

	return 0;
}
#endif

size_t rf_available_empty_items(size_t total, size_t wr, size_t rd)
{
	if(wr == rd)
		return total;
	else if(wr > rd)
		return rd + (total - wr);
	else
		return rd - wr;
}

#define RF_PRINT 0
//inline 
error_t remote_fifo_put(struct remote_fifo_s *remote_fifo, cid_t cid, void *item)
{
	size_t wridx;
	size_t rdidx;
	size_t total_slot_nbr;
	uint_t irq_state;

#if RF_PRINT
	uint32_t start;
	uint32_t end;

	start = cpu_time_stamp(); //cpu_get_ticks(current_cpu);
#endif

	total_slot_nbr  = remote_lw((void*)&remote_fifo->slot_nbr, cid);

	//assert(size);//if the message is bigger than cacheline, it could wrap arround the fifo => msg is not countigius

	mcs_lock_remote(&remote_fifo->lock, cid, &irq_state);

	wridx = remote_lw((void*)&remote_fifo->wridx, cid);
	rdidx = remote_lw((void*)&remote_fifo->rdidx, cid);

	if(((wridx + 1) % total_slot_nbr) == rdidx)
	{
		mcs_unlock_remote(&remote_fifo->lock, cid, irq_state);
		return EAGAIN;
	}

	item_set(remote_fifo, cid, item, wridx);

	remote_sw((void*)&remote_fifo->wridx, cid, 
				(wridx + 1) % total_slot_nbr);
	
	mcs_unlock_remote(&remote_fifo->lock, cid, irq_state);

#if RF_PRINT
	end = cpu_time_stamp();

	printk(INFO, "[%d] %s: posting in cid %d at %d\n", 
		cpu_get_id(), __FUNCTION__, cid, end-start);
#endif
	return 0;
}


//one thread
//inline 
error_t remote_fifo_get(struct remote_fifo_s *remote_fifo, void **item)
{
	size_t rdidx;
	size_t wridx;

	rdidx = remote_fifo->rdidx;
	wridx = remote_fifo->wridx;

	//cpu_invalid_dcache_line((void*)&remote_fifo->wridx);//?
	cpu_mbarrier; 

	if(rdidx == wridx)
		return EAGAIN;
	
	*item = item_get_base(remote_fifo, current_cid, rdidx);

	return 0;
}

//one thread
//inline 
void remote_fifo_release(struct remote_fifo_s *remote_fifo)
{
	remote_fifo->rdidx = (remote_fifo->rdidx + 1) % remote_fifo->slot_nbr;
	cpu_wbflush();
}


//The following operations are assumed to be local 
error_t remote_fifo_init(struct remote_fifo_s *remote_fifo, size_t slot_nbr, size_t size)
{
	kmem_req_t req;
	size_t slot_size;

	slot_size = size;

	req.type  = KMEM_GENERIC;
	req.flags = AF_KERNEL | AF_ZERO;
	req.size  = slot_size * slot_nbr;

	remote_fifo->tbl = kmem_alloc(&req);

	if(remote_fifo->tbl == NULL)
		return ENOMEM;
  
	remote_fifo->wridx = 0;
	remote_fifo->rdidx = 0;
	remote_fifo->slot_nbr = slot_nbr;
	remote_fifo->slot_size = slot_size;
	remote_fifo->item_size = size;
	mcs_lock_init(&remote_fifo->lock, "Remote fifo");

	return 0;
}

void remote_fifo_destroy(struct remote_fifo_s *remote_fifo)
{
	kmem_req_t req;
  
	req.type  = KMEM_GENERIC;
	req.flags = AF_KERNEL;
	//req.size  = remote_fifo->cache_line_nbr;
	req.ptr   = remote_fifo->tbl;

	kmem_free(&req);
}

#if 0
#if CONFIG_EVENT_FIFO_DEBUG
void remote_fifo_print(struct remote_fifo_s *remote_fifo)
{
	uint_t i;
  
	printk(DEBUG, "%s: size %d, rdidx %d, wridx %d : [", 
	       __FUNCTION__, 
	       remote_fifo->size,
	       remote_fifo->rdidx,
	       remote_fifo->wridx);

	for(i = 0; i < remote_fifo->size; i++)
		printk(DEBUG, "%d:%d, ", i, (int)remote_fifo->tbl[i]);
  
	printk(DEBUG, "\b\b]\n");
}
#endif
#endif
