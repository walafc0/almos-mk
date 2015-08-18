/*
 * kern/rr-sched.c - Round-Robin scheduling policy
 * 
 * Copyright (c) 2007,2008,2009,2010,2011,2012 Ghassan Almaless
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

#include <config.h>

#if !(CONFIG_USE_SCHED_LOCKS)

#include <types.h>
#include <list.h>
#include <kdmsg.h>
#include <spinlock.h>
#include <cpu.h>
#include <cluster.h>
#include <dqdt.h>
#include <thread.h>
#include <task.h>
#include <kmem.h>

#include <scheduler.h>


#if CONFIG_SCHED_RR_CHECK
#define sched_assert(cond) assert((cond))
#define SCHED_SCOPE
#else
#define sched_assert(cond)
#define SCHED_SCOPE static
#endif

#undef SCHED_SCOPE
#define SCHED_SCOPE

#define RR_QUANTUM          4	/* in TICs number */
#define SCHED_THREADS_NR    CONFIG_SCHED_THREADS_NR

#if CONFIG_SCHED_DEBUG
#define rr_dmsg(...) \
	isr_dmsg(__VA_ARGS__)
#else
#define rr_dmsg(...)
#endif

#define rr_cpu_dmsg(c,...)				\
	do{						\
		if(cpu_get_id() == (c))			\
			isr_dmsg(INFO, __VA_ARGS__);	\
	}while(0)

typedef struct rQueues_s
{
	struct cpu_s *cpu;
	uint_t clock_cntr;
	uint_t period;
	uint_t u_runnable;
	uint_t m_runnable;
	struct scheduler_s *scheduler;
	struct list_entry kthreads;
	struct list_entry runnable;
	struct list_entry migrate;

#if CONFIG_SCHED_DEBUG
	uint8_t cond1;
	uint8_t cond2;
	uint8_t cond3;
	uint8_t cond4;
#endif

} rQueues_t;

void __attribute__ ((noinline)) rr_sched_load_balance(struct thread_s *this,
						      struct sched_s *sched,
						      rQueues_t *rQueues,
						      bool_t isUrgent)
{
	struct thread_s *victim;
	struct thread_s *thread;
	struct list_entry *iter;
	struct cpu_s *cpu;
	register sint_t max;

	cpu         = current_cpu;
	victim      = this;

	if(!(thread_isExported(this)) && (thread_migration_isEnabled(this)))
		max = this->boosted_prio;
	else
		max = -1;

	this->boosted_prio >>= 1;
	thread_migration_deactivate(this);

	list_foreach(&rQueues->runnable, iter)
	{
 		thread = list_element(iter, struct thread_s, list);

		rr_dmsg(INFO,
			"%s: cpu %d, usage %d, u_runnable [%d,%d], "
			"thread pid %d, tid %d, state %s, flags %x, bprio %d, ticks %d, max %d, isUrg %d\n",
			__FUNCTION__, 
			cpu_get_id(),
			cpu->usage,
			rQueues->u_runnable,
			rQueues->m_runnable,
			thread->task->pid,
			thread->info.order,
			thread_state_name[thread->state],
			thread->flags,
			thread->boosted_prio,
			thread->ticks_nr,
			max,
			isUrgent);

		if((thread_isExported(thread)) || !(thread_migration_isEnabled(thread)))
			continue;

		if(thread->boosted_prio > max)
		{
			victim = thread;
			max    = victim->boosted_prio;
		}

		thread->boosted_prio >>= 1;
		thread_migration_deactivate(thread);
	}

	if((max >= 0) && ((this->type == PTHREAD) || (rQueues->u_runnable > 0)))
	{
		thread_migration_activate(victim);

		if(victim != this)
		{
			list_unlink(&victim->list);
			list_add_last(&rQueues->migrate, &victim->list);
			rQueues->m_runnable ++;
			rQueues->u_runnable --;
		}

		rr_dmsg(INFO, "%s: cpu %d, u_runnable [%d,%d], victim pid %d, tid %d , state %s, flags %x, [%u]\n",
			__FUNCTION__, 
			cpu_get_id(),
			rQueues->u_runnable,
			rQueues->m_runnable,
			victim->task->pid,
			victim->info.order,
			thread_state_name[victim->state],
			victim->flags,
			cpu_time_stamp());
	}
}

