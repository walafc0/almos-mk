/*
 * __do_interrupt.c - first stage interrupt handler
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

#include <types.h>
#include <kdmsg.h>
#include <device.h>
#include <interrupt.h>
#include <thread.h>
#include <cpu-internal.h>
#include <pic.h>
#include <cpu.h>
#include <cpu-io.h>

extern int __do_syscall (unsigned arg0,
			 unsigned arg1,
			 unsigned arg2,
			 unsigned arg3,
			 unsigned service_num,
			 int *errno);

void __do_interrupt(struct cpu_regs_s *regs)
{
	register uint_t irq = regs->int_nr;
	int err;

#if 0
	isr_dmsg(DEBUG, "DEBUG: IRQ %d has been recived\n", irq);
#endif

	if(irq > PIC_IRQ_MAX)
		PANIC("Interrupt %d", irq);
	else
	{
		if(irq == PIC_IRQ_MAX)
		{
#if 0
			isr_dmsg(DEBUG, "DEBUG:syscall: %x, %x, %x, %x #%d\n",
				 regs->ebx, regs->ecx, regs->edx, regs->edi, regs->eax);
#endif
			err = 0;
			regs->eax = __do_syscall(regs->ebx, regs->ecx, regs->edx, regs->edi, regs->eax, &err);
			regs->esi = err;
			return;
		}

		//isr_dmsg(DEBUG, "DEBUG: [ irq %d ], thread %x\n",irq,current_thread());
		do_interrupt(current_thread(), irq);

		if(irq < 8)
			cpu_io_out8(0x20, 0x20);
		else
			if(irq < 16)
				cpu_io_out8(0xA0, 0x20);
	}
}
