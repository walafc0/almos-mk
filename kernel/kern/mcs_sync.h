/*
 * kern/mcs_sync.h - ticket-based barriers and locks synchronization
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

#ifndef _MCS_SYNC_H_
#define _MCS_SYNC_H_

#include <types.h>

///////////////////////////////////////////////
//             Public Section                //
///////////////////////////////////////////////
struct mcs_barrier_s;
struct mcs_lock_s;

typedef struct mcs_barrier_s mcs_barrier_t;
typedef struct mcs_lock_s mcs_lock_t;

void mcs_barrier_init(mcs_barrier_t *ptr, char *name, uint_t count);
void mcs_barrier_wait(mcs_barrier_t *ptr);

void mcs_lock_init(mcs_lock_t *ptr, char *name);

void mcs_lock(mcs_lock_t *ptr, uint_t *irq_state);
void mcs_unlock(mcs_lock_t *ptr, uint_t irq_state);

void mcs_lock_remote(mcs_lock_t *ptr, cid_t cid, uint_t *irq_state);
void mcs_unlock_remote(mcs_lock_t *ptr, cid_t cid, uint_t irq_state);
//////////////////////////////////////////////
//             Private Section              //
//////////////////////////////////////////////

struct mcs_barrier_s
{
	cacheline_t val;
	cacheline_t phase;
	cacheline_t cntr;
	cacheline_t ticket;
	cacheline_t ticket2;
	char        *name CACHELINE;
};

struct mcs_lock_s
{
	cacheline_t cntr;
	cacheline_t ticket;
	char        *name CACHELINE;
};



#endif	/* _MCS_SYNC_H_ */
