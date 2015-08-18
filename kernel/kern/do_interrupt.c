/*
 * kern/do_interrupt.c - kernel unified interrupt entry-point
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

#include <cpu.h>
#include <task.h>
#include <thread.h>
#include <cpu-trace.h>
#include <device.h>
#include <system.h>
#include <signal.h>

#define irq_cpu_dmsg(c,...)				\
	do{						\
		if(cpu_get_id() == (c))			\
			isr_dmsg(INFO, __VA_ARGS__);	\
	}while(0)

void do_interrupt(struct thread_s *this, uint_t irq_num)
{
	register thread_state_t old_state;
	struct irq_action_s *action;
	struct cpu_s *cpu;
	uint_t irq_state;
	error_t ret;

	cpu_trace_write(thread_current_cpu(this), do_interrupt);

	cpu = thread_current_cpu(this);

	cpu->irq_nr ++;
	old_state = this->state;

	if(old_state == S_USR)
	{
		this->state = S_KERNEL;
		tm_usr_compute(this);
	}

	arch_cpu_get_irq_entry(cpu, irq_num, &action);
	action->irq_handler(action);
	   
	cpu_yield();

	if(old_state != S_USR)//and not a kernel thread ?
		return;

	/* it is safe to migrate as we are going to user *
	 * space.					 */
	if(thread_migration_isActivated(this))
	{
		thread_clear_cap_migrate(this);
		cpu_enable_all_irq(&irq_state);
		ret = thread_migrate(this,-1);
		cpu_restore_irq(irq_state);

		/* this pointer has expired */
		this = current_thread;
		cpu_wbflush();

		if(ret == 0)	
			thread_migration_deactivate(this);
		else
		{
			isr_dmsg(INFO, 
			"%s: cpu %d, migration failed for victim pid %d, tid %d, err %d\n", 
			 __FUNCTION__, 
			 cpu_get_id(),
			 this->task->pid,
			 this->info.order,
			 ret);
		}
	}
	
	tm_sys_compute(this);
	this->state = S_USR;
	signal_notify(this);
}
