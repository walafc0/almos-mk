/*
 * kern/cpu.c - CPU-Manager related operations
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
#include <thread.h>
#include <cpu.h>
#include <rpc.h>
#include <cluster.h>
#include <wait_queue.h>
#include <libk.h>
#include <kdmsg.h>
#include <kmem.h>
#include <cpu-trace.h>
#include <sysfs.h>
#include <dqdt.h>

static void cpu_sysfs_op_init(sysfs_op_t *op);
void rpc_manager_init(struct cpu_s *cpu);

#if CONFIG_CPU_TRACE
struct cpu_trace_recorder_s cpu_trace_recorder_0 = { 0 };
struct cpu_trace_recorder_s cpu_trace_recorder_1 = { 0 };
struct cpu_trace_recorder_s cpu_trace_recorder_2 = { 0 };
struct cpu_trace_recorder_s cpu_trace_recorder_3 = { 0 };
#endif

error_t cpu_init(struct cpu_s *cpu, struct cluster_s *cluster, uint_t lid, uint_t gid)
{
	sysfs_op_t op;

	cpu->state   = CPU_DEACTIVE;
	cpu->lid     = lid;
	cpu->gid     = gid;
	cpu->cluster = cluster;
        cpu->fpu_owner = NULL;

#if CONFIG_CPU_TRACE  
        switch ( cpu->lid )
        {
                case 0:
                        cpu->trace_recorder = &cpu_trace_recorder_0;
                        break;
                case 1:
                        cpu->trace_recorder = &cpu_trace_recorder_1;
                        break;
                case 2:
                        cpu->trace_recorder = &cpu_trace_recorder_2;
                        break;
                case 3:
                        cpu->trace_recorder = &cpu_trace_recorder_3;
                        break;
                default:
                        cpu->trace_recorder = &cpu_trace_recorder_0;

        }
#endif

	if((arch_cpu_init(cpu)))
		PANIC("Faild to initialize architecture specific data for CPU %d\n", cpu->gid);

	cpu->time.tmstmp       = 0;
	cpu->time.cycles       = 0;
	cpu->time.ticks_nr     = 0;
	cpu->time.ticks_period = CPU_CLOCK_TICK;
	cpu->ticks_count       = 0;
	cpu->usage             = 0;
	cpu->busy_percent      = 0;
	cpu->irq_nr            = 0;
	cpu->spurious_irq_nr   = 0;
	cpu->rpc_working_thread = 0;

	alarm_manager_init(&cpu->alarm_mgr);
	sched_init(&cpu->scheduler);

	event_listner_init(&cpu->le_listner);
	rpc_listner_init(&cpu->re_listner);
	rpc_manager_init(cpu);

	sprintk(cpu->name,
#if CONFIG_ROOTFS_IS_VFAT
		"CPU%d"
#else
		"cpu%d"
#endif
		,lid);
	
	cpu_sysfs_op_init(&op);
	sysfs_entry_init(&cpu->node, &op, cpu->name);
	sysfs_entry_register(&cluster->entry->node, &cpu->node);

	cpu->prng_A = 65519;
	cpu->prng_C = 64037 & 0xFFFFFFFB;
	srand(cpu_time_stamp() & 0xFFF);
	return 0;
}

void cpu_compute_stats(struct cpu_s *cpu, sint_t threshold)
{
	register struct thread_s *idle;
	register uint_t cpu_ticks;
	register uint_t idle_percent;
	register uint_t busy_percent;
	register uint_t usage;

	idle      = cpu_get_thread_idle(cpu);  
	cpu_ticks = cpu->ticks_count;

	if((sint_t)cpu_ticks < threshold)
	{
#if CONFIG_DQDT_DEBUG
		dqdt_dmsg(1, "%s: cpu %d, low ticks number %d\n", __FUNCTION__, cpu->gid, cpu_ticks);
#endif
		return;
	}

	cpu_ticks         = (cpu_ticks) ? cpu_ticks : 1;
	idle_percent      = (idle->ticks_nr * 100) / cpu_ticks;
	idle_percent      = (idle_percent > 100) ? 100 : idle_percent;
	busy_percent      = 100 - idle_percent;
	usage             = busy_percent + (cpu->usage / 2);
	//usage             = cpu->usage + ((busy_percent - cpu->usage) / 2);
	cpu->usage        = usage;
	cpu->busy_percent = busy_percent;
	cpu_wbflush();

#if CONFIG_SHOW_CPU_USAGE
	printk(INFO, "INFO: cpu %d, cpu_ticks %d, idle %d, idle %d%%, busy %d%%, usage %d%%\n",
	       cpu->gid,
	       cpu_ticks,
	       idle->ticks_nr,
	       idle_percent,
	       busy_percent,
	       usage);
#endif

	cpu->ticks_count = 0;
	idle->ticks_nr   = 0;
}

static void cpu_time_update(struct cpu_s *cpu)
{
	uint64_t cycles;
	uint_t ticks_nr;

#if CONFIG_CPU_64BITS
	cycles   = cpu_get_cycles(cpu);
	ticks_nr = (cycles - cpu->time.cycles) / cpu->time.ticks_period;
#else
	register uint_t tm_now;
	register uint_t elapsed;
	register uint_t tm_stmp;

	cycles  = cpu->time.cycles;
	tm_stmp = cpu->time.tmstmp;

	/* TODO: use memory barrier here */

	tm_now  = cpu_time_stamp();

	if(tm_now < tm_stmp)
		elapsed = (UINT32_MAX - tm_stmp) + tm_now;
	else
		elapsed = tm_now - tm_stmp;
    
	cycles  += elapsed;
	ticks_nr = elapsed / cpu->time.ticks_period;
	cpu->time.tmstmp = tm_now;
