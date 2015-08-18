/*
 * kern/barrier.c - barrier synchronization implementaion 
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
#include <errno.h>
#include <bits.h>
#include <task.h>
#include <thread.h>
#include <list.h>
#include <cluster.h>
#include <pmm.h>
#include <kmagics.h>
#include <scheduler.h>
#include <atomic.h>
#include <kmem.h>
#include <system.h>
#include <barrier.h>

#define PTHREAD_BARRIER_SERIAL_THREAD 1

typedef struct wqdb_record_s
{
	uint_t event;
	void* listner;
} wqdb_record_t;

struct wait_queue_db_s
{
	wqdb_record_t tbl[0];
};

typedef struct wait_queue_db_s wqdb_t;


static void barrier_do_broadcast(struct barrier_s *barrier)
{
	register uint_t tm_first;
	register uint_t tm_last;
	register uint_t tm_start;
	register uint_t wqdbsz;
	register uint_t tm_end;
	register uint_t ticket;
	register uint_t index;
	register uint_t count;
	register uint_t event;
	register void  *listner;
	register wqdb_t *wqdb;
	register uint_t i;
 
	tm_start = cpu_time_stamp();
	tm_first = barrier->tm_first;
	tm_last  = barrier->tm_last;
	wqdbsz   = PMM_PAGE_SIZE / sizeof(wqdb_record_t);
	ticket   = 0;

#if ARCH_HAS_BARRIERS
	count    = barrier->count;
#else
	count    = barrier->count - 1;	/* last don't sleep */
#endif

	for(index = 0; ((index < BARRIER_WQDB_NR) && (ticket < count)); index++)
	{
		wqdb = barrier->wqdb_tbl[index];

		for(i = 0; ((i < wqdbsz) && (ticket < count)); i++)
		{

#if CONFIG_BARRIER_BORADCAST_UREAD
			event   = cpu_uncached_read(&wqdb->tbl[i].event);
			listner = (void*) cpu_uncached_read(&wqdb->tbl[i].listner);
#else
			event   = wqdb->tbl[i].event;
			listner = wqdb->tbl[i].listner;
#endif

			if(listner != NULL)
			{
				wqdb->tbl[i].listner = NULL;
#if CONFIG_USE_SCHED_LOCKS
				sched_wakeup((struct thread_s*) listner);
#else
				sched_event_send(listner, event);
#endif
				ticket ++;
			}
		}
	}

	tm_end = cpu_time_stamp();

	printk(INFO, "INFO: %s: cpu %d [F: %d, L: %d, B: %d, E: %d, T: %d]\n",
	       __FUNCTION__,
	       cpu_get_id(),
	       tm_first, 
	       tm_last, 
	       tm_start,
	       tm_end,
	       tm_end - tm_first);
}

#if ARCH_HAS_BARRIERS
static EVENT_HANDLER(barrier_broadcast_event)
{
	register struct barrier_s *barrier;

	barrier = event_get_argument(event);

#if CONFIG_BARRIER_ACTIVE_WAIT
	register uint_t next_phase;
	next_phase = ~(barrier->phase) & 0x1;
	barrier->state[next_phase] = 0; 
	barrier->state[barrier->phase] = 1;
	barrier->phase = next_phase;
	cpu_wbflush();
#else
	barrier_do_broadcast(barrier);
#endif
	return 0;
}

