/*
 * kern/cpu.h - unified CPU related interface
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

#ifndef _CPU_H_
#define _CPU_H_

#include <config.h>
#include <types.h>
#include <hal-cpu.h>
#include <system.h>
#include <scheduler.h>
#include <event.h>
#include <time.h>
#include <sysfs.h>
#include <rpc.h>

struct cluster_s;
struct wait_queue_s;
struct irq_action_s;
struct cpu_trace_s;
struct arch_cpu_s;

enum
{
	CPU_ACTIVE,
	CPU_IDLE,
	CPU_LOWPOWER,
	CPU_SUSPEND,
	CPU_DEACTIVE
};

struct cpu_time_s
{
	uint64_t cycles;
	uint_t tmstmp;
	uint_t ticks_nr;
	uint_t ticks_period;  
};

struct cpu_s
{
#if CONFIG_CPU_TRACE
	/* Trace and auditing info */
	struct cpu_trace_recorder_s *trace_recorder;
#endif
	/* CPU IDs */
	uint_t lid;
	uint_t gid;
  
	/* ARCH specific data */
	struct arch_cpu_s arch;
  
	/* Received IRQ, Time & Stats issus */
	uint_t irq_nr;
	struct cpu_time_s time;
	uint_t ticks_count;
	uint_t usage;
	uint_t busy_percent;
	uint_t spurious_irq_nr;

	/* Time-Driven alarms manager */
	struct alarm_s alarm_mgr;

	/* Events Listners */
	struct event_listner_s le_listner;
	struct rpc_listner_s re_listner;

	/* Events Manager */
	struct thread_s *event_mgr;

	/* RPC Manager */
	struct thread_s *rpc_mgr;
	struct wait_queue_s *rpc_wq;
	uint_t rpc_working_thread;	//synchro. by disabling interrupt

	/* Scheduler */
	struct scheduler_s scheduler;
  
	/* CPU Idle Thread */
	struct thread_s *thread_idle;

	/* FPU Owner Thread */
	struct thread_s *fpu_owner;

	/* Cluster in which CPU is located */
	struct cluster_s *cluster;// to remove: always point to local cluster!?

	/* Random Seeds */
	uint_t prng_A;
	uint_t prng_C;
	uint_t last_num;

	/* CPU State */
	uint_t state;

	/* Sysfs informations */
	char name[SYSFS_NAME_LEN];
	sysfs_entry_t node;
};

/** Initialize CPU object */
error_t cpu_init(struct cpu_s *cpu, struct cluster_s *cluster, uint_t lid, uint_t gid);

/** Compute CPU workload */
void cpu_compute_stats(struct cpu_s *cpu, sint_t threshold);

/** Reset usage statistics */
void cpu_time_reset(struct cpu_s *cpu);

/** Hardware events */
void cpu_clock(struct cpu_s *cpu);
void cpu_ipi_notify(struct cpu_s *cpu, uint32_t ipi_val);

/** Set CPU idle thread */
#define cpu_set_thread_idle(cpu,idle)

/** Get CPU idle thread */
#define cpu_get_thread_idle(cpu,idle)

/** Set CPU state */
#define cpu_set_state(cpu,state)

/** Get CPU state */
#define cpu_get_state(cpu,state)

/** Get CPU ticks number */
#define cpu_get_ticks(cpu)

/** Get CPU ticks period */
#define cpu_get_ticks_period(cpu)

/** Get CPU current cycles number */
static inline uint64_t cpu_get_cycles(struct cpu_s *cpu);

/** Convert CPU Global Id (GID) to desciptor pointer */
//struct cpu_s* cpu_gid2ptr(uint_t gid);

inline struct cpu_s* cpu_lid2ptr(uint_t lid);

/* check if a cpu is in the kernel */
inline bool_t __cpu_in_kernel(cid_t cid, gid_t lid);
bool_t cpu_in_kernel(cid_t cid, gid_t lid);

#define cpu_check_sched(__thd) ({bool_t __ret = 0; if(__thd->skip_check++%2) __ret = __cpu_check_sched(__thd); __ret;})
inline bool_t __cpu_check_sched(struct thread_s *thd);
inline bool_t cpu_yield();

///////////////////////////////////////////////
//             Private Section               //
///////////////////////////////////////////////

#undef cpu_set_thread_idle
#define cpu_set_thread_idle(_cpu,_idle)		\
	do{					\
		(_cpu)->thread_idle = (_idle);	\
	}while(0)

#undef cpu_get_thread_idle
#define cpu_get_thread_idle(_cpu)      ((_cpu)->thread_idle)

#undef cpu_set_state
#define cpu_set_state(_cpu,_state)		\
	do{					\
		(_cpu)->state = (_state);	\
	}while(0)

#undef cpu_get_state
#define cpu_get_state(_cpu,_state)    ((_cpu)->state)

#undef cpu_get_ticks
#define cpu_get_ticks(_cpu)           ((_cpu)->time.ticks_nr)

#undef cpu_get_ticks_period
#define cpu_get_ticks_period(_cpu)    ((_cpu)->time.ticks_period)

static inline uint64_t cpu_get_cycles(struct cpu_s *cpu)
{
	uint64_t cycles;
  
#if CONFIG_CPU_64BITS
	cycles = cpu_time_stamp();
#else
	register uint_t tm_now;
	register uint_t tm_stmp;
	
        /* Spend cycles to prepare needed values */
	tm_stmp = cpu->time.tmstmp;
	cycles  = cpu->time.cycles;

	/* TODO: use a memory barrier here */

	/* Now lets read the current CPU cycles */
	tm_now  = cpu_time_stamp();
	
	/* Update the total number of elapsed cycles with regard to register overlaping */
	if(tm_now < tm_stmp)
		cycles += (UINT32_MAX - tm_stmp) + tm_now;
	else
		cycles += tm_now - tm_stmp;

#endif	/* CONFIG_CPU_64BITS */
	return cycles;
}

#endif	/* _CPU_H_ */
