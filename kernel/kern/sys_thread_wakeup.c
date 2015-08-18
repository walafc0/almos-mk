/*
 * kern/sys_thread_wakeup.c - wakeup all indicated threads
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

#include <errno.h>
#include <kmagics.h>
#include <cpu.h>
#include <kdmsg.h>
#include <cluster.h>
#include <task.h>
#include <scheduler.h>
#include <thread.h>

/* 
 * FIXME: define spinlock_rdlock() so all locking on task->th_lock 
 * becoms rdlock but on join/detach/destroy 
 */
int sys_thread_wakeup(pthread_t tid, pthread_t *tid_tbl, uint_t count)
{
	struct task_s *task;
	struct thread_s *this;
	struct thread_s *target;
	pthread_t tbl[100];
	void *listner;
	uint_t event;
	sint_t i;
	error_t err;

	this = current_thread;
	task = this->task;
	i = -1;

	if(tid_tbl != NULL)
	{
		if((NOT_IN_USPACE((uint_t)tid_tbl + (count*sizeof(pthread_t)))) || 
		   (count == 0) || (count > 100))
		{
			err = -1;
			goto fail_tid_tbl;
		}

		if((err = cpu_copy_from_uspace(&tbl[0], tid_tbl, sizeof(pthread_t*) * count))) 
			goto fail_usapce;

		if(tbl[0] != tid)
		{
			err = -2;
			goto fail_first_tid;
		}
	}
	else
	{
		count = 1;
		tbl[0] = tid;
	}

	for(i = 0; i < count; i++)
	{
		tid = tbl[i];

		if(tid > task->max_order)
		{
			err = -3;
			goto fail_tid;
		}

		target = task->th_tbl[tid];
   
		if((target == NULL) || (target->signature != THREAD_ID))
		{
			err = -4;
			goto fail_target;
		}

		listner = sched_get_listner(target, SCHED_OP_UWAKEUP);
		event = sched_event_make(target,SCHED_OP_UWAKEUP);
    
		if(this->info.isTraced == true)
		{
			printk(INFO,"%s: tid %d --> tid %d [%d][%d]\n", 
			       __FUNCTION__, 
			       this->info.order, 
			       tid, 
			       cpu_time_stamp(),
			       i);
		}

		sched_event_send(listner,event);
		cpu_wbflush();
	}

	return 0;

fail_target:
fail_tid:
fail_first_tid:
fail_usapce:
fail_tid_tbl:

	printk(INFO, "%s: cpu %d, pid %d, tid %x, i %d, count %d, ttid %x, request has failed with err %d [%d]\n",
	       __FUNCTION__,
	       cpu_get_id(),
	       task->pid,
	       this,
	       i,
	       count,
	       tid,
	       err,
	       cpu_time_stamp());
  
	this->info.errno = EINVAL;
	return -1;
}
