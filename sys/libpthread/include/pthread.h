/*
 * pthread.h -  pthread API
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

#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#include <sys/types.h>
#include <sched.h>
#include <sys/list.h>
#include <semaphore.h>

/* START COPYING FROM KERNEL HEADER */
#define __PT_ATTR_DEFAULT             0x000
#define __PT_ATTR_DETACH              0x001 /* for compatiblity */
#define __PT_FORK_WILL_EXEC           0x001 /* for compatiblity */
#define __PT_FORK_USE_TARGET_CPU      0x002 /* for compatiblity */
#define __PT_ATTR_LEGACY_MASK         0x003 /* TODO: remove legacy attr issue*/

#define __PT_FORK_USE_AFFINITY        0x004
#define __PT_ATTR_MEM_PRIO            0x008
#define __PT_ATTR_INTERLEAVE_SEQ      0x010
#define __PT_ATTR_INTERLEAVE_ALL      0x020
#define __PT_ATTR_AUTO_MGRT           0x040
#define __PT_ATTR_AUTO_NXTT           0x080
#define __PT_ATTR_MEM_CID_RR          0x100

typedef struct
{
	uint_t key;
	uint_t flags;
	uint_t sched_policy;
	uint_t inheritsched;
	void *stack_addr;
	size_t stack_size;
	void *entry_func;
	void *exit_func;
	void *arg1;
	void *arg2;
	void *sigreturn_func;
	void *sigstack_addr;
	size_t sigstack_size;
	struct sched_param  sched_param;
	sint_t cid;
	sint_t cpu_lid;
	sint_t cpu_gid;
	uint_t tid; // kernel cookie can be used in debuggin userland
	pid_t pid;
} pthread_attr_t;
/* END COPYING FROM KERNEL HEADER */

/* Pthread related constants */
#define PTHREAD_THREADS_MAX           1024
#define PTHREAD_STACK_MIN             2*4096
#define PTHREAD_CREATE_DETACHED       1
#define PTHREAD_CREATE_JOINABLE       0 
#define PTHREAD_EXPLICIT_SCHED        0
#define PTHREAD_INHERIT_SCHED         1
#define PTHREAD_SCOPE_SYSTEM          0
#define PTHREAD_SCOPE_PROCESS         1
#define PTHREAD_PROCESS_PRIVATE       0
#define PTHREAD_PROCESS_SHARED        1
#define SEM_VALUE_MAX                 PTHREAD_THREADS_MAX

struct __shared_s
{
	volatile uint_t mailbox __CACHELINE;
	struct list_entry list;
	int tid;
	void *arg;
};

#define PTHREAD_KEYS_MAX              64
#define PTHREAD_DESTRUCTOR_ITERATIONS 32

enum{
	__PT_TLS_SHARED = 0,
	__PT_TLS_ERRNO,
	__PT_TLS_COND_WAIT,
	__PT_TLS_LOCAL_HEAP,
	__PT_TLS_FORK_FLAGS,
	__PT_TLS_FORK_CPUID,
	__PT_TLS_VALUES_NR
};

#define __pthread_tls_set(tls,index,val)
#define __pthread_tls_get(tls,index)
#define __pthread_tls_getlocation(tls,index)

typedef struct __pthread_tls_s
{
	int signature;
	pthread_attr_t attr;
	void *values_tbl[PTHREAD_KEYS_MAX];
	uint_t tls_tbl[__PT_TLS_VALUES_NR];
}__pthread_tls_t;

void __pthread_init(void);
void __pthread_tls_init(__pthread_tls_t *tls);
void __pthread_tls_destroy(void);
void __pthread_keys_init(void);
void __pthread_keys_destroy(void);
void __pthread_barrier_init(void);

#define __pthread_tls_seterrno(tls,errno)

#define __PTHREAD_OBJECT_CREATED   0xA5B5
#define __PTHREAD_OBJECT_DESTROYED 0xB0A0B0A0
#define __PTHREAD_OBJECT_BUSY      0x5A5A5A5A
#define __PTHREAD_OBJECT_FREE      0xC0A5C0A5

typedef unsigned long pthread_t;
typedef unsigned long pthread_rwlock_t;
typedef unsigned long pthread_rwlockattr_t;
typedef unsigned long pthread_key_t;

typedef struct
{ 
	uint_t val __CACHELINE;
}pthread_spinlock_t;

#define PTHREAD_MUTEX_NORMAL          0
#define PTHREAD_MUTEX_RECURSIVE       1
#define PTHREAD_MUTEX_ERRORCHECK      2
#define PTHREAD_MUTEX_DEFAULT         PTHREAD_MUTEX_NORMAL

