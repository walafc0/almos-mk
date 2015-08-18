/*
 * kern/do_syscall.c - kernel unified syscall entry-point
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

#include <syscall.h>
#include <errno.h>
#include <time.h>
#include <cpu.h>
#include <dma.h>
#include <thread.h>
#include <task.h>
#include <scheduler.h>
#include <kmem.h>
#include <kdmsg.h>
#include <sys-vfs.h>
#include <cpu-trace.h>
#include <semaphore.h>
#include <cond_var.h>
#include <barrier.h>
#include <rwlock.h>
#include <vmm.h>
#include <signal.h>
#include <page.h>

static int sys_notAvailable()
{
	printk(WARNING, "WARNING: User Asked a vaild Syscall NR but found no corresponding serice\n");
	return ENOSYS;
}

typedef int (*sys_func_t) ();

static const sys_func_t sys_call_tbl[__SYS_CALL_SERVICES_NUM] = {
	sys_thread_exit,
	sys_mmap,
	sys_thread_create,
	sys_thread_join,
	sys_thread_detach,
	sys_thread_yield,
	sys_sem,
	sys_cond_var,
	sys_barrier,
	sys_rwlock,
	sys_thread_sleep,
	sys_thread_wakeup,
	sys_open,
	sys_creat,
	sys_read,
	sys_write,
	sys_lseek,
	sys_close,
	sys_unlink,
	sys_pipe,
	sys_chdir,
	sys_mkdir,
	sys_mkfifo,
	sys_opendir,
	sys_readdir,
	sys_closedir,
	sys_getcwd,
	sys_clock,
	sys_alarm,
	sys_dma_memcpy,
	sys_utls,
	sys_notAvailable,		/* Reserved for sigreturn */
	sys_signal,
	sys_sigreturn_setup,
	sys_kill,
	sys_getpid,
	sys_fork,
	sys_exec,
	sys_thread_getattr,
	sys_ps,
	sys_madvise,
	sys_mcntl,
	sys_stat,
	sys_thread_migrate,
	sys_sbrk,
	sys_rmdir,
	sys_ftime,
	sys_chmod,
	sys_fsync
};

reg_t do_syscall (reg_t arg0,
		  reg_t arg1,
		  reg_t arg2,
		  reg_t arg3,
		  reg_t service_num)
  
{
	register int return_val, ret;
	register struct thread_s *this;
	struct cpu_s *cpu;
	
	return_val  = 0;
	this        = current_thread;
	this->state = S_KERNEL;
	cpu         = thread_current_cpu(this);
	
	tm_usr_compute(this);

	cpu_trace_write(current_cpu, do_syscall);

	if(thread_migration_isActivated(this))
	{
		cpu_enable_all_irq(NULL);
		ret = thread_migrate(this,-1);
		cpu_disable_all_irq(NULL);

		/* this pointer is expired */
		this = current_thread;
		cpu_wbflush();

		if(ret == 0)
			thread_migration_deactivate(this);
	}

	return_val = cpu_context_save(&this->info.pss);
   
	if(return_val != 0)
	{
		/* Reload these pointers as the task may be forked */
		this = current_thread;
		cpu  = current_cpu;
		cpu_wbflush();
		/* ----------------------------------------------- */
		
		return_val = this->info.retval;
		goto END_DO_SYSCALL;
	}

	cpu_enable_all_irq(NULL);
  
	if(service_num >= __SYS_CALL_SERVICES_NUM)
	{
		printk(INFO, "INFO: %s: Unknow requested service %d, thread %x, proc %d\n",
		       __FUNCTION__, 
		       service_num, 
		       this, 
		       cpu->gid);

		this->info.errno = ENOSYS;
		goto ABORT_DO_SYSCALL;
	}

	if(this->info.isTraced == true)
	{
		printk(DEBUG, "DEBUG: %s: Pid %d, Tid %d (%x), CPU %d, Service #%d\n  Arg0 %x, Arg1 %x, Arg2 %x, Arg3 %x\n",
		       __FUNCTION__, 
		       this->task->pid,
		       this->info.order,
		       this,
		       cpu->gid, 
		       service_num, 
		       arg0, 
		       arg1, 
		       arg2, 
		       arg3);
	}

	this->info.errno = 0;
	return_val = sys_call_tbl[service_num] (arg0,arg1,arg2,arg3);

	/* Reload these pointers as the thread may be migrated */
	this = current_thread;
	cpu  = current_cpu;
	cpu_wbflush();
	/* ----------------------------------------------- */
   
END_DO_SYSCALL:
	
	cpu_disable_all_irq(NULL);
   
	/* FIXME: Can we delete (safely) this block */
	if(event_is_pending(&cpu->le_listner))
		event_listner_notify(&cpu->le_listner);

	cpu_yield();

	if(thread_migration_isActivated(this))
	{
		thread_clear_cap_migrate(this);
		cpu_enable_all_irq(NULL);
		ret = thread_migrate(this,-1);
		cpu_disable_all_irq(NULL);

		/* this pointer is expired */
		this = current_thread;
		cpu_wbflush();

		if(ret == 0)
		{
			thread_migration_deactivate(this);
		}
		else
		{

			isr_dmsg(INFO, "%s: cpu %d, migration failed for victim pid %d, tid %d, err %d\n", 
				 __FUNCTION__, 
				 cpu_get_id(),
				 this->task->pid,
				 this->info.order,
				 ret);
		}
	}
	
ABORT_DO_SYSCALL:
	if(this->info.isTraced == true)
	{
		printk(DEBUG, "DEBUG: %s: Pid %d, Tid %d (%x), CPU %d, Service #%d, Return %x, Error %d\n",
		       __FUNCTION__, 
		       this->task->pid,
		       this->info.order,
		       this,
		       cpu->gid, 
		       service_num, 
		       return_val, 
		       this->info.errno);
	}

	tm_sys_compute(this);
	this->info.retval = return_val;
	this->state = S_USR;
	signal_notify(this);

	return return_val;
}
