/*
 * kern/thread.h - the definition of a thread and its related operations
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

#ifndef _THREAD_H_
#define _THREAD_H_

#include <types.h>
#include <list.h>
#include <spinlock.h>
#include <wait_queue.h>
#include <cpu.h>

typedef enum
{   
	PTHREAD = 0,
	KTHREAD,
	TH_IDLE,
	TH_BOOT,
	THREAD_TYPES_NR
} thread_attr_t;

typedef enum
{
	S_CREATE = 0,
	S_USR,
	S_KERNEL,
	S_READY,
	S_WAIT,
	S_DEAD,
	THREAD_STATES_NR
} thread_state_t;

const char* const thread_state_name[THREAD_STATES_NR];
const char* const thread_attr_name[THREAD_TYPES_NR];

struct cpu_desc_s;
struct sched_s;
struct list_entry;
struct task_s;
struct event_s;

#define PT_ATTR_DEFAULT             0x000
#define PT_ATTR_DETACH              0x001 /* for compatiblity */
#define PT_FORK_WILL_EXEC           0x001 /* for compatiblity */
#define PT_FORK_USE_TARGET_CPU      0x002 /* for compatiblity */
#define PT_ATTR_LEGACY_MASK         0x003 /* TODO: remove legacy attr issue*/

#define PT_FORK_USE_AFFINITY        0x004
#define PT_ATTR_MEM_PRIO            0x008
#define PT_ATTR_INTERLEAVE_SEQ      0x010
#define PT_ATTR_INTERLEAVE_ALL      0x020
#define PT_ATTR_AUTO_MGRT           0x040
#define PT_ATTR_AUTO_NXTT           0x080
#define PT_ATTR_MEM_CID_RR          0x100

/** 
 * Pthread attributes
 * Mandatory members must be set before 
 * initializing CPU new context.
 */
typedef struct
{
	uint_t key;
	uint_t flags;
	uint_t sched_policy;
	uint_t inheritsched;
	void *stack_addr;
	size_t stack_size;
	void *entry_func;                  /* mandatory */
	void *exit_func;                   /* mandatory */
	void *arg1;
	void *arg2;
	void *sigreturn_func;
	void *sigstack_addr;
	size_t sigstack_size;
	struct sched_param  sched_param;
	cid_t cid;
	sint_t cpu_lid;
	sint_t cpu_gid;
	uint_t tid;			   /* mandatory */
	uint_t pid;                        /* mandatory */
} pthread_attr_t;

typedef void before_sleep_t(struct thread_s *thread);
typedef void after_wakeup_t(struct thread_s *thread);

/** 
 * Thread additional informations
 */
struct thread_info
{
	//struct mcs_mailbox_s mailbox;
	uint_t pgfault_cntr;
	uint_t remote_pages_cntr;
	uint_t spurious_pgfault_cntr;
	uint_t sched_nr;
	uint_t ppm_last_cid;
	uint_t migration_fail_cntr;
	uint_t migration_cntr;
	bool_t isTraced;
	uint_t u_err_nr;
	uint_t m_err_nr;
	uint_t tm_exec;
	uint_t tm_tmp;                      /*! temporary date to compute execution duration */
	uint_t tm_usr;                      /*! user execution duration */
	uint_t tm_sys;                      /*! system execution duration */
	uint_t tm_sleep;                    /*! sleeping duration */
	uint_t tm_wait;
	error_t errno;                      /*! errno value of last system call */
	error_t retval;                     /*! return value of last system call */
	uint_t sig_state;
	uint_t sig_mask;
	struct cpu_s *ocpu;
	uint_t wakeup_date;                 /*! wakeup date in seconds */
	void *exit_value;                   /*! exit value returned by thread or joined one */
	struct thread_s *join;              /*! points to waiting thread in join case */
	struct thread_s *waker;
	struct wait_queue_s wait_queue;
	struct wait_queue_s *queue;
	before_sleep_t *before_sleep;	    /*! called before sleep with interuption disabled */
	after_wakeup_t *after_wakeup;	    /*! called after wakeup with interuption disabled  */
	struct cpu_context_s pss;	    /*! Processor Saved State */
	uint_t tm_create;                   /*! date of the creation */
	uint_t tm_born;                     /*! date of the thread loading */
	uint_t tm_dead;                     /*! date of the death */
	uint_t order;
	uint_t usr_tls;
	pthread_attr_t attr;
	void  *kstack_addr;
	uint_t kstack_size;
	struct event_s *e_info;
	struct page_s *page;
};


