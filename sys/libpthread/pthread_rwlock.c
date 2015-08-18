/*
 * pthread_mutex.c - pthread rwlock related functions
 * 
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS.
 *
 * ALMOS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <errno.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <cpu-syscall.h>

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

static int __sys_rwlock(pthread_rwlock_t *rwlock, uint_t operation)
{
	register int retval;
	retval = (int)cpu_syscall((void*)rwlock,(void*)operation,NULL,NULL,SYS_RWLOCK);
	return retval;
}

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr)
{
	return __sys_rwlock(rwlock, RWLOCK_INIT);
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
	return __sys_rwlock(rwlock, RWLOCK_TRYWRLOCK);
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
	return __sys_rwlock(rwlock, RWLOCK_WRLOCK);
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
	return __sys_rwlock(rwlock, RWLOCK_RDLOCK);
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
	return __sys_rwlock(rwlock, RWLOCK_TRYRDLOCK);
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	return __sys_rwlock(rwlock, RWLOCK_UNLOCK);
}

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
	return __sys_rwlock(rwlock, RWLOCK_DESTROY);
}
