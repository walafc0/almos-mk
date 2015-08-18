/*
 * kern/semaphore.h - semaphore related operations
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

#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <types.h>
#include <spinlock.h>
#include <wait_queue.h>
#include <kmem.h>

#define SEM_SCOPE_SYS  1
#define SEM_SCOPE_USR  2

typedef enum
{
	SEM_INIT,
	SEM_GETVALUE,
	SEM_WAIT,
	SEM_TRYWAIT,
	SEM_POST,
	SEM_DESTROY
} sem_operation_t;

struct semaphore_s
{
	spinlock_t lock;
	uint_t signature;
	uint_t value;
	sint_t count;
	uint_t scope;
	struct thread_s *owner;
	struct wait_queue_s wait_queue;
};

error_t sem_init(struct semaphore_s *sem, uint_t value, uint_t scope);
error_t sem_getvalue(struct semaphore_s *sem, int *value);
error_t sem_wait(struct semaphore_s *sem);
error_t sem_trywait(struct semaphore_s *sem);
error_t sem_post(struct semaphore_s *sem);
error_t sem_destroy(struct semaphore_s *sem);

int sys_sem(struct semaphore_s **sem, uint_t operation, uint_t pshared, int *value);
KMEM_OBJATTR_INIT(sem_kmem_init);

#endif	/* _SEMAPHORE_H_ */