/**
 * The Thread Descriptor
 */
struct thread_s
{
	struct cpu_uzone_s uzone;     /*! User related info, this offset is frozen */
	spinlock_t lock;
	thread_state_t state;
	uint_t flags;                 /*! TH_NEED_TO_SCHED, .. */
	uint_t joinable;	  
	error_t locks_count;	      /*! number of locks which are locked by the current thread */
	error_t distlocks_count;      /*! number of locks which are locked by the current thread */
	sint_t quantum;               /*! number of clock ticks given to the thread */
	uint_t ticks_nr;              /*! number of ticks used */
	uint_t skip_check;            /*! skip check once on two */
	uint_t static_prio;
	uint_t dynamic_prio;
	sint_t boosted_prio;
	uint_t prio;
	uint_t ltid;
	struct cpu_s *lcpu;           /*! pointer to the local CPU description structure */
	struct sched_s *local_sched;  /*! pointer to the local scheduler structure */
	struct rpc_listner_s *li;
	struct task_s *task;
	thread_attr_t type;           /*! 3 types : usr (PTHREAD), kernel (KTHREAD) or idle (TH_IDLE) */
	struct cpu_context_s pws;     /*! processor work state (register saved zone) */
	struct list_entry list;       /*! next/pred threads at the same state */
	struct list_entry rope;       /*! next/pred threads in the __rope list of thread */
	struct thread_info info;      /*! (exit value, statistics, ...) */
	uint_t signature;
};

/* Thread Attributes */
#define thread_isJoinable(thread)   
#define thread_set_joinable(thread) 
#define thread_clear_joinable(thread)
#define thread_sched_activate(thread)
#define thread_sched_isActivated(thread)
#define thread_sched_deactivate(thread)
#define thread_migration_activate(thread)
#define thread_migration_isActivated(thread)
#define thread_migration_deactivate(thread)
#define thread_migration_disable(thread)
#define thread_migration_enable(thread)
#define thread_migration_disabled(thread)
#define thread_migration_enabled(thread)
#define thread_migration_isEnabled(thread)
#define thread_set_cap_migrate(thread)
#define thread_clear_cap_migrate(thread)
#define thread_isCapMigrate(thread)
#define thread_set_exported(thread)
#define thread_clear_exported(thread)
#define thread_isExported(thread)
#define thread_set_imported(thread)
#define thread_clear_imported(thread)
#define thread_isImported(thread)
#define thread_preempt_disable(thread)
#define thread_preempt_enable(thread)
#define thread_locks_count_up(thread)
#define thread_locks_count_down(thread)
#define thread_isPreemptable(thread)
#define thread_set_forced_yield(thread)
#define thread_clear_forced_yield(thread)
#define thread_isForcedYield(thread)
#define thread_isWakeable(thread)
#define thread_isCapWakeup(thread)
#define thread_set_wakeable(thread)
#define thread_set_cap_wakeup(thread)
#define thread_clear_wakeable(thread)
#define thread_set_no_vmregion(thread)
#define thread_has_vmregion(thread)
#define thread_isBootStrap(thread)
#define thread_set_signaled(thread)
#define thread_clear_signaled(thread)
#define thread_isSignaled(thread)
#define thread_isStack_overflow(thread)

/* Currents task, thread, cluster, cpu  */
#define current_task
#define current_thread
#define current_cpu
#define origin_cpu
#define current_cluster
#define current_cid
#define origin_cluster

