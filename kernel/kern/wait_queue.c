/*
 * kern/wait_queue.c - wait-queue management
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
#include <thread.h>
#include <task.h>
#include <list.h>
#include <rt_timer.h>
#include <wait_queue.h>

/* Shared resource must be locked before calling this function */
error_t wait_on(struct wait_queue_s *queue, uint_t order)
{
	struct thread_s *this = current_thread;
  
	this->info.queue = queue;

	if(order == WAIT_LAST)
		list_add_last(&queue->root, &this->list);
	else
		list_add_first(&queue->root, &this->list);

	return 0;
}

/* Shared resource must be locked before calling this function */
struct thread_s* wakeup_one(struct wait_queue_s *queue, uint_t order)
{
	struct thread_s *thread;
  
	if(list_empty(&queue->root))
		return NULL;

	if(order == WAIT_LAST)
		thread = list_last(&queue->root, struct thread_s, list);
	else
		thread = list_first(&queue->root, struct thread_s, list);
  
	list_unlink(&thread->list);
	thread->info.queue = NULL;
	sched_wakeup(thread);
	return thread;
}

/* Shared resource must be locked before calling this function */
uint_t wakeup_all(struct wait_queue_s *queue)
{
	struct list_entry *iter;
	struct thread_s *thread;
	register uint_t i = 0;

	list_foreach(&queue->root, iter)
	{
		thread = list_element(iter, struct thread_s, list);
		list_unlink(iter);
		assert(thread->signature == THREAD_ID);
		thread->info.queue = NULL;
		sched_wakeup(thread);
		i++;
	}

	return i;
}
