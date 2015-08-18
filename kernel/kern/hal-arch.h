/*
 * kern/hal-arch.h - Architecture related Hardware Abstraction Layer
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

#ifndef  _HAL_ARCH_H_
#define  _HAL_ARCH_H_

#ifndef  _SYSTEM_H_
#error hal-arch.h cannot be included directly, use system.h instead
#endif

#include <types.h>

/** Used atructures */
struct cpu_s;
struct cluster_s;
struct irq_action_s;
struct event_s;
struct boot_info_s;
struct dqdt_cluster_s;
struct dqdt_attr_s;

/** Provided structures, functions and macros */
#define ARCH_THREAD_PAGE_ORDER	
struct arch_cpu_s;
struct arch_cluster_s;

/* CPUs/CLUSTERs Numbers */
#define CLUSTER_NR
#define CPU_PER_CLUSTER_NR
uint_t arch_onln_cpu_nr(void);
uint_t arch_onln_cluster_nr(void);
uint_t arch_cpu_per_cluster(void);
cid_t  arch_boot_cid(void);

static inline uint_t arch_cpu_gid(uint_t cid, uint_t lid);
static inline uint_t arch_cpu_lid(uint_t cpu_gid);
static inline uint_t arch_cpu_cid(uint_t cpu_gid);

/* Architecture Initialization */
void arch_init(struct boot_info_s *info);
void arch_memory_init(struct boot_info_s *info);
error_t arch_dqdt_build(struct boot_info_s *info);

/* Real time, Real clock timers */
#define CPU_CLOCK_TICK
#define MSEC_PER_TICK
#define CYCLES_PER_SECOND

/* Architecture context */
error_t arch_cpu_init(struct cpu_s *cpu);

/* IRQs, IPIs and others */
error_t arch_cpu_set_irq_entry(struct cpu_s *cpu, int irq_nr, struct irq_action_s *action);
error_t arch_cpu_get_irq_entry(struct cpu_s *cpu, int irq_nr, struct irq_action_s **action);
error_t arch_cpu_send_ipi(gid_t dest, uint32_t val);

/* Specific architecture-dependent DQDT macros & functions */
#define DQDT_DIST_NOTSET     0	/* must be zero (c.f: see dqdt_attr_init in kern/dqdt.h) */
#define DQDT_DIST_DEFAULT    1
#define DQDT_DIST_RANDOM     2

uint_t arch_dqdt_distance(struct dqdt_cluster_s *c1, struct dqdt_cluster_s *c2, struct dqdt_attr_s *attr);

/* Specific architecture depanding CPU operations */
typedef enum
{
	ARCH_PWR_IDLE,
	ARCH_PWR_SLEEP,
	ARCH_PWR_SHUTDOWN
} arch_power_state_t;

error_t arch_set_power_state(struct cpu_s *cpu, arch_power_state_t state);

/* Specific architecture depanding CLUSTER operations */


/* Specific architecture implementation */

#define ARCH_HAS_BARRIERS

sint_t  arch_barrier_init(struct cluster_s *cluster, struct event_s *event, uint_t count);
sint_t  arch_barrier_wait(struct cluster_s *cluster, uint_t barrier_id);
error_t arch_barrier_destroy(struct cluster_s *cluster, uint_t barrier_id);


#include <arch.h>

#endif	/* _HAL_CPU_H_ */
