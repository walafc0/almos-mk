/*
 * pthread_mutex.c - pthread mutex related functions
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
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <cpu-syscall.h>
#include <assert.h>
#include <unistd.h>

#define USE_PTSPINLOCK       1
#define USE_HEURISTIC_LOCK   0
//#define CONFIG_MUTEX_DEBUG

#ifdef CONFIG_MUTEX_DEBUG
#define mdmsg(...) fprintf(stderr, __VA_ARGS__)
#else
#define mdmsg(...)
#endif

#define print_mutex(x)							\
	do{mdmsg("%s: mtx @%x, [v %u, w %d, t %d, s %d, c %d]\n",	\
		 __FUNCTION__,						\
		 (unsigned)(x),						\
		 (unsigned)(x)->value,					\
		 (unsigned)(x)->waiting,				\
		 (x)->attr.type,					\
		 (x)->attr.scope,					\
		 (x)->attr.cntr);}while(0)

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	if(attr == NULL) return EINVAL;
  
	attr->type  = PTHREAD_MUTEX_DEFAULT;
	attr->scope = PTHREAD_PROCESS_PRIVATE;
	attr->cntr  = 0;
	return 0;
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared)
{
	if((attr == NULL) || ((pshared != PTHREAD_PROCESS_SHARED) && (pshared != PTHREAD_PROCESS_PRIVATE)))
		return EINVAL;
  
	attr->scope = attr->scope;
	return 0;
}

int pthread_mutexattr_getpshared(pthread_mutexattr_t *attr, int *pshared)
{
	if(attr == NULL)
		return EINVAL;
  
	*pshared = (int)attr->scope;
	return 0;
}

int pthread_mutexattr_gettype(pthread_mutexattr_t *attr, int *type)
{
	if(attr == NULL)
		return EINVAL;

	*type = (int)attr->type;
	return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	if(attr == NULL)
		return EINVAL;
  
	if(type == PTHREAD_MUTEX_RECURSIVE)
		attr->type = (uint8_t)type;
	else
		attr->type = PTHREAD_MUTEX_DEFAULT;

	return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	return 0;
}

int pthread_mutex_init (pthread_mutex_t *mutex, const pthread_mutexattr_t * attr)
{
	int err;
	pthread_mutexattr_t _attr;

	if(mutex == NULL)
		return EINVAL;

	if(attr == NULL)
	{
		pthread_mutexattr_init(&_attr);
		attr = &_attr;
	}

	err = pthread_spin_init(&mutex->lock, 0);

	if(err) return err;

	mutex->value      = __PTHREAD_OBJECT_FREE;
	mutex->waiting    = 0;
	mutex->attr.type  = attr->type;
	mutex->attr.scope = attr->scope;
	mutex->attr.cntr  = attr->cntr;

	if(mutex->attr.scope == PTHREAD_PROCESS_SHARED)
		return sem_init(&mutex->sem, PTHREAD_PROCESS_SHARED, 1);

	return 0;
}

int pthread_mutex_lock (pthread_mutex_t *mutex)
{
	volatile uint_t cntr;
	register __pthread_tls_t *tls;
	register uint_t this;
	register const uint_t limit1 = 100000;
	register const uint_t limit2 = 2 * limit1;
	int err;
  
	if(mutex == NULL)
		return EINVAL;

	cntr = 0;
	this = (uint_t)pthread_self();
	int pid = getpid();

	if(mutex->value == __PTHREAD_OBJECT_DESTROYED)
		return EINVAL;

	if((mutex->attr.type == PTHREAD_MUTEX_RECURSIVE) && (mutex->value == this))
	{
		mutex->attr.cntr += 1;
		return 0;
	}

	if(mutex->value == this)
		return EDEADLK;

	if(mutex->attr.scope == PTHREAD_PROCESS_SHARED)
		return sem_wait(&mutex->sem);

	for(cntr = 0; ((cntr < limit2) && (mutex->value != __PTHREAD_OBJECT_FREE)); cntr++)
	{
		if((cntr > limit1) && (mutex->value != __PTHREAD_OBJECT_FREE))// && (mutex->waiting > 4))
			break;
	}

#if USE_PTSPINLOCK
	err = pthread_spin_lock(&mutex->lock);
	if(err) return err;
#else
	cpu_spinlock_lock(&mutex->lock);
#endif

	if(mutex->value == __PTHREAD_OBJECT_FREE)
	{
		mutex->value = this;

#if USE_PTSPINLOCK
		(void)pthread_spin_unlock(&mutex->lock);
#else
		cpu_spinlock_unlock(&mutex->lock);
#endif
		return 0;
	}
   
	if(mutex->waiting == 0)
		list_root_init(&mutex->queue);
   
	mutex->waiting += 1;
	tls = cpu_get_tls();
      
	struct __shared_s *shared = (struct __shared_s*)__pthread_tls_get(tls,__PT_TLS_SHARED);
      
	list_add_last(&mutex->queue, &shared->list);
  
#if USE_PTSPINLOCK
	(void)pthread_spin_unlock(&mutex->lock);
#else
	cpu_spinlock_unlock(&mutex->lock);
#endif 
	/* TODO: compute syscall return value (signals treatment) */
	(void)cpu_syscall(NULL, NULL, NULL, NULL, SYS_SLEEP);

	cpu_wbflush();

	return 0;
}

