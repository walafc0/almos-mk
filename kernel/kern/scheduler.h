/*
 * kern/scheduler.h - Per-CPU generic scheduler
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

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <types.h>
#include <list.h>
#include <kmagics.h>
#include <rr-sched.h>

struct sched_s;
struct thread_s;
struct scheduler_s;

typedef error_t sched_init_t     (struct scheduler_s *scheduler, struct sched_s *sched);
typedef void sched_add_t         (struct thread_s *thread);
typedef void sched_remove_t      (struct thread_s *thread);
typedef void sched_exit_t        (struct thread_s *thread);
typedef void sched_add_created_t (struct thread_s *thread);
typedef void sched_strategy_t    (struct sched_s *sched);

typedef void sched_yield_t    (struct thread_s *thread);
typedef void sched_sleep_t    (struct thread_s *thread);
typedef void sched_wakeup_t   (struct thread_s *thread);
typedef void sched_clock_t    (struct thread_s *thread, uint_t ticks_nr);
typedef void sched_setprio_t  (struct thread_s *thread, uint_t prio);

typedef struct thread_s* sched_elect_t (struct sched_s *sched);

struct sched_ops_s
{
	sched_add_t         *add;
	sched_remove_t      *remove;
	sched_exit_t        *exit;
	sched_elect_t       *elect;
	sched_yield_t       *yield;
	sched_sleep_t       *sleep;
	sched_wakeup_t      *wakeup;
	sched_strategy_t    *strategy;
	sched_clock_t       *clock;
	sched_add_created_t *add_created;
};

struct sched_s
{
	volatile uint_t count;      /* runnables counter */
	struct sched_ops_s op;
	void *data;
};

struct sched_param
{
	int sched_priority;
};

#define SCHED_RR            0
#define SCHED_FIFO          1
#define SCHED_OTHER         2

#define SCHEDS_NR           1

#define SCHED_OP_NOP
#define SCHED_OP_WAKEUP
#define SCHED_OP_UWAKEUP
#define SCHED_OP_ADD_CREATED

struct sched_db_s;

struct scheduler_s
{
	uint16_t total_nr;
	uint16_t user_nr;
	uint16_t u_runnable;
	uint16_t k_runnable;
	uint16_t export_nr;
	uint16_t import_nr;
	struct sched_db_s *db;
	struct list_entry sleep_check;
	struct sched_s scheds_tbl[SCHEDS_NR];
};

/** Init scheduler */
error_t sched_init(struct scheduler_s *scheduler);

/** Register a thread, must be called by a local thread */
error_t sched_register(struct thread_s *thread);

/** Add newly created thread */
void sched_add_created(struct thread_s *thread);

/** Add migrated thread, must be called by a local thread */
void sched_add(struct thread_s *thread);

/** Remove old thread (case of migration) */
void sched_remove(struct thread_s *this);

/** Exit and forget about it */
void sched_exit(struct thread_s *this);

/** Attach the the given thread to specific scheduling policy */
error_t sched_setpolicy(struct thread_s *thread, uint_t policy);

/** Get the currnet thread's scheduling policy */
uint_t sched_getpolicy(struct thread_s *thread);

/** Set thread priority */
void sched_setprio(struct thread_s *thread, uint_t prio);

/** Yield the current CPU */
int  sched_yield(struct thread_s *this);

/** Put the calling thread into passive wait state */
void sched_sleep(struct thread_s *this);


/** Put the calling thread into passive wait state 
 * The thread is to be checked at each schedule ... */
void sched_sleep_check(struct thread_s *this);

/** Wakeup the given thread from its passive wait */
void sched_wakeup(struct thread_s *thread);

/** Check threads sleeping on the sleep_check list */
void check_sleeping(struct scheduler_s *scheduler);

/** Enable the scheduler to balance its load and do some internal strategie */
void sched_strategy(struct scheduler_s *scheduler);

/** Signal the clock event to current thread's scheduling policy */
void sched_clock(struct thread_s *this, uint_t ticks_nr);

/** When current thread is idle inform the scheduler */
void sched_idle(struct thread_s *this);

/** Return runnable threads count (User & Kernel) */
static inline uint_t sched_runnable_count(struct scheduler_s *scheduler);

/** Return total number of alive threads attached to this scheduler (User & Kernel) */
static inline uint_t sched_alive_count(struct scheduler_s *scheduler);

/** Get the listner of the given scheduling operation */
void* sched_get_listner(struct thread_s *thread, uint_t sched_op);

/** Send the given scheduling event to the given listner */
static inline void sched_event_send(void *listner, uint_t event);

/** Make a scheduling event relative to thread and given scheduling operation */
#define sched_event_make(thread,operation)

/** Return true if there's pending scheduling event for given listner */
#define sched_isPending(listner,operation)

///////////////////////////////////////////////////
//                Private Section                //
///////////////////////////////////////////////////

#undef  SCHED_OP_NOP	         /* 12 bits = OP(4) + Port(8) */
#define SCHED_OP_NOP             0x000

#undef  SCHED_OP_WAKEUP          /* OP:1, Port:0 */
#define SCHED_OP_WAKEUP          0x100

#undef  SCHED_OP_UWAKEUP         /* OP:1, Port:1 */
#define SCHED_OP_UWAKEUP         0x101

#undef  SCHED_OP_ADD_CREATED     /* OP:2, Port:0 */
#define SCHED_OP_ADD_CREATED     0x200

//#undef  SCHED_OP_ADD		 /* OP:4, Port:0 */
//#define SCHED_OP_ADD             0x400

#define SCHED_PORT(x)            ((x) & 0xFF)
#define SCHED_OP(x)              ((x) >> 8)

#define SCHED_PORTS_NR           2
#define SCHED_OP_MASK            0xFFF

#undef sched_event_make
#define sched_event_make(_thread,_op) (((uint_t)(_thread)) | (_op))

#undef sched_isPending
#define sched_isPending(_listner,_op) (*((uint_t*)(_listner)) & (_op))

static inline void sched_event_send(void *listner, uint_t event)
{
	*((uint_t*)listner) = event;
}

static inline uint_t sched_runnable_count(struct scheduler_s *scheduler)
{
	volatile uint16_t *ptr;
	uint_t count;

	check_sleeping(scheduler);

	ptr = &scheduler->u_runnable;
	count = *ptr;

	ptr = &scheduler->k_runnable;
	count += *ptr;

	return count;
}

static inline uint_t sched_alive_count(struct scheduler_s *scheduler)
{
	volatile uint16_t *ptr;
 
	ptr = &scheduler->total_nr;
  
	return *ptr;
}


#endif	/* _SCHEDULER_H_ */
