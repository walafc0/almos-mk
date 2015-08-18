/*
 * kern/sys_thread_detach.c - detach a joinable thread
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
#include <kmem.h>
#include <kmagics.h>
#include <errno.h>
#include <task.h>
#include <spinlock.h>

int sys_thread_detach (pthread_t tid)
{
	register struct task_s *task;
	struct thread_s *target_th;
	uint_t state;
	error_t err;

	task = current_task;

	if(tid > task->max_order)
	{
		err = EINVAL;
		goto fail_arg;
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
  
	if(!(thread_isJoinable(target_th)))
	{
		err = EINVAL;
		goto fail_not_joinable;
	}
  
	err = 0;

	spinlock_lock(&target_th->lock);

	thread_clear_joinable(target_th);

	if((target_th->info.join == NULL) && 
	   !(state = wait_queue_isEmpty(&target_th->info.wait_queue)))
	{
		wakeup_one(&target_th->info.wait_queue, WAIT_ANY);
	}

	spinlock_unlock(&target_th->lock);
  
fail_not_joinable:
fail_srch:
	spinlock_unlock(&task->lock);

fail_arg:
	current_thread->info.errno = err;
	return err;
}