#endif

	cpu->time.cycles    = cycles;
	cpu->time.ticks_nr += ticks_nr;
	cpu->ticks_count   += ticks_nr;
}

void cpu_time_reset(struct cpu_s *cpu)
{
	cpu_time_update(cpu);
	cpu->ticks_count  = 0;
	cpu->usage        = 0;
	cpu->busy_percent = 0;
	cpu_get_thread_idle(cpu)->ticks_nr = 100;
	cpu_wbflush();
}

void cpu_clock(struct cpu_s *cpu)
{
	register uint_t ticks;

	cpu_time_update(cpu);

	ticks = cpu_get_ticks(cpu);
	alarm_clock(&cpu->alarm_mgr, ticks);
	sched_clock(current_thread, ticks);
	
	if(((ticks % CONFIG_DQDT_MGR_PERIOD) == 0) && 
			(cpu == cpu->cluster->local_bscpu))
		dqdt_update();
}

//#define SHOW_RPC_THREAD_WAKEUP

//WARNING: using printk may create a deadlock
void cpu_ipi_notify(struct cpu_s *cpu, uint32_t ipi_val)
{
	struct thread_s *thread;
	bool_t isYield = false;

	if(rpc_is_pending(&cpu->re_listner))
	{
		if((thread = wakeup_one(cpu->rpc_wq, WAIT_ANY)))
		{
#ifdef SHOW_RPC_THREAD_WAKEUP
			printk(INFO, "[%u] RPC thread %x waked up on core %d\n", 
					cpu_time_stamp(), thread, current_cpu->gid);
#endif
			isYield = true;
		}
#ifdef SHOW_RPC_THREAD_WAKEUP
		else printk(INFO, "[%u] No RPC thread in rpc_wq on core %d\n", 
					cpu_time_stamp(), current_cpu->gid);
#endif
	}

	if(isYield || (ipi_val==(uint32_t)-1))//TODO: use a define instead of -1
	{
		thread = current_thread;
		thread_set_forced_yield(thread);
		if(thread->locks_count == 0)
			sched_yield(thread);
		else
			thread_sched_activate(thread);
	}

//done in do_interrupt!
#if 0
	if(event_is_pending(&cpu->le_listner))
	{
		wakeup_one(&cpu->event_mgr->info.wait_queue, WAIT_ANY);
	}
#endif
		
#if CONFIG_SHOW_CPU_IPI_MSG
	isr_dmsg(INFO, "[IPI] cpu %d, U %d, B %d , V %d [%u]\n", 
		 cpu->gid,
		 cpu->usage,
		 cpu->busy_percent,
		 ipi_val,
		 cpu_time_stamp());
#endif
}

