/*
 * kern/dqdt.h - Distributed Quaternary Decision Tree
 * 
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012,2013,2014,2015 UPMC Sorbonne Universites
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

#ifndef _DQDT_H_
#define _DQDT_H_

#include <config.h>
#include <types.h>
#include <atomic.h>

#define DQDT_MAX_DEPTH        CONFIG_MAX_DQDT_DEPTH
#define DQDT_LEVELS_NR        CONFIG_DQDT_LEVELS_NR
#define DQDT_CLUSTER_UP       0x01
#define DQDT_CLUSTER_READY    0x02

struct boot_info_s;
struct cluster_s;
struct cpu_s;

/** Opaque structure */
struct dqdt_attr_s;

/** Initialize DQDT internal logics */
void dqdt_init(struct boot_info_s *info);

/** Initialize DQDT attribut, data is a service-specific info */
#define dqdt_attr_init(attr,data)

/** Get the selected cluster */
#define dqdt_attr_get_cid(attr)

/** Get the selected CPU */
#define dqdt_attr_get_cpu_id(attr)

/** Print DQDT starting from the given logical cluster */
void dqdt_print(struct dqdt_cluster_s *cluster);

/** Print summary information of the given logical cluster */
void dqdt_print_summary(struct dqdt_cluster_s *cluster);

/** Update the core-usage & memory estimations, should be called periodically */
error_t dqdt_update(void);

/** Update the threads number, should be called on each task's placement or destroy made outside the DQDT*/
#if CONFIG_USE_DQDT
void dqdt_update_threads_number(struct dqdt_cluster_s *logical, uint_t core_index, sint_t count);
#else
void dqdt_update_threads_number(uint_t cid, uint_t core_index, sint_t count);
#endif

/** Ask the DQDT for a thread placement decision */
error_t dqdt_thread_placement(struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr);

/** Ask the DQDT for a target decision for a thread migration */
error_t dqdt_thread_migrate  (struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr);

/** Ask the DQDT for a task placement decision */
error_t dqdt_task_placement  (struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr);

/** Ask the DQDT for a cluster decision for a memory allocation */
error_t dqdt_mem_request     (struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr);

/** Get the logical cluster of the given level */
struct dqdt_cluster_s* dqdt_logical_lookup(uint_t level);

///////////////////////////////////////////////
//             Private Section               //                  
///////////////////////////////////////////////

extern struct dqdt_cluster_s *dqdt_root;

struct dqdt_attr_s
{
	/* Public Members */
	uint_t cid;
        uint_t cid_exec;                /* cid where the task is moved on exec() */
	uint_t cpu_id;//local to cid
	void *data;
	uint_t tm_request;

	/* Private Members */
	uint_t flags;
	uint_t m_threshold;
	uint_t u_threshold;
	uint_t d_attr;
	sint_t *select_tbl;
	struct dqdt_cluster_s *origin;
};

#undef dqdt_attr_init
#define dqdt_attr_init(_attr_,_data_)			\
	do{(_attr_)->cid	    = (uint_t) -1;	\
		(_attr_)->cpu_id    = (uint_t) -1;	\
		(_attr_)->d_attr = 0;			\
		(_attr_)->data = (_data_);		\
	}while(0)


#undef dqdt_attr_get_cid
#define dqdt_attr_get_cid(_attr) ((_attr)->cid)

#undef dqdt_attr_get_cpu_id
#define dqdt_attr_get_cpu_id(_attr) ((_attr)->cpu_id)

typedef struct dqdt_indicators_s
{
	atomic_t T;
	uint_t   U;
	uint_t   M;
	uint_t   pages_tbl[CONFIG_PPM_MAX_ORDER] CACHELINE;
} dqdt_indicators_t;

typedef struct dqdt_estimation_s
{
	dqdt_indicators_t summary;
	dqdt_indicators_t tbl[4];
} dqdt_estimation_t;

struct dqdt_cluster_s
{
	uint_t index;
	uint_t level;
	uint_t flags;
	uint_t childs_nr;
	uint_t cores_nr;
	dqdt_estimation_t info;
	struct cluster_s *home;
	struct dqdt_cluster_s *parent;
	struct dqdt_cluster_s *children[4];
};

/////////////////////////////////////////////////////

#endif	/* _DQDT_H_ */
