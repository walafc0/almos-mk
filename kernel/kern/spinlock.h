/*
 * spinlock.c: kernel spinlock synchronization
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

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include <config.h>
#include <types.h>

struct spinlock_s;
typedef struct spinlock_s spinlock_t;
typedef sint_t slock_t;


/** Disable all irqs and lock a spinlock, Not a scheduling point */
void spinlock_lock_noirq(spinlock_t *lock, uint_t *irq_state);

/** Unlock a spinlock and restore old irqs activation mode, Not a scheduling point */
void spinlock_unlock_noirq(spinlock_t *lock, uint_t irq_state);

/** 
 * Lock a spinlock and increment thread current locks count
 * if current thread need to reschedule while spining on the lock
 * give up, do schedule, then retry agin.
 */
void spinlock_lock(spinlock_t *lock);

/**
 * Same as spinlock_lock but for slock_t type
 */
void spin_lock(slock_t *lock);

/**
 * Get a read access to the given spinlock
 * All readers are not blocked
 * If a writer (spinlock_lock) arrives, it will takes the priority
 * No priority is applied between writers
 */
void spinlock_rdlock(spinlock_t *lock);

/** 
 * Try once to lock a spinlock and increment 
 * thread current locks count only on success
 * Zero value is returned on success 
 */
uint_t spinlock_trylock(spinlock_t *lock);

/** Unlock a spinlock and do schedule if necessary */
void spinlock_unlock(spinlock_t *lock);

/**
 * Same as spinlock_unlock but for slock_t type
 */
void spin_unlock(slock_t *lock);

/** Unlock a spinlock and skip possible scheduling point */
void spinlock_unlock_nosched(spinlock_t *lock);

/**
 * Same as spinlock_unlock_nosched but for slock_t type
 */
void spin_unlock_nosched(slock_t *lock);

/** Initialize a spinlock and put it in free state */
inline void spinlock_init(spinlock_t *lock, char *name);

/** Same as spinlock_init but for slock_t type */
inline void spin_init(slock_t *lock);

/** Destroy a spinlock */
inline void spinlock_destroy(spinlock_t *lock);

/** Return lock's name */
static inline char* spinlock_get_name(spinlock_t *lock);

////////////////////////////////////////////
//              Private Section           //
////////////////////////////////////////////

struct thread_s;

struct spinlock_s
{
	sint_t val;		/* must be keeped sync with cpu_spinlock_lock */
	/* private fields */
	char *name;
	struct thread_s *owner;
}CACHELINE;

static inline char* spinlock_get_name(spinlock_t *lock)
{
	return lock->name;
}

#endif	/* _SPINLOCK_H_ */
