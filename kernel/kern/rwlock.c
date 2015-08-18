/*
 * kern/rwlock.c - Read-Write lock synchronization
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

#include <cpu.h>
#include <thread.h>
#include <scheduler.h>
#include <list.h>
#include <stdint.h>
#include <errno.h>
#include <spinlock.h>
#include <kmagics.h>

#include <rwlock.h>

/* TODO: put lock name as argument
 * and reconstruct each wait_queue name */
error_t rwlock_init(struct rwlock_s *rwlock)
{
	//spinlock_init(&rwlock->lock,"RWLOCK");
	mcs_lock_init(&rwlock->lock, "RWLOCK");
	rwlock->signature = RWLOCK_ID;
	rwlock->count     = 0;
	wait_queue_init(&rwlock->rd_wait_queue, "RWLOCK: Rreaders");
	wait_queue_init(&rwlock->wr_wait_queue, "RWLOCK: Writers");

	return 0;
}

error_t rwlock_wrlock(struct rwlock_s *rwlock)
{
	uint_t irq_state;

	mcs_lock(&rwlock->lock, &irq_state);
	//spinlock_lock(&rwlock->lock);

	if(rwlock->count == 0)
	{
		rwlock->count --;
		//spinlock_unlock(&rwlock->lock);
		mcs_unlock(&rwlock->lock, irq_state);
		return 0;
	}

	wait_on(&rwlock->wr_wait_queue, WAIT_LAST);
	//spinlock_unlock_nosched(&rwlock->lock);
	mcs_unlock(&rwlock->lock, irq_state);
	sched_sleep(current_thread);
	return 0;
}

error_t rwlock_rdlock(struct rwlock_s *rwlock)
{
	uint_t irq_state;

	//spinlock_lock(&rwlock->lock);
	mcs_lock(&rwlock->lock, &irq_state);

	/* priority is given to writers */
	if((rwlock->count >= 0) && (wait_queue_isEmpty(&rwlock->wr_wait_queue)))
	{
		rwlock->count ++;
		//spinlock_unlock(&rwlock->lock);
		mcs_unlock(&rwlock->lock, irq_state);
		return 0;
	}
  
	wait_on(&rwlock->rd_wait_queue, WAIT_LAST);
  
	//spinlock_unlock_nosched(&rwlock->lock);
	mcs_unlock(&rwlock->lock, irq_state);
	sched_sleep(current_thread);
	return 0;
}

error_t rwlock_trywrlock(struct rwlock_s *rwlock)
{
	register error_t err = 0;
	uint_t irq_state;

	//spinlock_lock(&rwlock->lock);
	mcs_lock(&rwlock->lock, &irq_state);

	if(rwlock->count != 0)
		err = EBUSY;
	else
		rwlock->count --;
  
	//spinlock_unlock(&rwlock->lock);
	mcs_unlock(&rwlock->lock, irq_state);
	return err;
}

error_t rwlock_tryrdlock(struct rwlock_s *rwlock)
{
	register error_t err = 0;
	uint_t irq_state;

	//spinlock_lock(&rwlock->lock);
	mcs_lock(&rwlock->lock, &irq_state);

	if((rwlock->count >= 0) && (wait_queue_isEmpty(&rwlock->wr_wait_queue)))
		rwlock->count ++;
	else
		err = EBUSY;

	//spinlock_unlock(&rwlock->lock);
	mcs_unlock(&rwlock->lock, irq_state);
	return err;
}

error_t rwlock_unlock(struct rwlock_s *rwlock)
{
	register error_t err = 0;
	uint_t irq_state;

	//spinlock_lock(&rwlock->lock);
	mcs_lock(&rwlock->lock, &irq_state);

	if(rwlock->count == 0)
	{
		printk(ERROR, "ERROR: Unexpected rwlock_unlock\n");
		err = EPERM;
		goto fail_eperm;
	}
  
	if(rwlock->count > 1)  /* caller is a reader and no waiting writers */
	{
		rwlock->count --; 
		goto unlock_end;
	}

	/* We are the last reader or a writer */
	rwlock->count = 0;

	if((wakeup_one(&rwlock->wr_wait_queue, WAIT_FIRST)))
	{
		rwlock->count --;
		goto unlock_end;
	}

	/* No pending writer was found */
	rwlock->count += wakeup_all(&rwlock->rd_wait_queue);
  
fail_eperm:
unlock_end:
	//spinlock_unlock(&rwlock->lock);
	mcs_unlock(&rwlock->lock, irq_state);
	return err;
}

error_t rwlock_destroy(struct rwlock_s *rwlock)
{
	register error_t err = 0;
	uint_t irq_state;

	//spinlock_lock(&rwlock->lock);
	mcs_lock(&rwlock->lock, &irq_state);

	if(rwlock->count != 0)
		err = EBUSY;

	//spinlock_unlock(&rwlock->lock);
	mcs_unlock(&rwlock->lock, irq_state);
	return err;
}
