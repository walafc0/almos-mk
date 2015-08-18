/*
 * kern/event.c - Per-CPU Events-Manager
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
#include <cpu.h>
#include <cluster.h>
#include <device.h>
#include <pmm.h>
#include <event.h>
#include <cpu-trace.h>
#include <thread.h>

/** Initialize event */
error_t event_init(struct event_s *event)
{
	/* nothing to do in this version */
	return 0;
}

/** Destroy event */
void event_destroy(struct event_s *event)
{
	/* nothing to do in this version */
}

/** Destroy event listner */
void event_listner_destroy(struct event_listner_s *el)
{
	/* nothing to do in this version */
}

error_t event_listner_init(struct event_listner_s *el)
{
	error_t err;
	register uint_t i;
  
	el->flags = 0;
	el->prio = 0;
	err = 0;

	for(i=0; i < E_PRIO_NR; i++)
	{
		list_root_init(&el->tbl[i].root);
		el->tbl[i].count = 0;
	}


	return 0;
}


static void local_event_send(struct event_s *event, struct event_listner_s *el, bool_t isFIFO) 
{
	uint_t prio;

	prio = event_get_priority(event);

	if(isFIFO)
		list_add_last(&el->tbl[prio].root, &event->e_list);
	else
		list_add_first(&el->tbl[prio].root, &event->e_list);

	el->tbl[prio].count ++;
	el->prio = (!(el->flags & EVENT_PENDING) || (el->prio > prio)) ? prio : el->prio;
	el->flags |= EVENT_PENDING;

#if CONFIG_SHOW_LOCAL_EVENTS
	isr_dmsg(INFO, "%s: cpu %d, prio %d, el->prio %d [%d]\n", 
		 __FUNCTION__, 
		 cpu_get_id(), 
		 prio, 
		 el->prio,
		 cpu_time_stamp());
#endif
}

#if 0
static EVENT_HANDLER(re_send_backoff_handler)
{
	uint_t cpu_gid;
  
	cpu_gid = (uint_t) event_get_argument(event);
  
	event_restore(event);
  
	event_send(event, cpu_gid);

	printk(INFO, "INFO: %s, cpu %d, event %x sent to cpu_gid %d [%d]\n",
	       __FUNCTION__,
	       cpu_get_id(),
	       event,
	       cpu_gid,
	       cpu_time_stamp());
  
	return 0;
}

static error_t __attribute__((noinline)) re_send_backoff(struct event_s *event, 
							 uint_t cpu_gid, 
							 uint_t tm_stamp)
{
	struct cpu_s *cpu;
	uint_t tm_now;
	uint_t irq_state;

	tm_now = cpu_time_stamp();
	cpu    = current_cpu;

	if((tm_now - tm_stamp) < cpu_get_ticks_period(cpu))
		return EAGAIN;

	event_backup(event);
  
	event_set_priority(event, E_CHR);
	event_set_handler(event, re_send_backoff_handler);
	event_set_argument(event, (void*)cpu_gid);

	cpu_disable_all_irq(&irq_state);
	local_event_send(event, &cpu->le_listner, false);
	cpu_restore_irq(irq_state);

	printk(INFO, "INFO: %s, cpu %d, event %x sent to cpu_gid %d [%d]\n",
	       __FUNCTION__,
	       cpu->gid,
	       event,
	       cpu_gid,
	       tm_now);

	return 0;
}

/** This is the only function that handle remote data **/
static void remote_event_send(struct event_s *event, 
				struct event_listner_s *el, 
				uint_t cid, uint_t cpu_gid)
{ 
	error_t retry;
	uint_t tm_stamp;
	uint_t prio;

	tm_stamp = cpu_time_stamp();
	prio     = event_get_priority(event);

	while((retry = event_fifo_put(&el->tbl[prio].fifo, cid, event)) != 0)
	{
		retry = re_send_backoff(event, cpu_gid, tm_stamp);
    
		if(retry == 0)
			break;
	}


#if 1
	bool_t isPending;
	uint_t flags = remote_lw((void*)&el->flags, cid);
	isPending = (flags & EVENT_PENDING) ? true : false;

	remote_sw((void*)&el->flags, cid, flags | EVENT_PENDING);
	cpu_wbflush();
	pmm_cache_flush_raddr((vma_t) &el->flags, cid, PMM_DATA);

	if((prio < E_FUNC) && (isPending == false))
	{
		(void)arch_cpu_send_ipi(cpu_gid);
	}
#else

	if((prio < E_FUNC))
	{
		(void)arch_cpu_send_ipi(cpu_gid);
	}
#endif
}
#endif

