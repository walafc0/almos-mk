/*
 * kern/sys_thread_sleep.c - puts the current thread in sleep state
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

#include <scheduler.h>
#include <thread.h>
#include <task.h>

int sys_thread_sleep()
{
	struct thread_s *this;

	this = current_thread;

	if(this->info.isTraced == true)
	{
		printk(INFO, "%s: cpu %d, pid %d, tid %d, asked to go sleep [%d]\n", 
		       __FUNCTION__,
		       cpu_get_id(),
		       this->task->pid,
		       this->info.order,
		       cpu_time_stamp());
	}

	thread_set_cap_wakeup(this);
	sched_sleep(this);
  
	if(this->info.isTraced == true)
	{
		printk(INFO, "%s: cpu %d, pid %d, tid %d, resuming [%d]\n",
		       __FUNCTION__,
		       cpu_get_id(),
		       this->task->pid,
		       this->info.order,
		       cpu_time_stamp());
	} 

	return 0;
}
