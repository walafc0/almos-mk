/*
 * kern/sys_thread_exit.c - terminates the execution of current thread
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
#include <wait_queue.h>
#include <cluster.h>
#include <dqdt.h>
#include <task.h>

int sys_thread_exit (void *exit_val)
{
	struct thread_s *this = current_thread;
	struct cpu_s *cpu;
	uint_t state;
	bool_t isEmpty;
	bool_t isReleased;

	cpu     = current_cpu;
 
	/* TODO: the cpu->lid must match the core index in the logical cluster */
	if(this->task->pid != 1)
		dqdt_update_threads_number(current_cid, cpu->lid, -1);

	spinlock_lock(&this->lock);

	if(!(thread_isJoinable(this)))
	{
		spinlock_unlock_nosched(&this->lock);
		goto exit_dead;
	}

	// Check if there's a thread waiting the end of callee thread
	isEmpty = wait_queue_isEmpty(&this->info.wait_queue);

	if(isEmpty)
	{
		this->info.exit_value = exit_val;
		wait_on(&this->info.wait_queue, WAIT_ANY);
		spinlock_unlock_nosched(&this->lock);
		sched_sleep(this);
	}
	else
	{
		this->info.join->info.exit_value = exit_val;
		wakeup_one(&this->info.wait_queue, WAIT_ANY);
		spinlock_unlock_nosched(&this->lock);
	}

exit_dead:

	isReleased = false;

	/*
	 * Remove FPU ownership
	 */
	cpu_disable_all_irq(&state);

	if(current_cpu->fpu_owner == this)
	{
		current_cpu->fpu_owner = NULL;
		isReleased = true;
	}

	cpu_restore_irq(state);

	if(isReleased)
	{
		printk(INFO, "INFO: Thread %x has released FPU on CPU %d\n",
		       this, 
		       current_cpu->gid);
	}

	sched_exit(this);

	PANIC ("Thread %x, CPU %d must never return", this, current_cpu->gid);
	return -1;		/* Fake return ! */
}
