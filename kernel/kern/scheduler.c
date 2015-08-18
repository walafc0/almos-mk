/*
 * kern/scheduler.c - Per-CPU generic scheduler
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
#include <list.h>
#include <cpu.h>
#include <time.h>
#include <thread.h>
#include <task.h>
#include <kdmsg.h>
#include <cpu-trace.h>
#include <cluster.h>
#include <event.h>
#include <rr-sched.h>
#include <signal.h>
#include <bits.h>
#include <scheduler.h>

#if CONFIG_USE_SCHED_LOCKS
#include <spinlock.h>
#endif

#if CONFIG_SCHED_DEBUG
#define SCHED_SCOPE
#else
#define SCHED_SCOPE static
#endif

#define SCHED_THREADS_NR    CONFIG_SCHED_THREADS_NR

#if CONFIG_SCHED_SHOW_NOTIFICATIONS
#define sched_notify_dmsg(_thread,_index,_event)			\
	do{								\
		isr_dmsg(INFO, "INFO: [%p] %s: pid %d, tid %d, cpu %d, index %d, "_event" [%u]\n", \
			 (_thread),					\
			 __FUNCTION__,					\
			 (_thread)->task->pid,				\
			 (_thread)->info.order,				\
			 cpu_get_id(),					\
			 (_index),					\
			 cpu_time_stamp());				\
	}while(0)
#else
#define sched_notify_dmsg(_thread,_index,_event)
#endif

/* Sched Ports Info */
typedef struct
{
	uint_t ports[SCHED_PORTS_NR];
}sched_pinfo_t;

/* Sched DataBase */
struct sched_db_s
{
#if CONFIG_USE_SCHED_LOCKS
	spinlock_t lock;
#endif

	BITMAP_DECLARE(bitmap,(SCHED_THREADS_NR >> 3));
	cacheline_t next;
	sched_pinfo_t tbl[SCHED_THREADS_NR];
};

void __sched_wakeup(struct thread_s *thread);

error_t sched_init(struct scheduler_s *scheduler)
{
	kmem_req_t req;
	struct sched_db_s *db;
	error_t err;
  
	req.type  = KMEM_GENERIC;
	req.size  = sizeof(struct sched_db_s);
	req.flags = AF_BOOT | AF_ZERO;
  
	db = kmem_alloc(&req);

	if(db == NULL) return ENOMEM;

#if CONFIG_USE_SCHED_LOCKS
	spinlock_init(&db->lock, "Sched-db");
#endif
	list_root_init(&scheduler->sleep_check);

	bitmap_set_range(db->bitmap, 0, SCHED_THREADS_NR);

	err = rr_sched_init(scheduler, &scheduler->scheds_tbl[SCHED_RR]);

	if(err != 0)
	{
		req.ptr = db;
		kmem_free(&req);
		return err;
	}
  
	scheduler->db = db;
	return 0;
}

error_t sched_register(struct thread_s *thread)
{
	struct cpu_s *cpu;
	struct sched_db_s *db;
	sint_t index;

	index = -1;
	cpu   = thread_current_cpu(thread);
	db    = cpu->scheduler.db;

#if CONFIG_USE_SCHED_LOCKS
	spinlock_lock(&db->lock);
#else
	uint_t irq_state;
	cpu_disable_all_irq(&irq_state);
#endif

	if(db->next.value >= SCHED_THREADS_NR)
		db->next.value = 0;
  
	index = bitmap_ffs2(db->bitmap, db->next.value, sizeof(db->bitmap));
  
	if(index != -1)
	{
		bitmap_clear(db->bitmap, index);
		db->next.value = index + 1;
		thread->ltid = index;
	}

#if CONFIG_USE_SCHED_LOCKS
	spinlock_unlock(&db->lock);
#else
	cpu_restore_irq(irq_state);
#endif

	return (index == -1) ? ERANGE : 0;
}

SCHED_SCOPE void sched_unregister(struct thread_s *thread)
{
	struct cpu_s *cpu;
	struct sched_db_s *db;
  
	cpu = thread_current_cpu(thread);
	db  = cpu->scheduler.db;

#if CONFIG_USE_SCHED_LOCKS
	spinlock_lock(&db->lock);
#else
	uint_t irq_state;
	cpu_disable_all_irq(&irq_state);
#endif
  
	memset(&db->tbl[thread->ltid],0,sizeof(sched_pinfo_t));
	bitmap_set(db->bitmap, thread->ltid);
  
	if(thread->ltid < db->next.value)
		db->next.value = thread->ltid;

#if CONFIG_USE_SCHED_LOCKS
	spinlock_unlock(&db->lock);
#else
	cpu_restore_irq(irq_state);
#endif
}

