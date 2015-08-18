/*
 * kern/semaphore.c - semaphore related operations
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

#define SEM_VALUE_MAX  CONFIG_PTHREAD_THREADS_MAX

error_t sem_init(struct semaphore_s *sem, uint_t value, uint_t scope)
{
	if((value > SEM_VALUE_MAX) || (sem == NULL))
		return EINVAL;
  
	spinlock_init(&sem->lock, "Semaphore Sync");
	sem->signature = SEMAPHORE_ID;
	sem->value     = value;
	sem->count     = value;
	sem->scope     = scope;
	wait_queue_init(&sem->wait_queue, "Semaphore Sync");

	return 0;
}

error_t sem_getvalue(struct semaphore_s *sem, int *value)
{
	*value = (int) sem->count;
	return 0;
}

error_t sem_wait(struct semaphore_s *sem)
{ 
	register struct thread_s *this;

	this = current_thread;

	if((sem->value == 1) && (sem->count != 1) && (sem->owner == this))
	{
		printk(INFO,"INFO: %s: scope %d, tid %x, sem %x, err: EDEADLK",
		       __FUNCTION__, 
		       sem->scope, 
		       this, 
		       sem);

		return EDEADLK;
	}
 
	spinlock_lock(&sem->lock);
  
	if(--sem->count >= 0)
	{
		spinlock_unlock(&sem->lock);
	}
	else
	{
		wait_on(&sem->wait_queue, WAIT_LAST);
		spinlock_unlock_nosched(&sem->lock);
		sched_sleep(current_thread);
	}

	sem->owner = this;
	return 0;
}

error_t sem_post(struct semaphore_s *sem)
{
	register struct thread_s *thread;
	register struct thread_s *this;
  
	this   = current_thread;
	thread = NULL;

	if(sem->value == 1)
	{
		if(((sem->count <= 0) && (sem->owner != this)) || (sem->count > 0))
		{
			printk(INFO,"INFO: %s: scope %d, tid %x, sem %x: illegal operation\n",
			       __FUNCTION__, 
			       sem->scope, 
			       this, 
			       sem);

			return EINVAL;
		}
	}
  
	spinlock_lock(&sem->lock);
  
	if(++sem->count <= 0)
		thread = wakeup_one(&sem->wait_queue, WAIT_FIRST);
   
	sem->owner = thread;
	spinlock_unlock(&sem->lock);
	return 0;
}

error_t sem_trywait(struct semaphore_s *sem)
{
	register error_t err;
	register struct thread_s *this;

	err  = EAGAIN;
	this = current_thread;

	if((sem->value == 1) && (sem->count != 1) && (sem->owner == this))
	{
		printk(INFO,"INFO: %s: scope %d, tid %x, sem %x, err: EDEADLK\n",
		       __FUNCTION__, 
		       sem->scope, 
		       this, 
		       sem);

		return EDEADLK;
	}
 
	spinlock_lock(&sem->lock);

	if(sem->count > 0)
	{
		sem->count --;
		err = 0;
		sem->owner = this;
	}
	spinlock_unlock(&sem->lock);

	return err;
}

error_t sem_destroy(struct semaphore_s *sem)
{
	register error_t err = 0;
 
	spinlock_lock(&sem->lock);

	if(sem->count != sem->value)
		err = EBUSY;
	else
		sem->owner = 0;

	spinlock_unlock(&sem->lock);
  
	return err;
}