//FIXME(40): manipulate parent dqdt!
SCHED_SCOPE void rr_clock_balancing(struct thread_s *this, uint_t ticks_nr)
{
	register struct sched_s *sched;
	register rQueues_t *rQueues;
	struct cpu_s *cpu;
	struct cluster_s *cluster;
	struct dqdt_cluster_s *logical;
	register bool_t cond1, cond2, cond3;

	if(this->quantum > 0)
		this->quantum --;

	if((this->quantum == 0) && (this->type != TH_IDLE))
		thread_sched_activate(this);

	sched   = this->local_sched;
	rQueues = (rQueues_t*) sched->data;
	cpu     = rQueues->cpu;
	cluster = cpu->cluster;
	logical = cluster->levels_tbl[0];
	cond1   = ((cpu->usage >= 150) && (rQueues->u_runnable > 0) && (rQueues->scheduler->user_nr > 1));
	logical = logical->parent;
	//FIXME(40): parent is null!
	cond2   = ((logical->info.summary.U <= 90) || (logical->parent->info.summary.U <= 95));
	cond3   = cond1 && cond2;

	rQueues->clock_cntr += ((cond3) ? (rQueues->period >> 1) : 1);

	if(rQueues->clock_cntr >= rQueues->period)
	{
		rQueues->clock_cntr = 0;

		//FIXME(40): deqdt_root is null!
		if(cond1 && (dqdt_root->info.summary.U < 100))
		{
			pmm_cache_flush_vaddr((vma_t)&dqdt_root->info.summary.U, PMM_DATA);
			rr_sched_load_balance(this, sched, rQueues, cond3);
		}
	}

	if((rQueues->m_runnable > 0) && !thread_isExported(this))
	{
		thread_sched_activate(this);
		this->quantum = 0;
	}

#if CONFIG_SCHED_DEBUG
	rQueues->cond1 = cond1;
	rQueues->cond2 = cond2;
	rQueues->cond3 = cond3;
#endif	

}

SCHED_SCOPE void rr_clock(struct thread_s *this, uint_t ticks_nr)
{
	if(this->quantum > 0)
		this->quantum -= 1;

	if((this->quantum == 0) && (this->type != TH_IDLE))
		thread_sched_activate(this);
}

SCHED_SCOPE void rr_sched_strategy(struct sched_s *sched)
{
}

SCHED_SCOPE void rr_yield(struct thread_s *this)
{
	sched_assert(this->state == S_KERNEL && "Unexpected yield op");
}

SCHED_SCOPE void rr_remove(struct thread_s *this)
{
	sched_assert(this->type == PTHREAD && "Unexpected remove op");
  
	thread_current_cpu(this)->scheduler.total_nr --;
	thread_current_cpu(this)->scheduler.user_nr --;
	cpu_wbflush();

	sched_assert(this->state == S_KERNEL && "Unexpected remove op");
}


SCHED_SCOPE void rr_exit(struct thread_s *this)
{
	thread_current_cpu(this)->scheduler.total_nr --;

	if(this->type == PTHREAD)
		thread_current_cpu(this)->scheduler.user_nr --;

	sched_assert(this->state == S_KERNEL && "Unexpected exit op");
	this->state = S_DEAD;
}

SCHED_SCOPE void rr_sleep(struct thread_s *this)
{
	sched_assert((this->state == S_KERNEL) && "Unexpected sleep op");
	this->state = S_WAIT;	
}