typedef struct
{
	int  scope;
	uint_t  type;
	uint_t  cntr;
}pthread_mutexattr_t;

typedef struct 
{
	sem_t sem;
	pthread_spinlock_t lock;
	volatile uint_t value    __CACHELINE;
	uint_t waiting           __CACHELINE;
	struct list_entry queue  __CACHELINE;
	pthread_mutexattr_t attr __CACHELINE;
}pthread_mutex_t;

#define __MUTEX_INITIALIZER(_t)						\
	{							        \
		.sem     = 0,						\
		.lock    = {.val = __PTHREAD_OBJECT_FREE},		\
		.value   = __PTHREAD_OBJECT_FREE,		        \
		.waiting = 0,				                \
		.queue   = {.next = 0, .pred = 0},		        \
		.attr    = {.type = (_t), .scope = PTHREAD_PROCESS_PRIVATE, .cntr = 0}\
	}

#define PTHREAD_MUTEX_INITIALIZER               __MUTEX_INITIALIZER(PTHREAD_MUTEX_DEFAULT)
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER     __MUTEX_INITIALIZER(PTHREAD_MUTEX_RECURSIVE)
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER    __MUTEX_INITIALIZER(PTHREAD_MUTEX_ERRORCHECK)

typedef struct
{
	int scope;
	void *sysid;
	pthread_spinlock_t lock;
	uint_t count             __CACHELINE;
	struct list_entry queue  __CACHELINE;
}pthread_cond_t;

#define PTHREAD_COND_INITIALIZER					\
	{								\
		.scope = PTHREAD_PROCESS_PRIVATE,			\
		.lock  = {.val = __PTHREAD_OBJECT_FREE},		\
		.count = 0,     					\
		.queue = {.next = 0, .pred = 0}         		\
	}

typedef struct
{
	int lock;
	int state;
}pthread_once_t;

#define PTHREAD_BARRIER_SERIAL_THREAD 1

typedef struct
{
	sint_t scope;
}pthread_barrierattr_t;

typedef struct
{
	sint_t scope;
}pthread_condattr_t;

typedef struct 
{
	int scope;
	void *sysid;
   
	__cacheline_t cntr;
	__cacheline_t count;

	volatile __cacheline_t state[2];
  
	union {
		uint_t phase;
		__cacheline_t pading;
	};
}pthread_barrier_t;

#define PTHREAD_ONCE_INIT {.lock = 0, .state = 0}

/** Pthread Attribute Modifiers */
int  pthread_attr_init (pthread_attr_t *attr);
int  pthread_attr_destroy(pthread_attr_t *attr);
int  pthread_attr_setstacksize(pthread_attr_t *attr, unsigned int stacksize);
int  pthread_attr_getstacksize(pthread_attr_t *attr, size_t *stacksize);
int  pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize);
int  pthread_attr_getstack(pthread_attr_t *attr, void **stackaddr, size_t *stacksize);
int  pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int  pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
int  pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);
int  pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy);
int  pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param);
int  pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param);
int  pthread_attr_setinheritsched(pthread_attr_t *attr, int inherit);
int  pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inherit);
int  pthread_attr_setscope(pthread_attr_t *attr, int scope);

/** Thread Management */
void pthread_exit(void *retval);
int  pthread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);
int  pthread_join(pthread_t th, void **thread_return);
int  pthread_detach(pthread_t thread);
int  pthread_equal(pthread_t thread1,pthread_t thread2);
int  pthread_once(pthread_once_t *once_control, void (*init_routine) (void));
void pthread_yield(void);
pthread_t pthread_self(void);

/** Thread specific data */
int pthread_key_create(pthread_key_t *key, void (*destructor)(void*));
int pthread_key_delete(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
void *pthread_getspecific(pthread_key_t key);

/** SpinLock Sync Object */
int  pthread_spin_init(pthread_spinlock_t *lock,int pshared);
int  pthread_spin_lock(pthread_spinlock_t *lock);
int  pthread_spin_trylock(pthread_spinlock_t *lock);
int  pthread_spin_unlock(pthread_spinlock_t *lock);
int  pthread_spin_destroy(pthread_spinlock_t *lock);

/** Mutex Sync Object */
int  pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int  pthread_mutexattr_init(pthread_mutexattr_t *attr);
int  pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared);
int  pthread_mutexattr_getpshared(pthread_mutexattr_t *attr, int *pshared);
int  pthread_mutexattr_gettype(pthread_mutexattr_t *attr, int *type);
int  pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int  pthread_mutex_init (pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int  pthread_mutex_lock(pthread_mutex_t *mutex);
int  pthread_mutex_trylock(pthread_mutex_t *mutex);
int  pthread_mutex_unlock(pthread_mutex_t *mutex);
int  pthread_mutex_destroy(pthread_mutex_t *mutex);

/** Condition Variable Sync Object */
int pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t *cond_attr);
int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared);
int pthread_condattr_getpshared(pthread_condattr_t *attr, int *pshared);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_destroy(pthread_cond_t *cond);

