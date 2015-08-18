/*
 * kern/sys_thread_join.c - passive wait on the end of a given thread
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

#include <list.h>
#include <thread.h>
#include <scheduler.h>
#include <kmem.h>
#include <errno.h>
#include <task.h>
#include <spinlock.h>

int sys_thread_join (pthread_t tid, void **thread_return)
{
	register struct task_s *task;
	register struct thread_s *this;
	register struct thread_s *target_th;
	register uint_t state = 0;
	void *retval;
	int err;

	this   = current_thread;
	task   = this->task;
	retval = 0;

	if((tid > task->max_order) || 
	   ((thread_return != NULL) && 
	    (NOT_IN_USPACE((uint_t)thread_return + sizeof(void*)))))
	{
		err = EINVAL;
		goto fail_arg;
	}

	/* try to write to userland address */
	if(thread_return)
	{
		if((err = cpu_copy_to_uspace(thread_return, &retval, sizeof(void *)))) 
			goto fail_uspace_ret;
	}

	spinlock_lock(&task->th_lock);
	target_th = task->th_tbl[tid];

	if((target_th == NULL)                 ||
	   (target_th->signature != THREAD_ID) || 
	   (target_th->info.attr.key != tid))
	{
		err = ESRCH;
		goto fail_srch;
	}

	if(target_th == this)
	{
		err = EDEADLK;
		goto fail_deadlock;
	}

	if(!(thread_isJoinable(target_th)))
	{
		err = EINVAL;
		goto fail_joinable;
	}
   
	spinlock_lock(&target_th->lock);
   
	if(target_th->info.join != NULL)
	{
		spinlock_unlock(&target_th->lock);
		err = EINVAL;
		goto fail_joined;
	}

	// Get the exit code of the target thread
	if ((state=wait_queue_isEmpty(&target_th->info.wait_queue)))
	{
		target_th->info.join = this;
		wait_on(&target_th->info.wait_queue, WAIT_ANY);
		spinlock_unlock(&target_th->lock);
		spinlock_unlock_nosched(&task->th_lock);
		sched_sleep(this);
		retval = this->info.exit_value;
	}
	else
	{
		retval = target_th->info.exit_value;
		wakeup_one(&target_th->info.wait_queue, WAIT_ANY);
		spinlock_unlock(&target_th->lock);
		spinlock_unlock(&task->th_lock);
	}

	/* Probably will not fail */
	if(thread_return)
	{
		if((err = cpu_copy_to_uspace(thread_return, &retval, sizeof(void *)))) 
			goto fail_uspace;
	}

	return 0;

fail_joined:
fail_joinable:
fail_deadlock:
fail_srch:
	spinlock_unlock(&task->th_lock); 

fail_uspace:
fail_uspace_ret:
fail_arg:
	this->info.errno = err;
	return err;
}
