/*
 * kern/distlock.c - kernel distlock synchronization
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

#include <system.h>
#include <cpu.h>
#include <thread.h>
#include <cluster.h>
#include <scheduler.h>
#include <libk.h>
#include <distlock.h>


uint_t remote_spinlock_trylock(spinlock_t *lock, cid_t cid)
{ 
	uint_t mode;
	register bool_t isAtomic;
	register struct thread_s *this;

	this     = current_thread;
	isAtomic = false;

	cpu_disable_all_irq(&mode);

	if(remote_lw((void*)&lock->val,cid) == 0)
		isAtomic = remote_atomic_cas((void*)&lock->val, cid, 0, -1);
  
	if(isAtomic == false)
	{
		cpu_restore_irq(mode);
		return 1;
	}
  
	this->distlocks_count ++;

#if CONFIG_SPINLOCK_CHECK
	lock->owner = this;
#endif

	cpu_restore_irq(mode);  
	return 0;
}

static void __attribute__((noinline)) rm_spinlock_check(struct thread_s *this, uint_t *mode)
{
	if(cpu_check_sched(this))
		sched_yield(this);
   
	cpu_restore_irq(*mode);
	cpu_disable_all_irq(mode);
}

void remote_spinlock_lock(spinlock_t *lock, cid_t cid)
{
	volatile sint_t *ptr;
	register struct thread_s *this;
	register bool_t isAtomic;
	register sint_t val;
	uint_t mode;

	ptr      = &lock->val;
	this     = current_thread;
	isAtomic = false;

	cpu_disable_all_irq(&mode);
  
	while(isAtomic == false)
	{
		val = remote_lw((void*)ptr, cid);

		if(val != 0)
		{
			rm_spinlock_check(this,&mode);
			continue;
		}
    
		isAtomic = remote_atomic_cas((void*)ptr, cid, 0, -1);
	}

	this->distlocks_count ++;
	cpu_restore_irq(mode);
}


void remote_spinlock_unlock(spinlock_t *lock, cid_t cid)
{
	struct thread_s *this;

#if CONFIG_SPINLOCK_CHECK
	lock->owner = NULL;
#endif
	remote_sw((void*)&lock->val, cid, 0);

	this = current_thread;
	assert(this->distlocks_count > 0);
	this->distlocks_count -= 1;
}


#define __distlock_exec(lock, func)		\
({						\
	register uint_t __bcid;			\
	__bcid	 = arch_boot_cid();	\
	func(lock, __bcid);			\
})

void distlock_lock(distlock_t *lock)
{
	__distlock_exec(lock, remote_spinlock_lock);	
}


uint_t distlock_trylock(distlock_t *lock)
{
	return __distlock_exec(lock, remote_spinlock_trylock);	

}

void distlock_unlock(distlock_t *lock)
{
	__distlock_exec(lock, remote_spinlock_unlock);	
}