/* Thread's current & origin cpu, cluster */
#define thread_current_cpu(thread)
#define thread_current_cluster(thread)
#define thread_origin_cpu(thread)
#define thread_origin_cluster(thread)
#define thread_set_current_cpu(thread,cpu)
#define thread_set_origin_cpu(thread,cpu)

struct kthread_args_s
{
	union
	{
		uint_t val[6];
		cacheline_t pad;
	};
};

typedef struct kthread_args_s kthread_args_t;
typedef void* (kthread_t) (void *);

int sys_thread_create (pthread_t *tid, pthread_attr_t *thread_attr);
int sys_thread_join (pthread_t tid, void **thread_return);
int sys_thread_detach (pthread_t tid);
int sys_thread_getattr(pthread_attr_t *attr);
int sys_thread_exit (void *exit_val);
int sys_thread_migrate(pthread_attr_t *thread_attr);
int sys_thread_yield();
int sys_thread_sleep();
int sys_thread_wakeup(pthread_t tid, pthread_t *tid_tbl, uint_t count);
int sys_utls(uint_t operation, uint_t value);

void* thread_idle(void *arg);

error_t thread_create(struct task_s *task, 
		      pthread_attr_t *attr, 
		      struct thread_s **new_thread);

struct thread_s* kthread_create(struct task_s *task, 
				kthread_t *kfunc, 
				void *arg, 
				uint_t cpu_lid);

void kthread_destroy(struct thread_s *thread);

error_t thread_dup(struct task_s *task, 
		   struct thread_s *dst,
		   struct cpu_s *dst_cpu,
		   struct cluster_s *dst_clstr,
		   struct thread_s *src);

/* Migration can be done only when the thread is not holding	*
 * private pointers to the data of a cluster. Typically it can	*
 * be done when jumping from kernel space to user space. At the *
 * of a syscall or an interrupt (if we are going back to user). */
error_t thread_migrate(struct thread_s *thread, sint_t cpu_gid);
error_t thread_destroy(struct thread_s *thread);

EVENT_HANDLER(thread_destroy_handler);

//////////////////////////////////////////////////////////////////////
///                       Private Section                          ///
//////////////////////////////////////////////////////////////////////

#define TH_NEED_TO_SCHED    0x001
#define TH_JOINABLE         0x002
#define TH_NO_VM_REGION     0x004
#define TH_CAN_WAKEUP       0x008
#define TH_CAP_WAKEUP       0x010
#define TH_NEED_TO_MIGRATE  0x020
#define TH_CAN_MIGRATE      0x040
#define TH_CAP_MIGRATE      0x080
#define TH_EXPORTED         0x100
#define TH_IMPORTED         0x200
#define TH_FORCED_YIELD     0x400
#define TH_DOING_SIGNAL     0x800

/* Thread Attributes */
#undef thread_isJoinable
#undef thread_set_joinable
#undef thread_clear_joinable
#undef thread_sched_activate
#undef thread_sched_deactivate
#undef thread_sched_isActivated
#undef thread_migration_activate
#undef thread_migration_deactivate
#undef thread_migration_isActivated
#undef thread_migration_disable
#undef thread_migration_enable
#undef thread_migration_isEnabled
#undef thread_migration_disabled
#undef thread_migration_enabled
#undef thread_set_cap_migrate
#undef thread_clear_cap_migrate
#undef thread_isCapMigrate
#undef thread_set_exported
#undef thread_clear_exported
#undef thread_isExported
#undef thread_set_imported
#undef thread_clear_imported
#undef thread_isImported
#undef thread_isPreemptable
#undef thread_set_forced_yield
#undef thread_clear_forced_yield
#undef thread_isForcedYield
#undef thread_preempt_disable
#undef thread_preempt_enable
#undef thread_locks_count_down
#undef thread_locks_count_up
#undef thread_isWakeable
#undef thread_isCapWakeup
#undef thread_set_wakeable
#undef thread_set_cap_wakeup
#undef thread_clear_wakeable
#undef thread_set_no_vmregion
#undef thread_has_vmregion
#undef thread_isBootStrap
#undef thread_set_signaled
#undef thread_clear_signaled
#undef thread_isSignaled
#undef thread_isStack_overflow

