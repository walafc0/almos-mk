/*
 * pthread_barrier.c - pthread barrier related functions
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
#include <sys/syscall.h>
#include <cpu-syscall.h>

#include <pthread.h>

typedef enum
{
	BARRIER_INIT_PRIVATE,
	BARRIER_WAIT,
	BARRIER_DESTROY,
	BARRIER_INIT_SHARED
} barrier_operation_t;


void __pthread_barrier_init(void)
{
	/* Nothing to do in this version */
}


static int __sys_barrier(void *sysid, uint_t operation, uint_t count)
{
	register int retval;
  
	retval = (int)cpu_syscall(sysid,(void*)operation,(void*)count,NULL,SYS_BARRIER);

	return retval;
}

int pthread_barrierattr_destroy(pthread_barrierattr_t *attr)
{
	if(attr == NULL)
		return EINVAL;

	attr->scope = -1;
	return 0;
}

int pthread_barrierattr_init(pthread_barrierattr_t *attr)
{
	if(attr == NULL)
		return EINVAL;

	attr->scope = PTHREAD_PROCESS_PRIVATE;
	return 0;
}

int pthread_barrierattr_getpshared(pthread_barrierattr_t *attr, int *scope)
{
	if((attr == NULL) || (attr->scope < 0) || (scope == NULL))
		return EINVAL;

	*scope = attr->scope;
	return 0;
}

int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int scope)
{
	if((attr == NULL) || ((scope != PTHREAD_PROCESS_PRIVATE) && (scope != PTHREAD_PROCESS_SHARED)))
		return EINVAL;

	attr->scope = scope;
	return 0;
}

int pthread_barrier_init (pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned count)
{
	if((attr != NULL) && (attr->scope == PTHREAD_PROCESS_SHARED))
	{
		barrier->scope = PTHREAD_PROCESS_SHARED;
		return __sys_barrier(&barrier->sysid, BARRIER_INIT_SHARED, count);
	}

	barrier->scope          = PTHREAD_PROCESS_PRIVATE;
	barrier->cntr.value     = count;
	barrier->count.value    = count;
	barrier->state[0].value = 0;
	barrier->state[1].value = 0;
	barrier->phase          = 0;
 
	return 0;
}

int pthread_barrier_wait (pthread_barrier_t *barrier)
{
	sint_t ticket;
	uint_t phase;
	uint_t signature;

	if(barrier->scope == PTHREAD_PROCESS_SHARED)
		return __sys_barrier(&barrier->sysid, BARRIER_WAIT, 0);

	phase  = barrier->phase;
	ticket = cpu_atomic_add(&barrier->cntr.value, -1);

	if(ticket == 1)
	{
		barrier->cntr.value                  = barrier->count.value;
		barrier->phase                       = ~(barrier->phase) & 0x1;
		barrier->state[barrier->phase].value = 0;
		barrier->state[phase].value          = 1;

		cpu_invalid_dcache_line(&barrier->phase);
		cpu_invalid_dcache_line((void*)&barrier->state[phase].value);

		return PTHREAD_BARRIER_SERIAL_THREAD;
	}

	cpu_invalid_dcache_line(&barrier->phase);

	while(barrier->state[phase].value == 0)
		;		/* wait */

	cpu_invalid_dcache_line((void*)&barrier->state[phase].value);
	return 0;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
  	if(barrier->scope == PTHREAD_PROCESS_SHARED)
		return __sys_barrier(barrier, BARRIER_DESTROY, 0);

	if(barrier->cntr.value != barrier->count.value)
		return EBUSY;

	return 0;
}

