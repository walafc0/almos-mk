/*
 * kern/distlock.h: kernel distlock synchronization
 * 
 * Copyright (c) 2014 UPMC Sorbonne Universites
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

#ifndef _DISTLOCK_H_
#define _DISTLOCK_H_

#include <config.h>
#include <types.h>
#include <spinlock.h>

typedef struct spinlock_s distlock_t;

#define DISTLOCK_DECLARE(nm)	\
distlock_t nm = {.val = 0,	\
		.name = #nm,	\
		.owner = NULL	}	

#define DISTLOCK_TABLE(nm, nb)				\
distlock_t nm[nb] = { [0 ... (nb-1)] = { .val = 0,	\
					.name = #nm,	\
					.owner = NULL	} }	

/** 
 * Lock a distlock and increment thread current locks count
 * if current thread need to reschedule while disting on the lock
 * give up, do schedule, then retry agin.
 */
void distlock_lock(distlock_t *lock);
void remote_spinlock_lock(spinlock_t *lock, cid_t cid);

/** 
 * Try once to lock a distlock and increment 
 * thread current locks count only on success
 * Zero value is returned on success 
 */
uint_t distlock_trylock(distlock_t *lock);
uint_t remote_spinlock_trylock(spinlock_t *lock, cid_t cid);

/** Unlock a distlock and do schedule if necessary */
void distlock_unlock(distlock_t *lock);
void remote_spinlock_unlock(spinlock_t *lock, cid_t cid);

#if 0
/**
 * Get a read access to the given distlock
 * All readers are not blocked
 * If a writer (distlock_lock) arrives, it will takes the priority
 * No priority is applied between writers
 */
void distlock_rdlock(distlock_t *lock);
/** Unlock a distlock and skip possible scheduling point */
void distlock_unlock_nosched(distlock_t *lock);

/** Disable all irqs and lock a distlock, Not a scheduling point */
void distlock_lock_noirq(distlock_t *lock, uint_t *irq_state);

/** Unlock a distlock and restore old irqs activation mode, Not a scheduling point */
void distlock_unlock_noirq(distlock_t *lock, uint_t irq_state);
#endif

#if 0
/** Return lock's name */
static inline char* distlock_get_name(distlock_t *lock);

////////////////////////////////////////////
//              Private Section           //
////////////////////////////////////////////

struct thread_s;

#define DISTLOCK_NSIZE 16

struct __distlock_s
{
	sint_t val;  			/* order is fixed */
	char name[DISTLOCK_NSIZE];
	struct thread_s *owner;
	uint_t cid;
}CACHELINE;

static inline char* distlock_get_name(distlock_t *lock)
{
	return lock->name;
}
#endif
#endif
