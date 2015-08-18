/*
 * pthread.c - threads creation/synchronization related functions
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

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <cpu-syscall.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

pthread_mutex_t __printf_lock = PTHREAD_MUTEX_INITIALIZER;
uint_t ___dmsg_lock = 0;
uint_t ___dmsg_ok = 0;

void __pthread_init(void)
{
	__pthread_keys_init();
	__pthread_barrier_init();
}

void __pthread_tls_init(struct __pthread_tls_s *tls)
{
	memset(tls, 0, sizeof(*tls));
	tls->signature = __PTHREAD_OBJECT_CREATED;
	cpu_set_tls(tls);
	cpu_syscall(&tls->attr,NULL,NULL,NULL,SYS_GETATTR);
}

void pthread_exit (void *retval)
{
	cpu_syscall(retval,NULL,NULL,NULL,SYS_EXIT);
}

static void* __pthread_start(void* (*entry_func)(void*), void *arg)
{
	void *retval;
	__pthread_tls_t tls;
	pthread_t tid;
	struct __shared_s *shared;
  
	__pthread_tls_init(&tls);

	shared          = arg;
	shared->tid     = tls.attr.key;
	shared->mailbox = 0;

	__pthread_tls_set(&tls,__PT_TLS_SHARED, shared);

#if 0
	fprintf(stderr, "%s: tid %d, shared %x\n", 
		__FUNCTION__, 
		shared->tid, 
		(unsigned) shared);
#endif

	retval = entry_func(shared->arg);
  
	__pthread_keys_destroy();
	tid = pthread_self();

	if(tid != tls.attr.key)
	{
		fprintf(stderr,"tid %d, found %d\n", (uint32_t)tls.attr.key, (uint32_t)tid);
		while(1);
	}
 
	pthread_exit(retval);
	return retval; 		/* fake return */
}

static void __pthread_sigreturn(void)
{
	cpu_syscall(NULL,NULL,NULL,NULL,SYS_SIGRETURN);
}


int pthread_create (pthread_t *thread, pthread_attr_t *attr, void *(*start_routine) (void *), void *arg)
{
	register int retval;
	pthread_attr_t _attr;
	struct __shared_s *shared;

	//___dmsg_ok = 1;

	if(attr == NULL)
	{
		pthread_attr_init(&_attr);
		attr = &_attr;
	}

	if(attr->stack_addr != NULL)
	{
		if((attr->sigstack_addr = valloc(1)) == NULL)
			return ENOMEM;

		attr->sigstack_size = 2048;
		shared = (struct __shared_s*)(attr->sigstack_addr + attr->sigstack_size + 64);
	}
	else
	{
		shared = malloc(sizeof(*shared));

		if(shared == NULL)
			return ENOMEM;

		//fprintf(stderr, "%s: shared @%x\n", __FUNCTION__, (unsigned) shared);
	}
  
	shared->arg          = arg;
	attr->entry_func     = (void*)__pthread_start;
	attr->exit_func      = (void*)pthread_exit;
	attr->sigreturn_func = (void*)__pthread_sigreturn;
	attr->arg1           = (void*)start_routine;
	attr->arg2           = shared;
	
	retval = (int)cpu_syscall((void*)thread,attr,NULL,NULL,SYS_CREATE);
	return retval;
}

int pthread_join (pthread_t th, void **thread_return)
{
	register int retval;
	retval = (int)cpu_syscall((void*)th,(void*)thread_return,NULL,NULL,SYS_JOIN);
	return retval;
}

int pthread_detach(pthread_t thread)
{
	register int retval; 
	retval = (int)cpu_syscall((void*)thread,NULL,NULL,NULL,SYS_DETACH);
	return retval;
}

int pthread_migrate_np(pthread_attr_t *attr)
{
	register __pthread_tls_t *tls;
	register int retval;
	pthread_attr_t _attr;

	if(attr == NULL)
	{
		pthread_attr_init(&_attr);
		attr = &_attr;
	}

	retval = (int)cpu_syscall((void*)attr,NULL,NULL,NULL,SYS_MIGRATE);

	if(retval == 0)
	{
		tls = cpu_get_tls();
		cpu_syscall(&tls->attr,NULL,NULL,NULL,SYS_GETATTR);
	}

	return retval;
}

void pthread_yield (void)
{
	cpu_syscall(NULL,NULL,NULL,NULL,SYS_YIELD);
}

int pthread_once(pthread_once_t *once_control, void (*init_routine) (void))
{
	if((once_control == NULL) || (init_routine == NULL))
		return EINVAL;

	if(cpu_spinlock_trylock(&once_control->lock))
		return 0;

	if(once_control->state != 0)
	{
		cpu_spinlock_unlock(&once_control->lock);
		return 0;
	}
    
	once_control->state = 1;
	cpu_spinlock_unlock(&once_control->lock);

	init_routine();

	return 0;
}

pthread_t pthread_self (void)
{
	__pthread_tls_t *tls;

	tls = cpu_get_tls();
	assert((tls != NULL) && (tls->signature == __PTHREAD_OBJECT_CREATED));
	return (pthread_t) tls->attr.key;
}

int pthread_equal (pthread_t thread1, pthread_t thread2)
{
	return thread1 == thread2;
}

int pthread_profiling_np(int cmd, pid_t pid, pthread_t tid)
{
	return (int)cpu_syscall((void*)cmd, (void*)pid, (void*)tid, NULL,SYS_PS);
}