/** Rread/Write Lock Sync Object */
int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

/** Barrier Sync Object */
int pthread_barrierattr_destroy(pthread_barrierattr_t *attr);
int pthread_barrierattr_init(pthread_barrierattr_t *attr);
int pthread_barrierattr_getpshared(pthread_barrierattr_t *attr, int *scope);
int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int scope);
int pthread_barrier_init (pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned count);
int pthread_barrier_wait (pthread_barrier_t *barrier);
int pthread_barrier_destroy(pthread_barrier_t *barrier);


/** POSIX-Like, Not Protable, Additional Operations */
#define PT_TRACE_OFF              0
#define PT_TRACE_ON               1
#define PT_SHOW_STATE             2

#define PT_FORK_DEFAULT           __PT_ATTR_DEFAULT
#define PT_FORK_WILL_EXEC         __PT_FORK_WILL_EXEC
#define PT_FORK_TARGET_CPU        __PT_FORK_USE_TARGET_CPU
#define PT_FORK_SET_AFFINITY      __PT_FORK_USE_AFFINITY
#define PT_ATTR_MEM_PRIO          __PT_ATTR_MEM_PRIO
#define PT_ATTR_AUTO_MGRT         __PT_ATTR_AUTO_MGRT
#define PT_ATTR_AUTO_NXTT         __PT_ATTR_AUTO_NXTT
#define PT_ATTR_INTERLEAVE_SEQ    __PT_ATTR_INTERLEAVE_SEQ
#define PT_ATTR_INTERLEAVE_ALL    __PT_ATTR_INTERLEAVE_ALL
#define PT_ATTR_MEM_CID_RR        __PT_ATTR_MEM_CID_RR


int pthread_migrate_np(pthread_attr_t *attr);
int pthread_profiling_np(int cmd, pid_t pid, pthread_t tid);
int pthread_attr_setflags_np(pthread_attr_t *attr, unsigned int flags, unsigned int *old);
int pthread_attr_setcpuid_np(pthread_attr_t *attr, int cpu_id, int *old_cpu_id);
int pthread_attr_getcpuid_np(int *cpu_id);
int pthread_attr_setforkinfo_np(int flags);
int pthread_attr_setforkcpuid_np(int cpu_id);
int pthread_attr_getforkcpuid_np(int *cpu_id);


extern pthread_mutex_t __printf_lock;
extern uint_t ___dmsg_lock;
extern uint_t ___dmsg_ok;

#define printf_r(...)					\
	do{						\
		pthread_mutex_lock(&__printf_lock);	\
		printf(__VA_ARGS__);			\
		pthread_mutex_unlock(&__printf_lock);	\
	}while(0)

#define fprintf_r(x,...)				\
	do{						\
		pthread_mutex_lock(&__printf_lock);	\
		fprintf((x), __VA_ARGS__);		\
		pthread_mutex_unlock(&__printf_lock);	\
	}while(0)

#define dmsg_r(x,...)						\
	do{							\
		if(___dmsg_ok){					\
			cpu_spinlock_lock(&___dmsg_lock);	\
			fprintf((x), __VA_ARGS__);		\
			cpu_spinlock_unlock(&___dmsg_lock);	\
		}						\
	}while(0)

#define dmsg(x,...)						\
	do{							\
		if(___dmsg_ok){					\
			fprintf((x), __VA_ARGS__);		\
		}						\
	}while(0)


////////////////////////////////////////////////////////////////////
//                       Private Section                          //
////////////////////////////////////////////////////////////////////
#undef __pthread_tls_set
#define __pthread_tls_set(_tls,_indx,_val) do{(_tls)->tls_tbl[(_indx)] = (uint_t)(_val);}while(0)

#undef __pthread_tls_get
#define __pthread_tls_get(_tls,_indx) ((_tls)->tls_tbl[(_indx)])

#undef __pthread_tls_getlocation
#define __pthread_tls_getlocation(_tls,_indx) (&(_tls)->tls_tbl[(_indx)])

#endif	/* _PTHREAD_H_ */
