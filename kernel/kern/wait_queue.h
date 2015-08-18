/*
 * kern/wait_queue.h - wait-queue management interface
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

#ifndef _WAIT_QUEUE_H_
#define _WAIT_QUEUE_H_

#include <list.h>
#include <kdmsg.h>

/** Wait Order */
#define WAIT_FIRST  0
#define WAIT_LAST   1
#define WAIT_ANY    2

/** Wait Queue Structure */
struct wait_queue_s
{
	const char *name;
	sint_t state;
	struct list_entry root;
};

/** 
 * Initialize specified wait queue 
 */
static inline void wait_queue_init(struct wait_queue_s *queue, const char* name)
{
	queue->name = name;
	queue->state = 0;
	list_root_init(&queue->root);
}


/** 
 * Initialize the wait queue starting from a given one 
 */
static inline void wait_queue_init2(struct wait_queue_s *queue, struct wait_queue_s *another)
{
	queue->name = another->name;
	queue->state = 0;
	
	if(list_empty(&another->root))
		list_root_init(&queue->root);
	else
		list_replace(&another->root, &queue->root);
}


/** 
 * Destroy specified wait queue
 */
static inline void wait_queue_destroy(struct wait_queue_s *queue)
{
	assert(list_empty(&queue->root) && "Wait Queue is not empty");
}

/** 
 * Checks if specified wait queue is empty 
 * Caution: shared resource must be locked before calling this function
 */
static inline uint_t wait_queue_isEmpty(struct wait_queue_s *queue)
{
	return (list_empty(&queue->root));
}

/** 
 * Add current thread to wait queue upon to specified order
 * Caution: shared resource must be locked before calling this function
 */
error_t wait_on(struct wait_queue_s *queue, uint_t order);


/** 
 * Wakeup all waiting Threads on this queue 
 * Caution: shared resource must be locked before calling this function
 */
uint_t wakeup_all(struct wait_queue_s *queue);


/** 
 * Wakeup one waiting Thread upon to specified order 
 * Caution: shared resource must be locked before calling this function
 */
struct thread_s* wakeup_one(struct wait_queue_s *queue, uint_t order);


#endif	/* _WAIT_QUEUE_H_ */
