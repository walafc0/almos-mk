/*
 * pthread.c -  conditions variables related functions
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
#include <semaphore.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <cpu-syscall.h>

typedef enum
{
	CV_INIT,
	CV_WAIT,
	CV_SIGNAL,
	CV_BROADCAST,
	CV_DESTROY
} cv_operation_t;

static int __sys_cond_var(void *sysid, int operation, sem_t *sem)
{
	register int retval;
  
	retval = (int)cpu_syscall(sysid,(void*)operation,(void*)sem,NULL,SYS_COND_VAR);
	return retval;
}

int pthread_condattr_init(pthread_condattr_t *attr)
{
	if(attr == NULL)
		return EINVAL;

	attr->scope = PTHREAD_PROCESS_PRIVATE;
	return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
	if(attr == NULL)
		return EINVAL;

	attr->scope = -1;
	return 0;
}

int pthread_condattr_getpshared(pthread_condattr_t *attr, int *scope)
{
	if((attr == NULL) || (attr->scope < 0) || (scope == NULL))
		return EINVAL;

	*scope = attr->scope;
	return 0;
}

int pthread_condattr_setpshared(pthread_condattr_t *attr, int scope)
{
	if((attr == NULL) || ((scope != PTHREAD_PROCESS_PRIVATE) && (scope != PTHREAD_PROCESS_SHARED)))
		return EINVAL;

	attr->scope = scope;
	return 0;
}

int pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t *cond_attr)
{
	int err;

	if(cond == NULL) return EINVAL;

	if((cond_attr != NULL) && (cond_attr->scope == PTHREAD_PROCESS_SHARED))
	{
		cond->scope = PTHREAD_PROCESS_SHARED;
		return __sys_cond_var(&cond->sysid, CV_INIT, NULL);
	}

	cond->scope = PTHREAD_PROCESS_PRIVATE;

	err = pthread_spin_init(&cond->lock, 0);

	if(err) return err;

	cond->count = 0;
	return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	register __pthread_tls_t *tls;
	register struct __shared_s *shared;
	int err;

	if((cond == NULL) || (mutex == NULL) || (mutex->attr.scope != cond->scope))
		return EINVAL;

	if(cond->scope == PTHREAD_PROCESS_SHARED)
		return __sys_cond_var(&cond->sysid, CV_WAIT, &mutex->sem);

	if(cond->count == 0)
		list_root_init(&cond->queue);
  
	cond->count ++;

	tls = cpu_get_tls();
	shared = (struct __shared_s*)__pthread_tls_get(tls, __PT_TLS_SHARED);

	list_add_last(&cond->queue, &shared->list);

	(void)pthread_mutex_unlock(mutex);

	/* TODO: compute syscall return value (signals treatment) */
	(void)cpu_syscall(NULL, NULL, NULL, NULL, SYS_SLEEP);
  
	err = pthread_mutex_lock(mutex);
	return err;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
	struct __shared_s *next;
	int err;

	if(cond == NULL) return EINVAL;

	if(cond->scope == PTHREAD_PROCESS_SHARED)
		return __sys_cond_var(&cond->sysid, CV_SIGNAL, NULL);

	if(cond->count == 0)
		return 0;

	next = list_first(&cond->queue, struct __shared_s, list);
	list_unlink(&next->list);
	cond->count --;

	(void)cpu_syscall((void*)next->tid, NULL, NULL, NULL, SYS_WAKEUP);
	return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
	struct list_entry root;
	struct list_entry *list;
	struct __shared_s *next;
	uint_t count;
	uint_t i;
	int err = 0;
  
	if(cond == NULL) return EINVAL;
  
	if(cond->scope == PTHREAD_PROCESS_SHARED)
		return __sys_cond_var(&cond->sysid, CV_BROADCAST, NULL);

	if(cond->count == 0)
	{
		return 0;
	}

	count       = cond->count;
	cond->count = 0;
	list_replace(&cond->queue, &root);

	uint_t tbl[100];

	i = 0;
  
	while(count)
	{
		list_foreach_forward(&root, list)
		{
			next   = list_element(list, struct __shared_s, list);
			list_unlink(&next->list);
			tbl[i] = next->tid;
			count --;
			i++;

			if(i == 100)
			{
				i = 0;
				(void)cpu_syscall((void*)tbl[0], (void*)&tbl[0], (void*)100, NULL, SYS_WAKEUP);
			}
		}
	}

	if(i != 0)
		(void)cpu_syscall((void*)tbl[0], (void*)&tbl[0], (void*)i, NULL, SYS_WAKEUP);
    
	return err;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
	int err = 0;
  
	if(cond == NULL)
		return EINVAL;

	if(cond->scope == PTHREAD_PROCESS_SHARED)
		return __sys_cond_var(&cond->sysid, CV_DESTROY, NULL);

	if(cond->count != 0)
		err = EBUSY;

	return err;
}