error_t barrier_wait(struct barrier_s *barrier)
{
	register uint_t event;
	register void *listner;
	register uint_t ticket;
	register uint_t index;
	register uint_t wqdbsz;
	register wqdb_t *wqdb;
	register struct thread_s *this;
	uint_t irq_state;
	uint_t tm_now;

	tm_now = cpu_time_stamp(); 
	this   = current_thread;
	index  = this->info.order;

	if((barrier->signature != BARRIER_ID) || ((barrier->owner != NULL) && (barrier->owner != this->task)))
		return EINVAL;

	wqdbsz  = PMM_PAGE_SIZE / sizeof(wqdb_record_t);
	wqdb    = barrier->wqdb_tbl[index / wqdbsz];

#if !(CONFIG_USE_SCHED_LOCKS)
	event   = sched_event_make (this, SCHED_OP_WAKEUP);
	listner = sched_get_listner(this, SCHED_OP_WAKEUP);
#else
	listner = (void*)this;
#endif

	wqdb->tbl[index % wqdbsz].event   = event;
	wqdb->tbl[index % wqdbsz].listner = listner;

#if CONFIG_BARRIER_ACTIVE_WAIT
	register uint_t current_phase;
	current_phase = barrier->phase;
#endif	/* CONFIG_BARRIER_ACTIVE_WAIT */

	cpu_disable_all_irq(&irq_state);

	ticket = arch_barrier_wait(barrier->cluster, barrier->hwid);

	cpu_restore_irq(irq_state);

	if(ticket < 0) return EINVAL;

	if(ticket == barrier->count)
		barrier->tm_first = tm_now;

	else if(ticket == 1)
		barrier->tm_last  = tm_now;

#if CONFIG_BARRIER_ACTIVE_WAIT
	while(cpu_uncached_read(&barrier->state[current_phase]) == 0)
		sched_yield(this);
#else
	sched_sleep(this);
#endif	/* CONFIG_BARRIER_ACTIVE_WAIT */

	return (ticket == 1) ? PTHREAD_BARRIER_SERIAL_THREAD : 0;
}

#else  /* ! ARCH_HAS_BARRIERS */

/* TODO: reintroduce barrier's ops to deal with case-specific treatment */
error_t barrier_wait(struct barrier_s *barrier)
{
	register uint_t ticket;
	register uint_t index;
	register uint_t wqdbsz;
	register wqdb_t *wqdb;
	register bool_t isShared;
	struct thread_s *this;
	uint_t tm_now;

	tm_now   = cpu_time_stamp();
	this     = current_thread;
	index    = this->info.order;
	ticket   = 0;
	isShared = (barrier->owner == NULL) ? true : false;

	if((barrier->signature != BARRIER_ID) || ((isShared == false) && (barrier->owner != this->task)))
		return EINVAL;

	wqdbsz = PMM_PAGE_SIZE / sizeof(wqdb_record_t);

	if(isShared)
	{
		spinlock_lock(&barrier->lock);
		index  = barrier->index ++;
		ticket = barrier->count - index;
	}

	wqdb   = barrier->wqdb_tbl[index / wqdbsz];

#if CONFIG_USE_SCHED_LOCKS
	wqdb->tbl[index % wqdbsz].listner = (void*)this;
#else
	uint_t irq_state;
	cpu_disable_all_irq(&irq_state); /* To prevent against any scheduler intervention */
	wqdb->tbl[index % wqdbsz].event   = sched_event_make (this, SCHED_OP_WAKEUP);
	wqdb->tbl[index % wqdbsz].listner = sched_get_listner(this, SCHED_OP_WAKEUP);
#endif

	if(isShared == false)
		ticket = atomic_add(&barrier->waiting, -1);

	if(ticket == 1)
	{
#if !(CONFIG_USE_SCHED_LOCKS)
		cpu_restore_irq(irq_state);
#endif
		barrier->tm_last = tm_now;
		wqdb->tbl[index % wqdbsz].listner = NULL;

		if(isShared)
		{
			barrier->index = 0;
			spinlock_unlock(&barrier->lock);
		}
		else
			atomic_init(&barrier->waiting, barrier->count);

		barrier_do_broadcast(barrier);
		return PTHREAD_BARRIER_SERIAL_THREAD;
	}

	if(ticket == barrier->count)
		barrier->tm_first = tm_now;

	spinlock_unlock_nosched(&barrier->lock);
	sched_sleep(this);

#if !(CONFIG_USE_SCHED_LOCKS)
	cpu_restore_irq(irq_state);
#endif
	return 0;
}

#endif	/* ARCH_HAS_BARRIERS */