int pthread_mutex_unlock (pthread_mutex_t *mutex)
{
	int err;
	struct __shared_s *next;
	uint_t this;

	int pid = getpid();

	if(mutex == NULL)
		return EINVAL;

	this = (uint_t)pthread_self();

	if((mutex->attr.type == PTHREAD_MUTEX_RECURSIVE) && 
	   (mutex->value == this)                        && 
	   (mutex->attr.cntr > 0))
	{
		mutex->attr.cntr -= 1;
		return 0;
	}

	if(mutex->attr.scope == PTHREAD_PROCESS_SHARED)
	{
		if(mutex->value != this)
			err = EPERM;
		else
			err = sem_post(&mutex->sem);

		return err;
	}

#if USE_PTSPINLOCK 
	err = pthread_spin_lock(&mutex->lock);
	if(err) return err;
#else
	cpu_spinlock_lock(&mutex->lock);
#endif

	if(mutex->value != this)
	{
		(void)pthread_spin_unlock(&mutex->lock);
		return EPERM;
	}

	if(mutex->waiting == 0)
	{
		mutex->value = __PTHREAD_OBJECT_FREE;

#if USE_PTSPINLOCK
		(void)pthread_spin_unlock(&mutex->lock);
#else
		cpu_spinlock_unlock(&mutex->lock);
#endif
		return 0;
	}

#if 0
	if(!(list_empty(&mutex->queue)))
#endif
	{
		next = list_first(&mutex->queue, struct __shared_s, list);
		list_unlink(&next->list);

		mutex->waiting --;
		mutex->value = next->tid;
		cpu_wbflush();

		(void)cpu_syscall((void*)next->tid, NULL, NULL, NULL, SYS_WAKEUP);
	}

#if USE_PTSPINLOCK
	(void)pthread_spin_unlock(&mutex->lock);
#else
	cpu_spinlock_unlock(&mutex->lock);
#endif
	return 0;
}


int pthread_mutex_trylock (pthread_mutex_t *mutex)
{
	int err;
	uint_t this;

	if(mutex == NULL)
		return EINVAL;

	this = (uint_t)pthread_self();

	if((mutex->attr.type == PTHREAD_MUTEX_RECURSIVE) && (mutex->value == this))
	{
		mutex->attr.cntr += 1;
		return 0;
	}

	if(mutex->value == this)
		return EDEADLK;

	if(mutex->attr.scope == PTHREAD_PROCESS_SHARED)
		return sem_trywait(&mutex->sem);

	if(mutex->value != __PTHREAD_OBJECT_FREE)
		return EBUSY;

#if USE_PTSPINLOCK
	err = pthread_spin_lock(&mutex->lock);
	if(err) return err;
#else
	cpu_spinlock_lock(&mutex->lock);
#endif
  
	if(mutex->value != __PTHREAD_OBJECT_FREE)
		err = EBUSY;
	else
		mutex->value = this;
  
#if USE_PTSPINLOCK
	(void)pthread_spin_unlock(&mutex->lock);
#else
	cpu_spinlock_unlock(&mutex->lock);
#endif

	return err;
}


int pthread_mutex_destroy (pthread_mutex_t *mutex)
{
	int err;

	if(mutex == NULL)
		return EINVAL;

	if(mutex->attr.scope == PTHREAD_PROCESS_SHARED)
		return sem_destroy(&mutex->sem);

	print_mutex(mutex);

	if((mutex->waiting != 0) || (mutex->value != __PTHREAD_OBJECT_FREE))
		return EBUSY;
  
	err = pthread_spin_destroy(&mutex->lock);

	if(err) return err;

	mutex->value   = __PTHREAD_OBJECT_DESTROYED;
	mutex->waiting = __PTHREAD_OBJECT_BUSY;
	return 0;
}