#define thread_isJoinable(_th)         ((_th)->joinable != 0)
#define thread_set_joinable(_th)     do{(_th)->joinable = 1;}while(0)
#define thread_clear_joinable(_th)   do{(_th)->joinable = 0;}while(0)

#define thread_sched_activate(_th)   do{(_th)->flags |= TH_NEED_TO_SCHED;}while(0)
#define thread_sched_deactivate(_th) do{(_th)->flags &= ~TH_NEED_TO_SCHED;}while(0)
#define thread_sched_isActivated(_th) ((_th)->flags & TH_NEED_TO_SCHED)

#define thread_migration_activate(_th)   do{(_th)->flags |= TH_NEED_TO_MIGRATE;}while(0)
#define thread_migration_deactivate(_th) do{(_th)->flags &= ~TH_NEED_TO_MIGRATE;}while(0)
#define thread_migration_isActivated(_th) ((_th)->flags & TH_NEED_TO_MIGRATE)

#define thread_set_cap_migrate(_th) do{(_th)->flags |= TH_CAP_MIGRATE;}while(0)
#define thread_isCapMigrate(_th)  (((_th)->flags & TH_CAP_MIGRATE) && ((_th)->flags & TH_CAN_MIGRATE))
#define thread_clear_cap_migrate(_th) do{(_th)->flags &= ~TH_CAP_MIGRATE;}while(0)

#define thread_migration_disabled(_th) do{(_th)->flags &= ~TH_CAN_MIGRATE;}while(0)
#define thread_migration_enabled(_th)  do{(_th)->flags |= TH_CAN_MIGRATE;}while(0)
#define thread_migration_isEnabled(_th) ((_th)->flags & TH_CAN_MIGRATE)

#define thread_migration_disable(_th)				\
	do{{							\
			uint_t irq_state;			\
			cpu_disable_all_irq(&irq_state);	\
			(_th)->flags &= ~TH_CAN_MIGRATE;	\
			cpu_restore_irq(irq_state);		\
	  }}while(0)

#define thread_migration_enable(_th)				\
	do{{							\
			uint_t irq_state;			\
			cpu_disable_all_irq(&irq_state);	\
			(_th)->flags |= TH_CAN_MIGRATE;		\
			cpu_restore_irq(irq_state);		\
	  }}while(0)


#define thread_set_exported(_th)				\
	do{{							\
			uint_t irq_state;			\
			cpu_disable_all_irq(&irq_state);	\
			(_th)->flags |= TH_EXPORTED;		\
			cpu_restore_irq(irq_state);		\
	  }}while(0)

#define thread_clear_exported(_th) do{(_th)->flags &= ~TH_EXPORTED;}while(0)

#if 0
#define thread_clear_exported(_th)				\
	do{{							\
			uint_t irq_state;			\
			cpu_disable_all_irq(&irq_state);	\
			(_th)->flags &= ~TH_EXPORTED;		\
			cpu_restore_irq(irq_state);		\
	  }}while(0)
#endif

#define thread_isExported(_th)  ((_th)->flags & TH_EXPORTED)

#define thread_set_imported(_th) do{(_th)->flags |= TH_IMPORTED;}while(0)
#define thread_clear_imported(_th) do{(_th)->flags &= ~TH_IMPORTED;}while(0)
#define thread_isImported(_th)  ((_th)->flags & TH_IMPORTED)

#define thread_isPreemptable(_th)     (((_th)->locks_count == 0) && ((_th)->distlocks_count == 0))

#define thread_locks_count_up(_th)				\
	do{{							\
			uint_t irq_state;			\
			cpu_disable_all_irq(&irq_state);	\
			(_th)->locks_count ++;			\
			cpu_restore_irq(irq_state);		\
	  }}while(0)

