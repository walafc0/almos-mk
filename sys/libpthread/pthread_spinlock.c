/*
 * pthread_spinlock.c - pthread spinlock related functions
 * 
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS.
 *
 * ALMOS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <errno.h>
#include <cpu-syscall.h>
#include <pthread.h>

#include <stdio.h>

#undef dmsg_r
#define dmsg_r(...)


int pthread_spin_init (pthread_spinlock_t *lock, int pshared)
{
	if (lock == NULL)
		return EINVAL;
  
	lock->val = __PTHREAD_OBJECT_FREE;

	return 0;
}

#include <unistd.h>

int pthread_spin_lock (pthread_spinlock_t *lock)
{
	volatile uint_t cntr;
	register bool_t isAtomic;
	register uint_t this;
	register uint_t limit;
	
	if (lock == NULL)
	{
		dmsg_r(stderr, "%s: pid %d, tid %d, EINVAL\n", 
		       __FUNCTION__, (int)getpid(), (unsigned)pthread_self);

		return EINVAL;
	}

	this = (uint_t)pthread_self();

	cpu_invalid_dcache_line(lock);

	// Check if lock is not owned by the caller thread.
	if (lock->val == this)
	{
		dmsg_r(stderr, "%s: pid %d, tid %d, 0x%x EDEADLK\n", 
		       __FUNCTION__, (int)getpid(), (unsigned)pthread_self(), (unsigned)lock);

		return EDEADLK;
	}

	cntr = 0;
	limit = 10000;

	// Take the lock
	while((isAtomic = cpu_atomic_cas(&lock->val, __PTHREAD_OBJECT_FREE, (sint_t)this)) == false)
	{
		cntr ++;
		if(cntr > limit)
		{
			pthread_yield();

			dmsg_r(stderr, "%s: pid %d, tid %d, val %d, 0x%x, yielded\n", 
			       __FUNCTION__, (int)getpid(), (unsigned)this, (unsigned)lock->val, (unsigned)lock);

			cpu_invalid_dcache_line(lock);
			limit = 2000;
			cntr  = 0;
		}
	}

	dmsg_r(stderr, "%s: pid %d, tid %d, 0x%x locked\n", 
	       __FUNCTION__, (int)getpid(), (unsigned)pthread_self(), (unsigned)lock);

	return 0;
}

int pthread_spin_trylock(pthread_spinlock_t *lock)
{
	register bool_t isAtomic;
	register uint_t this;

	if(lock == NULL)
		return EINVAL;

	this = (uint_t)pthread_self();
	cpu_invalid_dcache_line(lock);

	// Check if lock is not owned by the caller thread.
	if (lock->val == this)
		return EDEADLK;

	isAtomic = cpu_atomic_cas(&lock->val, __PTHREAD_OBJECT_FREE, (sint_t)this);
   
	if(isAtomic == false)
	{
		cpu_invalid_dcache_line(lock);
		return EBUSY;
	}

	return 0;
}

int pthread_spin_unlock (pthread_spinlock_t *lock)
{
	if (lock == NULL)
	{
		dmsg_r(stderr, "%s: pid %d, tid %d EINVAL\n", 
		       __FUNCTION__, (int)getpid(), (unsigned)pthread_self());
		return EINVAL;
	}

	// Check if lock is busy and owned by the caller thread.
	if (lock->val != pthread_self())
	{
		dmsg_r(stderr, "libpthread: %s: pid %d, tid %d, val %d, EPERM\n",
		       __FUNCTION__, (int)getpid(), (int)pthread_self(), (int)lock->val);
		
       		return EPERM;
	}
	// Update lock's control informations  
	lock->val = __PTHREAD_OBJECT_FREE;
	cpu_wbflush();
	cpu_invalid_dcache_line(lock);

	dmsg_r(stderr, "%s: pid %d, tid %d, 0x%x unlocked\n", 
	       __FUNCTION__, (int)getpid(), (unsigned)pthread_self(), (unsigned)lock);

	return 0;
}

int pthread_spin_destroy (pthread_spinlock_t *lock)
{
	if (lock == NULL)
		return EINVAL;

	// Check if lock is not occupied by a thread.
	// If lock is occupied, then return immediately with EBUSY error code.
	if(lock->val != __PTHREAD_OBJECT_FREE)
		return EBUSY;

	// Update lock's control informations.
	lock->val = __PTHREAD_OBJECT_DESTROYED;
	cpu_invalid_dcache_line(lock);
	cpu_wbflush();
	return 0;
}
