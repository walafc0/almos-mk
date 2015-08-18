/*
 * kern/cond_var.h - condition variables interface
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

#ifndef _COND_VAR_H_
#define _COND_VAR_H_

#include <types.h>
#include <spinlock.h>
#include <wait_queue.h>

typedef enum
{
	CV_INIT,
	CV_WAIT,
	CV_SIGNAL,
	CV_BROADCAST,
	CV_DESTROY
} cv_operation_t;

struct cv_s
{
	spinlock_t lock;
	uint_t signature;
	struct wait_queue_s wait_queue;
};

struct semaphore_s;

error_t cv_init(struct cv_s *cv);
error_t cv_wait(struct cv_s *cv, struct semaphore_s *sem);
error_t cv_signal(struct cv_s *cv);
error_t cv_broadcast(struct cv_s *cv);
error_t cv_destroy(struct cv_s *cv);

int sys_cond_var(struct cv_s **cv, uint_t operation, struct semaphore_s **sem);

KMEM_OBJATTR_INIT(cv_kmem_init);

#endif	/* _COND_VAR_H_ */
