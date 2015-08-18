/*
 * kern/cond_var.c - condition variables implementation
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

#include <types.h>
#include <errno.h>
#include <thread.h>
#include <kmem.h>
#include <scheduler.h>
#include <spinlock.h>
#include <wait_queue.h>
#include <kmagics.h>
#include <semaphore.h>
#include <cond_var.h>

error_t cv_init(struct cv_s *cv)
{  
	spinlock_init(&cv->lock, "CondVar Sync");
	cv->signature = COND_VAR_ID;
	wait_queue_init(&cv->wait_queue, "Condition Variable Sync");
	return 0;
}

error_t cv_wait(struct cv_s *cv, struct semaphore_s *sem)
{  
	int value;
	register error_t err;
	register struct thread_s *this;

	this = current_thread;
	(void)sem_getvalue(sem, &value);

	if((sem->owner != this) || (value > 0))
	{
		if(sem->scope == SEM_SCOPE_SYS)
		{
			printk(ERROR,"ERROR: cv_wait: TID %x, sem %x is not owned or not locked [ value %d ]\n",
			       this, 
			       sem, 
			       value);
		}
		return EINVAL;
	}

	spinlock_lock(&cv->lock);

	if((err=sem_post(sem)))
	{
		spinlock_unlock(&cv->lock);
		return err;
	}

	wait_on(&cv->wait_queue, WAIT_LAST);

	spinlock_unlock_nosched(&cv->lock);
	sched_sleep(current_thread);

	return sem_wait(sem);
}

error_t cv_signal(struct cv_s *cv)
{
	struct thread_s *thread;

	spinlock_lock(&cv->lock);
	thread = wakeup_one(&cv->wait_queue, WAIT_FIRST); 
	spinlock_unlock(&cv->lock);
	return 0;
}

error_t cv_broadcast(struct cv_s *cv)
{
	spinlock_lock(&cv->lock);
	wakeup_all(&cv->wait_queue); 
	spinlock_unlock(&cv->lock);
	return 0;
}

error_t cv_destroy(struct cv_s *cv)
{
	register error_t err = 0;

	spinlock_lock(&cv->lock);

	if(!(wait_queue_isEmpty(&cv->wait_queue)))
		err = EBUSY;
    
	spinlock_unlock(&cv->lock);
  
	return err;
}