/* must be called with all irq disabled */
void event_send(struct event_s *event, uint_t cpu_gid)
{
	uint_t cid = arch_cpu_cid(cpu_gid);
	uint_t lid = arch_cpu_lid(cpu_gid);

	if (cid != current_cid)
	{
		PANIC("event not local cid [cpu %d, cid %d, cycle %u]\n", 
		      cpu_get_id(), 
		      cid,
		      cpu_time_stamp());
	}
	
	local_event_send(event, &cpu_lid2ptr(lid)->le_listner, true);
}


static void local_event_listner_notify(struct event_listner_s *el)
{
	register struct event_s *event;
	register uint_t count;
	register uint_t start_prio;
	register uint_t current_prio;

#if CONFIG_SHOW_LOCAL_EVENTS
	register uint_t cntr;
#endif
  
	count = 0;
	assert((el->flags & EVENT_PENDING) && "event_notify is called but no event is pending");

	el->flags   &= ~EVENT_PENDING;
	start_prio   = el->prio ++;
	current_prio = start_prio;

	while(current_prio < E_PRIO_NR)
	{
		if(el->prio <= start_prio)
		{
			start_prio   = el->prio ++;
			current_prio = start_prio;
		}

		while(el->tbl[current_prio].count != 0)
		{
			assert(!(list_empty(&el->tbl[current_prio].root)) && "pending event queue is empty");

			event = list_first(&el->tbl[current_prio].root, struct event_s, e_list);
			list_unlink(&event->e_list);
      
			el->tbl[current_prio].count --;
      
			cpu_enable_all_irq(NULL);
			event_get_handler(event)(event);	 /* el->prio can be changed */
			count ++;

#if CONFIG_SHOW_LOCAL_EVENTS
			cntr ++;
#endif

			cpu_disable_all_irq(NULL);
		}

#if CONFIG_SHOW_LOCAL_EVENTS    
		cpu_enable_all_irq(NULL);
		if(cntr)
		{
			printk(INFO, "INFO: cpu %d, %d pending events of priority %d have been Delivered [ %u ]\n",
			       cpu_get_id(),
			       cntr, 
			       current_prio, 
			       cpu_time_stamp());
		}
		cntr = 0;
		cpu_disable_all_irq(NULL);
#endif
    
		current_prio ++;
	}

	el->count += count;
}


#if 0
static void remote_event_listner_notify(struct event_listner_s *el)
{
	struct event_listner_s *local_el;
	struct event_s *event;
	event_prio_t current_prio;
	uint_t irq_state;
	uint_t count;
	error_t err;
	uint_t cid;
  
	local_el = &current_cpu->le_listner;
	cid = current_cid;
	count    = 0;

	do
	{
		el->flags &= ~EVENT_PENDING;
		cpu_wbflush();
		pmm_cache_flush_vaddr((vma_t)&el->flags, PMM_DATA);

		cpu_disable_all_irq(&irq_state);

		for(current_prio = E_CLK; current_prio < E_PRIO_NR; current_prio++, count++)
		{
			while((err = event_fifo_get(&el->tbl[current_prio].fifo, cid, (void**)&event)) == 0)
				local_event_send(event, local_el, false);//event is local
		}

		cpu_restore_irq(irq_state);
		el->count += count;

	}while(el->flags & EVENT_PENDING);
}
#endif

/* must be called with all irq disabled */
void event_listner_notify(struct event_listner_s *el)
{
	local_event_listner_notify(el);
}


void* thread_event_manager(void *arg)
{
	struct thread_s *this;
	struct cpu_s *cpu;
	uint_t irq_state;

	cpu_enable_all_irq(NULL);
  
	this = current_thread;
	cpu  = current_cpu;

	thread_preempt_disable(this);

	while(1)
	{
#if 0
		printk(INFO, "INFO: Event Handler on CPU %d, event is le pending %d [%d,%d]\n",
		       cpu->gid,
		       event_is_pending(&cpu->le_listner),
		       cpu_time_stamp(),
		       cpu_get_ticks(cpu)); 
#endif
   
		cpu_disable_all_irq(&irq_state);

		if(event_is_pending(&cpu->le_listner))
			event_listner_notify(&cpu->le_listner);
    
		wait_on(&this->info.wait_queue, WAIT_ANY);

		sched_sleep(this);

		cpu_restore_irq(irq_state);
	}

	return NULL;
}
