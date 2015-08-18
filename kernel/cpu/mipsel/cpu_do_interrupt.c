/*
 * cpu_do_interrupt.c - first interrupts handler
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
#include <thread.h>
#include <task.h>
#include <interrupt.h>
#include <kdmsg.h>

#include <cpu-regs.h>

void cpu_do_interrupt(struct thread_s *this, 
		      reg_t cpu_id, 
		      reg_t *regs_tbl, 
		      reg_t irq_state)
{
	register uint_t irq_num = 0;
 
#if CONFIG_SHOW_EPC_CPU0
	if(cpu_id == 0)
		isr_dmsg(INFO,"cpu %d, pid %d, tid %d, state %s, EPC %x, ra %x\n",
			 cpu_id, this->task->pid, this->info.order,
			 thread_state_name[this->state], regs_tbl[EPC], regs_tbl[RA]);
#endif

	if(this->info.isTraced)
		except_dmsg("cpu %d, tid %x, EPC %x, ra %x\n", cpu_id, this, regs_tbl[EPC], regs_tbl[RA]);

	while(irq_state && (irq_num < CPU_IRQ_NR))
	{
		if(irq_state & 0x1)
			do_interrupt(this, irq_num);
		irq_state >>=1; 
		irq_num ++;
	}
}