#define thread_preempt_disable(_th) thread_locks_count_up(_th)

#define thread_locks_count_down(_th)				\
	do{{							\
			uint_t irq_state;			\
			cpu_disable_all_irq(&irq_state);	\
			assert((_th)->locks_count>0);		\
			(_th)->locks_count --;			\
			cpu_restore_irq(irq_state);		\
	  }}while(0)

#define thread_preempt_enable(_th)	thread_locks_count_down(_th)

#define thread_set_forced_yield(_th)   do{(_th)->flags |= TH_FORCED_YIELD;}while(0)
#define thread_clear_forced_yield(_th) do{(_th)->flags &= ~TH_FORCED_YIELD;}while(0)
#define thread_isForcedYield(_th)      ((_th)->flags & TH_FORCED_YIELD)

#define thread_isWakeable(_th)         ((_th)->flags & TH_CAN_WAKEUP)
#define thread_isCapWakeup(_th)        ((_th)->flags & TH_CAP_WAKEUP)

#define thread_set_cap_wakeup(_th)				\
	do{{							\
			uint_t irq_state;			\
			cpu_disable_all_irq(&irq_state);	\
			(_th)->flags |= TH_CAP_WAKEUP;		\
			cpu_restore_irq(irq_state);		\
	  }}while(0)

#define thread_set_wakeable(_th)		\
	do{					\
		(_th)->flags |= TH_CAN_WAKEUP;	\
		(_th)->flags &= ~TH_CAP_WAKEUP;	\
	}while(0)

#define thread_clear_wakeable(_th)		\
	(_th)->flags &= ~TH_CAN_WAKEUP


#define thread_set_no_vmregion(_th) do{(_th)->flags |= TH_NO_VM_REGION;}while(0)
#define thread_has_vmregion(_th)    (((_th)->flags & TH_NO_VM_REGION) == 0)

#define thread_isBootStrap(_th) ((_th)->type == TH_BOOT)

#define thread_set_signaled(_th) do{(_th)->flags |= TH_DOING_SIGNAL;}while(0)
#define thread_clear_signaled(_th) do{(_th)->flags &= ~TH_DOING_SIGNAL;}while(0)
#define thread_isSignaled(_th) ((_th)->flags & TH_DOING_SIGNAL)

#define thread_isStack_overflow(_th) ((_th)->signature != THREAD_ID)

/* Currents task, thread, cluster, cpu  */
#undef current_task
#undef current_thread
#undef current_cpu
#undef origin_cpu
#undef current_cluster
#undef current_cid
#undef origin_cluster
#define current_task    (cpu_current_thread()->task)
#define current_thread  (cpu_current_thread())
#define current_cpu     (cpu_current_thread()->lcpu)
#define origin_cpu      (cpu_current_thread()->info.ocpu)
#define current_cluster (&cluster_manager) //(current_cpu->cluster)
#define current_cid	(cpu_get_cid()) //(current_cluster->id)
#define origin_cluster  (origin_cpu->cluster)

/* Thread's current & origin cpu, cluster */
#undef thread_current_cpu
#undef thread_current_cluster
#undef thread_origin_cpu
#undef thread_origin_cluster
#undef thread_set_current_cpu
#undef thread_set_origin_cpu
#define thread_current_cpu(_th)            ((_th)->lcpu)
#define thread_current_cluster(_th)        ((_th)->lcpu->cluster)
#define thread_origin_cpu(_th)             ((_th)->info.ocpu)
#define thread_origin_cluster(_th)         ((_th)->info.ocpu->cluster)
#define thread_set_current_cpu(_th,_cpu)   do{(_th)->lcpu = (_cpu);}while(0)
#define thread_set_origin_cpu(_th,_cpu)    do{(_th)->info.ocpu = (_cpu);}while(0)

#endif	/* _THREAD_H_ */
