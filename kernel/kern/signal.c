/*
 * kern/signal.c - signal-management related operations
 * 
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012,2013,2014,2015 UPMC Sorbonne Universites
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

#include <types.h>
#include <errno.h>
#include <utils.h>
#include <signal.h>
#include <thread.h>
#include <task.h>
#include <pid.h>
#include <cpu.h>

SIGNAL_HANDLER(kill_sigaction)
{
	struct thread_s *this;
  
	this = current_thread;
	this->state = S_KERNEL;

	printk(INFO, "INFO: Recieved signal %d, pid %d, tid %x, core %d  [ KILLED ]\n",
	       sig,
	       this->task->pid,
	       this,
	       cpu_get_id());

	sys_thread_exit((void*)EINTR);
}

error_t signal_manager_init(struct task_s *task)
{
	memset(&task->sig_mgr, 0, sizeof(task->sig_mgr));
	task->sig_mgr.sigactions[SIGCHLD] = SIG_IGNORE;
	task->sig_mgr.sigactions[SIGURG]  = SIG_IGNORE;
	return 0;
}

error_t signal_init(struct thread_s *thread)
{
	thread->info.sig_state = 0;
	thread->info.sig_mask  = current_thread->info.sig_mask;
	return 0;
}

static error_t signal_rise_all(struct task_s *task, uint_t sig)
{
	struct list_entry *iter;
	struct thread_s *thread;
	uint_t count;

	count = 0;

	spinlock_lock(&task->th_lock);

	list_foreach(&task->th_root, iter)
	{
		thread = list_element(iter, struct thread_s, rope);

		spinlock_lock(&thread->lock);
		thread->info.sig_state |= (1 << sig);
		spinlock_unlock(&thread->lock);

		count ++;
	}

	spinlock_unlock(&task->th_lock);

	sig_dmsg(1, "%s: pid %d, %d threads has been signaled\n",       \
                        __FUNCTION__, task->pid, count);

	return 0;
}

static error_t signal_rise_one(struct task_s *task, uint_t sig)
{
	struct thread_s *thread;

	spinlock_lock(&task->th_lock);

//mdify to current_thread, not the one pointed by the sig_mgr ?
	if(task->sig_mgr.handler == NULL)
		thread = list_first(&task->th_root, struct thread_s, rope);
	else
		thread = task->sig_mgr.handler;

	spinlock_lock(&thread->lock);
	thread->info.sig_state |= (1 << sig);
	spinlock_unlock(&thread->lock);

	spinlock_unlock(&task->th_lock);

        sig_dmsg(1, "%s: Thread %u of task %u has received signal %u. sig_state = %x\n",        \
                        __FUNCTION__, thread_current_cpu(thread)->gid,                          \
                        task->pid, sig, thread->info.sig_state);

	return 0;
}

RPC_DECLARE( __signal_rise,                             \
                RPC_RET( RPC_RET_PTR(error_t, err)),    \
                RPC_ARG( RPC_ARG_VAL(pid_t,   pid),     \
                         RPC_ARG_VAL(uint_t,  sig))     \
           )
{
        struct task_s   *task;
        struct hnode_s  *hnode;

        /* Avoid killing task0 and init */
        /* FIXME: Zero should not be hard-coded but obtains with something like MAIN_KERN */
        if( ((pid == PID_MIN_GLOBAL) || (pid == PID_MIN_GLOBAL+1))
                        && (current_cid == 0) )
        {
                *err = EPERM;
                sig_dmsg(1, "%s: can't kill task %u on cluster %u\n",           \
                                __FUNCTION__, PID_GET_LOCAL(pid), current_cid);
                goto SYS_RISE_ERR_PID;
        }

        /* Step 1 : lock the task manager */
        tasks_manager_lock();

        /* Step 2 : Get the task' address */
        /* Case 1 : current cluster is the anchor and the owner. */
        if ( PID_GET_CLUSTER(pid) == current_cid )
        {
                sig_dmsg(1, "%s: task %u is in the tasks manager array of cluster  %u\n",       \
                                __FUNCTION__, pid, current_cid);
                task = task_lookup(pid)->task;

        }
        else /* Case 2 : current cluster is not the anchor, so the struct
              * task_s is in its hash table.
              */
        {
                sig_dmsg(1, "%s: task %u is in the tasks manager hash table of cluster  %u\n",  \
                                __FUNCTION__, pid, current_cid);
                hnode = hfind(tasks_manager_get_htable(), (void*)pid);
                task    = (struct task_s*) container_of(hnode,          \
                                struct task_s, t_hnode);
        }

        /* Step 4 : check task' address */
        if ( task == NULL )
        {
                *err = ESRCH;
                goto SYS_RISE_ERR;
        }

        /* Step 5 : deliver signal */
        if((sig == SIGTERM) || (sig == SIGKILL))
                *err = signal_rise_all(task, sig);
        else
                *err = signal_rise_one(task, sig);

        /* Step 6 : unlock tasks manager */
        tasks_manager_unlock();

        return;

SYS_RISE_ERR:
        tasks_manager_unlock();
SYS_RISE_ERR_PID:
        sig_dmsg(1, "%s: Cluster %u has not deliver signal %u to task %u (err %u)\n",  \
                        __FUNCTION__, current_cid, sig, err );

        return;
}

error_t signal_rise(pid_t pid, cid_t location, uint_t sig)
{
	error_t err;

        /* Check location error */
        if ( location == CID_NULL )
        {
                err = ESRCH;
                printk(WARNING, "%s: there is no process with pid %u\n", \
                                __FUNCTION__, pid);
                return err;
        }

        err = EAGAIN;
        RCPC( location, RPC_PRIO_SIG_RISE, __signal_rise,       \
                        RPC_RECV( RPC_RECV_OBJ(err) ),          \
                        RPC_SEND( RPC_SEND_OBJ(pid),            \
                                  RPC_SEND_OBJ(sig))            \
            );

	return err;
}

void signal_notify(struct thread_s *this)
{
	register uint_t sig_state;
	register uint_t sig;
	register struct sig_mgr_s *sig_mgr;
	uint_t irq_state;

	sig_state = this->info.sig_state & this->info.sig_mask;
	sig       = 0;
 
	while((sig_state != 0) && ((sig_state & 0x1) == 0) && (sig < SIG_NR))
	{
		sig ++; 
		sig_state >>= 1;
	}
  
	if(sig)
	{
		cpu_disable_all_irq(&irq_state);

		if(thread_isSignaled(this))
		{
			cpu_restore_irq(irq_state);
			return;
		}

		thread_set_signaled(this);
		cpu_restore_irq(irq_state);

		spinlock_lock(&this->lock);
		this->info.sig_state &= ~(1 << sig);
		spinlock_unlock(&this->lock);

		sig_mgr = &this->task->sig_mgr;

		if(sig_mgr->sigactions[sig] == SIG_IGNORE)
			return;

		if(sig_mgr->sigactions[sig] == SIG_DEFAULT)
			kill_sigaction(sig);

		cpu_signal_notify(this, sig_mgr->sigactions[sig], sig);
	}
}
