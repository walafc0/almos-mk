/*
 * kern/rwlock.h - Read-Write lock synchronization
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

#ifndef _RW_LOCK_H_
#define _RW_LOCK_H_

#include <types.h>
#include <wait_queue.h>
#include <list.h>
#include <kmem.h>
#include <spinlock.h>

typedef enum
{
	RWLOCK_INIT,
	RWLOCK_WRLOCK,
	RWLOCK_RDLOCK,
	RWLOCK_TRYWRLOCK,
	RWLOCK_TRYRDLOCK,
	RWLOCK_UNLOCK,
	RWLOCK_DESTROY
} rwlock_operation_t;

struct rwlock_s
{
	//spinlock_t lock;
	mcs_lock_t lock;
	uint_t signature;
	sint_t count;
	struct wait_queue_s rd_wait_queue;
	struct wait_queue_s wr_wait_queue;
};

#define rwlock_get_value(rwlock)

error_t rwlock_init(struct rwlock_s *rwlock);
error_t rwlock_wrlock(struct rwlock_s *rwlock);
error_t rwlock_rdlock(struct rwlock_s *rwlock);
error_t rwlock_trywrlock(struct rwlock_s *rwlock);
error_t rwlock_tryrdlock(struct rwlock_s *rwlock);
error_t rwlock_unlock(struct rwlock_s *rwlock);
error_t rwlock_destroy(struct rwlock_s *rwlock);

int sys_rwlock(struct rwlock_s **rwlock, uint_t operation);
KMEM_OBJATTR_INIT(rwlock_kmem_init);


///////////////////////////////////////////////////////
///                Private section                  ///
///////////////////////////////////////////////////////
#undef rwlock_get_value
#define rwlock_get_value(_rwlock) ((_rwlock)->count)

#endif	/* _RW_LOCK_H_ */