SCHED_SCOPE void rr_wakeup (struct thread_s *thread)
{
	register struct sched_s *sched;
	register struct thread_s *this;
	register rQueues_t *rQueues;
	register thread_attr_t type;

	if(thread->state == S_READY)
		return;
	
	sched_assert((thread->state == S_WAIT) && "Unexpected sleep wakeup op");

	thread->state = S_READY;

	sched   = thread->local_sched;
	type    = thread->type;
	rQueues = (rQueues_t*) sched->data;

	sched->count ++;

	if(type == KTHREAD)
	{
		rQueues->scheduler->k_runnable ++;
		//list_add_last(&rQueues->kthreads, &thread->list);
		list_add_first(&rQueues->kthreads, &thread->list);
		this = current_thread;
		thread_sched_activate(this);
		thread_set_forced_yield(this);
	}
	else
	{
		rQueues->u_runnable ++;
		rQueues->scheduler->u_runnable ++;
		//list_add_last(&rQueues->runnable, &thread->list);
		list_add_first(&rQueues->runnable, &thread->list);
	}
}

SCHED_SCOPE void rr_add_created(struct thread_s *thread)
{
	register struct sched_s *sched;
	register rQueues_t *rQueues;
	register thread_attr_t type;

	sched   = thread->local_sched;
	rQueues = (rQueues_t*) sched->data;
	type    = thread->type;

	sched->count ++;
	rQueues->scheduler->total_nr ++;

	if(type == KTHREAD)
	{
		rQueues->scheduler->k_runnable ++;
		list_add_first(&rQueues->kthreads, &thread->list);
	}
	else
	{
		rQueues->scheduler->user_nr ++;
		rQueues->scheduler->u_runnable ++;
		rQueues->u_runnable ++;
		list_add_last(&rQueues->runnable, &thread->list);

		if(thread_isImported(thread))
		{
			thread->boosted_prio = 3 * RR_QUANTUM;
			thread_clear_imported(thread);
		}
		else
			thread->boosted_prio = RR_QUANTUM;
	}
}

SCHED_SCOPE struct thread_s *rr_elect_balancing(struct sched_s *sched)
{
	register struct thread_s *elected;
	register rQueues_t *rQueues;
	register struct thread_s *this;
	register uint_t count;

	this    = current_thread;
	rQueues = (rQueues_t*) sched->data;
	elected = NULL;
	count   = sched->count;

	thread_sched_deactivate(this);

	if(this->type != TH_IDLE)
	{
		if(this->state == S_KERNEL)
		{
			this->state = S_READY;
			this->boosted_prio += this->quantum;
			count ++;
			
			if(this->type == PTHREAD)
			{
				if(thread_isForcedYield(this))
				{
					list_add_first(&rQueues->runnable, &this->list);
					this->boosted_prio -= this->quantum;
				}
				else
				{
					list_add_last(&rQueues->runnable, &this->list);
					thread_migration_deactivate(this);
					this->quantum = RR_QUANTUM;
				}

				rQueues->scheduler->u_runnable ++;
				rQueues->u_runnable ++;
			}
			else
			{
				list_add_last(&rQueues->kthreads, &this->list);
				rQueues->scheduler->k_runnable ++;
				this->quantum = RR_QUANTUM;
			}
		
		}
	}

	if(count > 0)
	{
		if(!(list_empty(&rQueues->kthreads)))
		{
			elected = list_first(&rQueues->kthreads, struct thread_s, list);
			rQueues->scheduler->k_runnable --;
		}
		else if(!(list_empty(&rQueues->migrate)))
		{
			elected = list_first(&rQueues->migrate, struct thread_s, list);
			rQueues->scheduler->u_runnable --;
			rQueues->m_runnable --;
		}
		else
		{
			elected = list_first(&rQueues->runnable, struct thread_s, list);
			rQueues->scheduler->u_runnable --;
			rQueues->u_runnable --;
		}

		rr_dmsg(INFO, 
			"%s: cpu %d, usage %d, u_runnable [%d,%d], count %d, usr %d, this type %d, "
			"elected pid %d, tid %d, type %d, flags %x, (%d && (%d || %d)) [%u]\n", 
			__FUNCTION__, 
			cpu_get_id(),
			rQueues->cpu->usage,
			rQueues->u_runnable,
			rQueues->m_runnable,
			count,
			rQueues->scheduler->user_nr,
			this->type,
			elected->task->pid,
			elected->info.order,
			elected->type,
			elected->flags,
			rQueues->cond1,
			rQueues->cond2,
			rQueues->cond3,
			cpu_time_stamp());

		list_unlink(&elected->list);
		count --;

		thread_sched_deactivate(elected);
		thread_clear_forced_yield(elected);
	}
   
	sched->count = count;
	return elected;
}