#if CONFIG_USE_SCHED_LOCKS
#define sched_event_notify(x)
#else
/* TODO: don't visit all threads, just sched->count one */
SCHED_SCOPE void sched_event_notify(struct scheduler_s *scheduler)
{
	register uint_t i;
	register uint_t event;
	register struct thread_s *thread;
	register struct sched_db_s *db;
  
	db = scheduler->db;

	for(i = 0; i < SCHED_THREADS_NR; i++)
	{
		event = db->tbl[i].ports[SCHED_PORT(SCHED_OP_WAKEUP)];
 
		if(event & SCHED_OP_WAKEUP)
		{
			thread = (struct thread_s*)(event & ~SCHED_OP_MASK);
			db->tbl[i].ports[SCHED_PORT(SCHED_OP_WAKEUP)] = 0;
			__sched_wakeup(thread);
			
			sched_notify_dmsg(thread,i,"WAKEUP");
			continue;
		}

		event = db->tbl[i].ports[SCHED_PORT(SCHED_OP_ADD_CREATED)];

		if(event & SCHED_OP_ADD_CREATED)
		{
			thread = (struct thread_s*)(event & ~SCHED_OP_MASK);
			thread->state = S_CREATE;
			db->tbl[i].ports[SCHED_PORT(SCHED_OP_ADD_CREATED)] = 0;
			thread->local_sched->op.add_created(thread);
			
			sched_notify_dmsg(thread,i,"ADD_CREATED");
			continue;
		}

		event = db->tbl[i].ports[SCHED_PORT(SCHED_OP_UWAKEUP)];
		thread = (struct thread_s*)(event & ~SCHED_OP_MASK);

		if((event & SCHED_OP_UWAKEUP) && thread_isWakeable(thread))
		{
			thread_clear_wakeable(thread);
			db->tbl[i].ports[SCHED_PORT(SCHED_OP_UWAKEUP)] = 0;
			__sched_wakeup(thread);

			sched_notify_dmsg(thread,i,"UWAKEUP");
			continue;
		}
	}
}
#endif	/* CONFIG_USE_SCHED_LOCKS */

error_t sched_setpolicy(struct thread_s *thread, uint_t policy)
{
	register struct scheduler_s *scheduler;
  
	scheduler = &thread_current_cpu(thread)->scheduler;

	switch(policy)
	{
	case SCHED_RR:
		thread->local_sched = &scheduler->scheds_tbl[SCHED_RR];
		return 0;
	default:
		printk(ERROR, "ERR: sched_setpolicy: unexpected policy [Tid %x, policy %d]\n", thread, policy);
		return -1;
	}
}

uint_t sched_getpolicy(struct thread_s *thread)
{
	register struct scheduler_s *scheduler;
  
	scheduler = &thread_current_cpu(thread)->scheduler;
  
	if(thread->local_sched == &scheduler->scheds_tbl[SCHED_RR])
		return SCHED_RR;
  
	printk(ERROR, "ERR: sched_getpolicy: unexpected policy [Tid %x]\n", thread);
	return SCHED_RR;
}


void sched_setprio(struct thread_s *thread, uint_t prio)
{
	thread->static_prio = prio;
}

void sched_clock(struct thread_s *this, uint_t ticks_nr)
{ 
	register struct cpu_s *cpu;
	register struct scheduler_s *scheduler;
	register uint_t i;

	cpu       = thread_current_cpu(this);
	scheduler = &cpu->scheduler;

	this->ticks_nr ++;

	if(this->type == KTHREAD)
		cpu_get_thread_idle(cpu)->ticks_nr ++;

	cpu_wbflush();

	sched_event_notify(&thread_current_cpu(this)->scheduler);

	for(i = 0; i < SCHEDS_NR; i++)
		scheduler->scheds_tbl[i].op.clock(this, ticks_nr);
}

void sched_strategy(struct scheduler_s *scheduler)
{
	register uint_t i;

	for(i = 0; i < SCHEDS_NR; i++)
		scheduler->scheds_tbl[i].op.strategy(&scheduler->scheds_tbl[i]);
}

void sched_idle(struct thread_s *this)
{
	sched_event_notify(&(thread_current_cpu(this)->scheduler));
}


