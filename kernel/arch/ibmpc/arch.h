/*
 * arch.h - architecture specific implementation of kernel's HAL
 * (see kern/hal-arch.h)
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

#ifndef _ARCH_H_
#define _ARCH_H_

#include <config.h>
#include <segmentation.h>
#include <types.h>

#ifndef _HAL_ARCH_H_
#error arch.h connot be included directly, use system.h instead
#endif

/* CPUs/CLUSTERs Numbers */
#define CACHE_LINE_SIZE   CONFIG_CACHE_LINE_SIZE
#define __CPU_NR          CONFIG_CPU_NR
#define __CPUs            CONFIG_CPU_PER_CLUSTER_NR
#define __CLUSTER_NR      CONFIG_CLUSTER_NR
#define __CPU_BITS_NR     CONFIG_CPU_BITS_NR
#define __CLUSTER_BITS_NR CONFIG_CLUSTER_BITS_NR
#define __PAGE_SIZE       CONFIG_CACHE_LINE_SIZE

#undef CPU_NR
#undef CLUSTER_NR
#undef CPU_PER_CLUSTER
#undef GET_CPU_ID
#undef GET_CLUSTER_ID
#undef MAKE_CPU_ID

#define CPU_NR                      __CPU_NR
#define CLUSTER_NR                  __CLUSTER_NR
#define CPU_PER_CLUSTER             __CPUs
#define GET_CPU_ID(_cpu_id)         ((_cpu_id) & (__CPU_NR - 1))
#define GET_CLUSTER_ID(_cpu_id)     ((_cpu_id) >> __CPU_BITS_NR)
#define MAKE_CPU_ID(_cluster,_cpu)  (((_cluster) << __CPU_BITS_NR) | (_cpu))

/* Timer parameters macros */
#undef TIC
#undef CYCLES_PER_SECOND

#define TIC                 40000
#define CYCLES_PER_SECOND   200000
#define ALPHA               20

/* Structures definitions */
struct irq_action_s;

struct arch_cpu_s
{
	struct irq_action_s *action;
};

#endif	/* _ARCH_H_ */