void cpu_kentry(uint_t cpu_lid, struct thread_s *this)
{
	uint32_t *in_kernel;
	
	in_kernel = &cpus_in_kernel[cpu_lid];
	if(this->state == S_USR)
	{
		assert(!(*in_kernel));
		*in_kernel = 1;
	}else
	{
		assert(*in_kernel);
	}
}

void cpu_kexit(uint_t cpu_lid, struct thread_s *this)
{
	uint32_t *in_kernel;
	
	in_kernel = &cpus_in_kernel[cpu_lid];
	assert(*in_kernel);/* We must be in kernel mode */
	/* if we are going to user space then this->state == S_USR */
	if(this->state == S_USR)
	{
		*in_kernel = 0;
	}
}

inline bool_t __cpu_in_kernel(cid_t cid, gid_t lid)
{
	uint32_t val;

	val = remote_lw(&cpus_in_kernel[lid], cid);

	return !!val;
}

/* TODO: invalidate cache line ? */
bool_t cpu_in_kernel(cid_t cid, gid_t lid)
{
	return __cpu_in_kernel(cid,lid);
}

inline bool_t __cpu_check_sched(struct thread_s *this)
{
	struct cpu_s *cpu;
	
	this = current_thread;

	if(!thread_isPreemptable(this))
		return false;

	if(thread_sched_isActivated(this))
		return true;

	cpu = current_cpu;

	if(rpc_check())
	{
		if(wakeup_one(cpu->rpc_wq, WAIT_ANY))
			return true;
	}

	if(event_is_pending(&cpu->le_listner))
	{
		if(wakeup_one(&cpu->event_mgr->info.wait_queue, WAIT_ANY))
			return true;
	}

	return false;
}

inline bool_t cpu_yield()
{
	struct thread_s *this = current_thread;
	if(__cpu_check_sched(this))
	{
		sched_yield(this);
		//thread_set_forced_yield(this); ?
		return true;
	}

	return false;
}


inline struct cpu_s* cpu_lid2ptr(uint_t lid)
{
	return &current_cluster->cpu_tbl[lid];
}

static error_t cpu_sysfs_read_op(sysfs_entry_t *entry, sysfs_request_t *rq, uint_t *offset)
{
	register struct cpu_s *cpu;
	register uint_t th_nr;
	register uint_t u_runnable;
	register uint_t k_runnable;

	if(*offset != 0)
	{
		*offset = 0;
		rq->count = 0;
		return 0;
	}

	cpu        = sysfs_container(entry, struct cpu_s, node);
	u_runnable = cpu->scheduler.u_runnable;
	k_runnable = cpu->scheduler.k_runnable;
	th_nr      = cpu->scheduler.total_nr;
  
	sprintk((char*)rq->buffer, 
		"%s\n\tUsage %d %%\n\tTimer-IRQs %d\n\tDev-IRQs %d\n"
		"\tScheduler\n\t\tRunnable %d [k:%d u:%d]\n\t\tTotal %d [k:%d u:%d]\n",
		cpu->name,
		(cpu->usage >= 100) ? 100 : cpu->usage,
		cpu_get_ticks(cpu),
		cpu->irq_nr,
		u_runnable + k_runnable,
		k_runnable,
		u_runnable,
		th_nr,
		cpu->scheduler.total_nr - cpu->scheduler.user_nr,
		cpu->scheduler.user_nr);
  
	rq->count = strlen((const char*)rq->buffer);
	*offset   = 0;

	return 0;
}

static void cpu_sysfs_op_init(sysfs_op_t *op)
{
	op->open  = NULL;
	op->read  = cpu_sysfs_read_op;
	op->write = NULL;
	op->close = NULL;
}