void check_sleeping(struct scheduler_s * scheduler)
{
	register struct thread_s *thread;
	register struct list_entry *iter;

	list_foreach(&scheduler->sleep_check, iter)
	{
		thread = list_element(iter, struct thread_s, list);
		if(thread->state == S_READY)
		{
#if CONFIG_SCHED_DEBUG
			isr_dmsg(INFO, "%s: [%d] wakening thread %x\n", 
			__FUNCTION__,
			cpu_get_id(),
			thread);
#endif
			list_unlink(&thread->list);
			thread->state = S_WAIT;
			sched_wakeup(thread);
			break;
		}
	}
}


/* to be called with interrupts disabled */
SCHED_SCOPE void schedule(struct thread_s *this)
{
	register struct thread_s *elected;
	register struct scheduler_s *scheduler;
	register struct sched_s *sched;
	register struct cpu_s *cpu;
	register uint_t ret, i;
	uint_t state;
   
	elected   = NULL;
	cpu       = current_cpu;
	scheduler = &cpu->scheduler;

	sched_event_notify(scheduler);

	check_sleeping(scheduler);

	for(i=0; (i < SCHEDS_NR) && (elected == NULL); i++)
	{
		sched   = &scheduler->scheds_tbl[i];
		elected = sched->op.elect(sched);
	}

	if(elected == NULL)
	{
		cpu_set_state(cpu, CPU_IDLE);
		elected = cpu_get_thread_idle(cpu);
	}
	else
		cpu_set_state(cpu, CPU_ACTIVE);

	if(thread_isStack_overflow(elected))
		PANIC ("The kernel stack of thread %x on CPU %d has overflowed !!", elected, cpu->gid);
		
#if CONFIG_SCHED_DEBUG
	isr_dmsg(INFO, "%s: cpu %d, current tid %d (%x), pid %d, next tid %d (%x), pid %d\n", 
		 __FUNCTION__,
		 cpu_get_id(),
		 this->info.order,
		 this,
		 this->task->pid,
		 elected->info.order,
		 elected,
		 elected->task->pid);
#endif

	if(this->state == S_DEAD)
		event_send(this->info.e_info, cpu->gid);
    
	if(elected != this)
	{
		if((ret = cpu_context_save(&this->pws)) == 0)
		{  
			if(elected->state == S_CREATE)
			{
				tm_born_compute(elected);
				elected->state = (elected->type == PTHREAD) ? S_USR : S_KERNEL;
			}
			else
			{
				tm_sleep_compute(elected);
				elected->info.sched_nr ++;
				elected->state = S_KERNEL;
			}

			cpu_context_restore(&elected->pws, 1);
			PANIC ("Thread %x on CPU %d, must never to return here !!", elected, cpu->gid);
		}
	}
	else
		this->state = S_KERNEL;

	if(this->type != PTHREAD)
		return;

	if(this == cpu->fpu_owner)
		cpu_fpu_enable();
	else
		cpu_fpu_disable();
	
	if(thread_isCapMigrate(this) && thread_migration_isActivated(this))
	{
		thread_clear_cap_migrate(this);
		cpu_enable_all_irq(&state);
		ret = thread_migrate(this,-1);
		cpu_restore_irq(state);
		
		if(ret == 0)
		{
			this = current_thread; 
			thread_migration_deactivate(this);
		}
		else
		{
			isr_dmsg(INFO, 
				 "%s: cpu %d, migration failed for victim pid %d, "
				 "tid %d, err %d\n", 
				 __FUNCTION__, 
				 cpu_get_id(),
				 this->task->pid,
				 this->info.order,
				 ret);
		}

		thread_set_cap_migrate(this);
	}

#if 0
	if(event_is_pending(&cpu->re_listner))
		event_listner_notify(&cpu->re_listner);
   
	if(event_is_pending(&cpu->le_listner))
		event_listner_notify(&cpu->le_listner);
#endif
}

int sched_yield(struct thread_s *this)
{
	uint_t irq_state;
  
	cpu_trace_write(current_cpu, sched_yield);

	tm_sys_compute(this);

	cpu_disable_all_irq(&irq_state);

	this->local_sched->op.yield(this);
	schedule(this);

	cpu_restore_irq(irq_state);

	return 0;
}

void sched_exit(struct thread_s *this)
{
	uint_t irq_state;
	struct event_s e_info;

	cpu_trace_write(current_cpu, sched_exit);

	tm_exit_compute(this);
  
	sched_unregister(this);

	event_set_priority(&e_info, E_FUNC);
	event_set_handler(&e_info, &thread_destroy_handler);
	event_set_argument(&e_info, this);
	event_set_senderId(&e_info, current_cpu);
	this->info.e_info = &e_info;
 
	cpu_disable_all_irq(&irq_state);
  
	sched_notify_dmsg(this,this->ltid,"EXIT");

	this->local_sched->op.exit(this);

	this->state = S_DEAD;
	schedule(this);
}