SCHED_SCOPE struct thread_s *rr_elect(struct sched_s *sched)
{
	register struct thread_s *elected;
	register rQueues_t *rQueues;
	register struct thread_s *this;
	register uint_t count;

	this = current_thread;
	thread_sched_deactivate(this);
	rQueues = (rQueues_t*) sched->data;
	elected = NULL;
  
	count = sched->count;
   
	if((this->state == S_KERNEL) && (this->type != TH_IDLE))
	{
		this->state = S_READY;

		if(this->type == PTHREAD)
		{
			list_add_last(&rQueues->runnable, &this->list);
			rQueues->scheduler->u_runnable ++;
		}
		else
		{
			//list_add_first(&rQueues->kthreads, &this->list);
			list_add_last(&rQueues->kthreads, &this->list);
			printk(INFO, "[%u] rr-elect adding on core %d: this %d, first thread %d, kthreads nbr %d\n", 
					cpu_time_stamp(), current_cpu->gid, this, 
					list_first(&rQueues->kthreads, struct thread_s, list),
					rQueues->scheduler->k_runnable );
			rQueues->scheduler->k_runnable ++;
		}
		count ++;
	}
   
	if(count > 0)
	{
		if(!(list_empty(&rQueues->kthreads)))
		{
			elected = list_first(&rQueues->kthreads, struct thread_s, list);
			printk(INFO, "[%u] rr-elect on core %d: this %d, elected %d, kthreads nbr %d\n", 
					cpu_time_stamp(), current_cpu->gid, this, elected, 
					rQueues->scheduler->k_runnable );
			rQueues->scheduler->k_runnable --;
		}
		else
		{
			elected = list_first(&rQueues->runnable, struct thread_s, list);
			rQueues->scheduler->u_runnable --;
			rQueues->u_runnable --;
		}
		
		list_unlink(&elected->list);
		count --;
   
		if((elected->state != S_CREATE) && (elected->state != S_READY))
			except_dmsg("cpu %d, tid %x, state %s\n",
				    cpu_get_id(), 
				    elected, 
				    thread_state_name[elected->state]);
		
		elected->quantum = RR_QUANTUM;
		thread_sched_deactivate(elected);
	}
   
	sched->count = count;
	return elected;
}


static const struct sched_ops_s rr_sched_op = 
{
	.exit        = &rr_exit,
	.yield       = &rr_yield,
	.sleep       = &rr_sleep,
	.wakeup      = &rr_wakeup,
	.strategy    = &rr_sched_strategy,
	.add_created = &rr_add_created,
	.add         = &rr_add_created,
	.remove      = &rr_remove,

#if CONFIG_CPU_LOAD_BALANCING
	.elect       = &rr_elect_balancing,
	.clock       = &rr_clock_balancing,
#else
	.elect       = &rr_elect,
	.clock       = &rr_clock,
#endif
};

error_t rr_sched_init(struct scheduler_s *scheduler, struct sched_s *sched)
{
	kmem_req_t req;
	register rQueues_t *rQueues;

	req.type  = KMEM_GENERIC;
	req.size  = sizeof(rQueues_t);
	req.flags = AF_BOOT | AF_ZERO;

	if((rQueues = kmem_alloc(&req)) == NULL)
		PANIC("rr-sched.init: fatal error NOMEM\n", 0);

	rQueues->cpu       = current_cpu;
	rQueues->period    = CONFIG_CPU_BALANCING_PERIOD * RR_QUANTUM;
	rQueues->scheduler = scheduler;

	list_root_init(&rQueues->kthreads);
	list_root_init(&rQueues->runnable);
	list_root_init(&rQueues->migrate);
	
	sched->count     = 0;
	sched->op        = rr_sched_op;
	sched->data      = rQueues;
	return 0;
}

#endif	/* !(CONFIG_USE_SCHED_LOCKS) */
