/*
 * kern/barrier.h - barrier synchronization interface
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

#ifndef _BARRIER_H_
#define _BARRIER_H_

#include <config.h>
#include <types.h>
#include <mcs_sync.h>

typedef enum
{
	BARRIER_INIT_PRIVATE,
	BARRIER_WAIT,
	BARRIER_DESTROY,
	BARRIER_INIT_SHARED,
} barrier_operation_t;

struct task_s;
struct wait_queue_db_s;
struct cluster_s;

#define BARRIER_WQDB_NR    CONFIG_BARRIER_WQDB_NR

struct barrier_s
{
	union
	{
		atomic_t waiting;
		spinlock_t lock;
	};

	uint_t signature;
	struct task_s *owner;
	uint_t count;

	union
	{
		uint_t index;
		uint_t hwid;
	};

	uint_t state[2];
	uint_t phase;
	uint_t tm_first;
	uint_t tm_last;
	struct cluster_s *cluster;
	struct wait_queue_db_s *wqdb_tbl[BARRIER_WQDB_NR];
	struct page_s *pages_tbl[BARRIER_WQDB_NR];
	const char *name;
	struct event_s event;
};

error_t barrier_init(struct barrier_s *barrier, uint_t count, uint_t scope);
error_t barrier_destroy(struct barrier_s *barrier);
error_t barrier_wait(struct barrier_s *barrier);
int sys_barrier(struct barrier_s **barrier, uint_t operation, uint_t count);

KMEM_OBJATTR_INIT(wqdb_kmem_init);
KMEM_OBJATTR_INIT(barrier_kmem_init);

#endif	/* _BARRIER_H_ */
