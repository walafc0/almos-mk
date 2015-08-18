/*
 * semaphore.c - semaphore related functions
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
#include <sys/syscall.h>
#include <cpu-syscall.h>

typedef enum
{
	SEM_INIT,
	SEM_GETVALUE,
	SEM_WAIT,
	SEM_TRYWAIT,
	SEM_POST,
	SEM_DESTROY
} sem_operation_t;

static int __sys_sem(sem_t *sem, int operation, int pshared, int *value)
{
	register int retval;
  
	retval = (int)cpu_syscall((void*)sem,(void*)operation,(void*)pshared,(void*)value,SYS_SEMAPHORE);
	return retval;
}

int sem_init(sem_t *sem, int pshared, unsigned int value)
{
	int v          = (int)value;
	int *value_ptr = &v;

	return __sys_sem(sem, SEM_INIT, pshared, value_ptr);
}

int sem_getvalue(sem_t *sem, int *value)
{
	return __sys_sem(sem, SEM_GETVALUE, 0, value);
}

int sem_wait(sem_t *sem)
{
	return __sys_sem(sem, SEM_WAIT, 0, NULL);
}

int sem_trywait(sem_t *sem)
{
	return __sys_sem(sem, SEM_TRYWAIT, 0, NULL);
}

int sem_post(sem_t *sem)
{
	return __sys_sem(sem, SEM_POST, 0, NULL);
}

int sem_destroy(sem_t *sem)
{
	return __sys_sem(sem, SEM_DESTROY, 0, NULL);
}