KMEM_OBJATTR_INIT(wqdb_kmem_init)
{
	attr->type = KMEM_BARRIER;
	attr->name = "WaitQueue DB";
	attr->size = sizeof(struct barrier_s);
	attr->aligne = 0;
	attr->min  = CONFIG_WAITQUEUEDB_MIN;
	attr->max  = CONFIG_WAITQUEUEDB_MAX;
	attr->ctor = NULL;
	attr->dtor = NULL;
	return 0;
}

error_t barrier_init(struct barrier_s *barrier, uint_t count, uint_t scope)
{
	struct page_s *page;
	uint_t wqdbsz;
	uint_t i;
	error_t err;
	kmem_req_t req;

	wqdbsz = PMM_PAGE_SIZE / sizeof(wqdb_record_t);
  
	if(count == 0) 
		return EINVAL;

	if(current_task->threads_limit > (BARRIER_WQDB_NR*wqdbsz))
	{
		printk(INFO, "INFO: %s: pid %d, cpu %d, task threads limit exceed barrier ressources of %d\n",
		       __FUNCTION__,
		       current_task->pid,
		       cpu_get_id(),
		       BARRIER_WQDB_NR*wqdbsz);

		return ENOMEM;
	}
    
	if(count > BARRIER_WQDB_NR*wqdbsz) 
		return ENOMEM;

	barrier->owner = (scope == BARRIER_INIT_PRIVATE) ? current_task : NULL;

#if ARCH_HAS_BARRIERS
	barrier->cluster = current_cluster;
	event_set_handler(&barrier->event, &barrier_broadcast_event);
	event_set_argument(&barrier->event, barrier);
	barrier->hwid = arch_barrier_init(barrier->cluster, &barrier->event, count);
    
	if(barrier->hwid < 0)
		return ENOMEM;		/* TODO: we can use software barrier instead */
#else
	if(barrier->owner != NULL)
		atomic_init(&barrier->waiting, count);
	else
	{
		spinlock_init(&barrier->lock, "barrier");
		barrier->index = 0;
	}

#endif	/* ARCH_HAS_BARRIERS */

	req.type  = KMEM_PAGE;
	req.size  = 0;
	req.flags = AF_USER | AF_ZERO;
	err       = 0;

	for(i = 0; i < BARRIER_WQDB_NR; i++)
	{
		page = kmem_alloc(&req);
    
		if(page == NULL)
		{ 
			err = ENOMEM;
			break;
		}
    
		barrier->wqdb_tbl[i]  = ppm_page2addr(page);
		barrier->pages_tbl[i] = page;
	}

	if(err)
	{
		err = i;
    
		for(i = 0; i < err; i++)
		{
			req.ptr = barrier->pages_tbl[i];
			kmem_free(&req);
		}
    
		return ENOMEM;
	}

	barrier->count     = count;
	barrier->signature = BARRIER_ID;
	barrier->state[0]  = 0;
	barrier->state[1]  = 0;
	barrier->phase     = 0;
	barrier->name      = "Barrier-Sync";
  
	return 0;
}

error_t barrier_destroy(struct barrier_s *barrier)
{
	register uint_t cntr;
	kmem_req_t req;
  
	if(barrier->signature != BARRIER_ID)
		return EINVAL;

	if((barrier->owner != NULL) && (barrier->owner != current_task))
		return EINVAL;

	req.type = KMEM_PAGE;

#if ARCH_HAS_BARRIERS
	(void) arch_barrier_destroy(barrier->cluster, barrier->hwid);
#else
	if(barrier->owner == NULL)
		cntr = barrier->index;
	else
		cntr = atomic_get(&barrier->waiting);

	if(cntr != 0) return EBUSY;
#endif	/* ARCH_HAS_BARRIERS */

	barrier->signature = 0;
	cpu_wbflush();
    
	for(cntr = 0; cntr < BARRIER_WQDB_NR; cntr++)
	{
		req.ptr = barrier->pages_tbl[cntr];
		kmem_free(&req);
	}

	if(barrier->owner == NULL)
		spinlock_destroy(&barrier->lock);

	return 0;
}