void sched_add_created(struct thread_s *thread)
{
	cpu_trace_write(current_cpu, sched_add_created);

	tm_create_compute(thread);
	thread->state = S_CREATE;

#if CONFIG_USE_SCHED_LOCKS
	uint_t irq_state;

	thread->local_sched->op.add(thread);
	cpu_disable_all_irq(&irq_state);
	sched_notify_dmsg(thread, thread->ltid, "ADD_CREATED");
	cpu_restore_irq(irq_state);
#else
	uint_t event;
	void* listner;

	listner = sched_get_listner(thread, SCHED_OP_ADD_CREATED);
	event = sched_event_make(thread, SCHED_OP_ADD_CREATED);
	sched_event_send(listner,event);

	cpu_wbflush();
#endif
}

/* Used in kern/thread_migrate.c */
void sched_add(struct thread_s *thread)
{
	uint_t irq_state;

	thread->state = S_READY;
	tm_sys_compute(thread);
	cpu_disable_all_irq(&irq_state);
	sched_notify_dmsg(thread,thread->ltid,"ADD");
	thread->local_sched->op.add(thread);
	cpu_restore_irq(irq_state);
}

/* Used in kern/thread_migrate.c */
void sched_remove(struct thread_s *this)
{
	uint_t irq_state;
	struct event_s e_info;

	sched_unregister(this);

	event_set_priority(&e_info, E_FUNC);
	event_set_handler(&e_info, &thread_destroy_handler);
	event_set_argument(&e_info, this);
	event_set_senderId(&e_info, current_cpu);
	this->info.e_info = &e_info;

	cpu_disable_all_irq(&irq_state);

	sched_notify_dmsg(this,this->ltid,"REMOVE");

	this->local_sched->op.remove(this);
	this->type  = KTHREAD;
	this->state = S_DEAD;
	schedule(this);
}

void __sched_sleep(struct thread_s *this, bool_t add_to_sleep_check)
{
	uint_t irq_state;

	cpu_trace_write(current_cpu, sched_sleep);

	tm_sys_compute(this);

	cpu_disable_all_irq(&irq_state);

	if(thread_isCapWakeup(this))
		thread_set_wakeable(this);

	sched_notify_dmsg(this,this->ltid,"GOING TO SLEEP");

	if(this->info.before_sleep)
		this->info.before_sleep(this);

	this->local_sched->op.sleep(this);
	
	if(add_to_sleep_check)
	{
		list_add_last(	&current_cpu->scheduler.sleep_check, 
				&this->list);
	}
		
	schedule(this);

	thread_clear_wakeable(this);

	sched_notify_dmsg(this,this->ltid,"DONE");

	cpu_restore_irq(irq_state);
}

void sched_sleep(struct thread_s *this)
{
	__sched_sleep(this, false);
}


void sched_sleep_check(struct thread_s *this)
{
	__sched_sleep(this, true);
}

void __sched_wakeup(struct thread_s *thread)
{
	uint_t irq_state;

	cpu_disable_all_irq(&irq_state);

	thread->local_sched->op.wakeup(thread);

	if(thread->info.after_wakeup)
		thread->info.after_wakeup(thread);

	sched_notify_dmsg(thread, thread->ltid, "WAKEUP");

	cpu_restore_irq(irq_state);
}

void sched_wakeup (struct thread_s *thread)
{ 
	cpu_trace_write(current_cpu, sched_wakeup);

#if CONFIG_USE_SCHED_LOCKS
	__sched_wakeup(thread);
#else
	uint_t event;
	void* listner;

	event = sched_event_make(thread, SCHED_OP_WAKEUP);
	listner = sched_get_listner(thread, SCHED_OP_WAKEUP);
	sched_event_send(listner,event);
	cpu_wbflush();
#endif
}

void* sched_get_listner(struct thread_s *thread, uint_t sched_op)
{
	struct sched_db_s *db;
	struct cpu_s *cpu;
	sched_pinfo_t *pinfo;

	cpu = thread_current_cpu(thread);
	db = cpu->scheduler.db;
	pinfo = &db->tbl[thread->ltid]; 
	return &pinfo->ports[SCHED_PORT(sched_op)];
}
