/*
 * cpu_do_syscall.c - first syscall handler
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

#include <errno.h>
#include <syscall.h>
#include <cpu.h>
#include <pmm.h>
#include <thread.h>
#include <task.h>
#include <interrupt.h>
#include <kdmsg.h>
#include <cpu-regs.h>

extern reg_t do_syscall (reg_t arg0,
			 reg_t arg1,
			 reg_t arg2,
			 reg_t arg3,
			 reg_t service_nr);


static void sys_sigreturn(void)
{
	register struct thread_s *this;
	register uint_t uzone_src;
	register error_t err;
 
	this = current_thread;
 
	uzone_src  = this->pws.sigstack_top;
	uzone_src -= sizeof(this->uzone);
  
	err = cpu_copy_from_uspace(&this->uzone, (void*)uzone_src, sizeof(this->uzone));

	if(err)
	{
		printk(WARNING, "WARNING: %s: CPU %d, Task %d, Thread %d (%x) [ KILLED ]\n", 
		       __FUNCTION__, 
		       cpu_get_id(), 
		       this->task->pid, 
		       this->info.order,
		       this);

		sys_thread_exit((void*)EINTR);
	}

	if(current_cpu->fpu_owner == this)
	{
		current_cpu->fpu_owner = NULL;
		cpu_fpu_disable();
	}

	thread_clear_signaled(this);

	this->uzone.regs[V0]   = this->info.retval;
	this->uzone.regs[V1]   = this->info.errno;
	this->uzone.regs[EPC] += 4;
}

//return this
struct thread_s* cpu_do_syscall(reg_t *regs_tbl)
{
	register reg_t arg0;
	register reg_t arg1;
	register reg_t arg2;
	register reg_t arg3;
	register reg_t service_nr;
	register reg_t retval;
	register struct thread_s *this;
  
	service_nr = regs_tbl[V0];

	if(service_nr == SYS_SIGRETURN)
	{
		sys_sigreturn();
		return current_thread;
	}

	arg0 = regs_tbl[A0];
	arg1 = regs_tbl[A1];
	arg2 = regs_tbl[A2];
	arg3 = regs_tbl[A3];
 
	retval = do_syscall(arg0, arg1, arg2, arg3, service_nr);
	this   = current_thread;
 
	this->uzone.regs[V0]   = retval;
	this->uzone.regs[V1]   = this->info.errno;
	this->uzone.regs[EPC] += 4;

	return this;
}
