/*
 * __do_exception.c - first stage exception handler
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
#include <cpu.h>
#include <cpu-internal.h>

static char *exception_name[32] = {"Division By Zero Exception",
				   "Debug Exception",
				   "Non Maskable Interrupt Exception",
				   "Breakpoint Exception",
				   "Into Detected Overflow Exception",
				   "Out of Bounds Exception",
				   "Invalid Opcode Exception",
				   "Coprocessor Exception",
				   "Double Fault Exception",
				   "Coprocessor Segment Overrun Exception",
				   "Bad TSS Exception",
				   "Segment Not Present Exception",
				   "Stack Fault Exception",
				   "General Protection Fault Exception",
				   "Page Fault Exception",
				   "Unknown Interrupt Exception",
				   "Coprocessor Fault Exception",
				   "Alignment Check Exception (486+)",
				   "Machine Check Exception (Pentium/586+)",
				   "Resrved Exception",
				   "Resrved Exception",
				   "Resrved Exception",
				   "Resrved Exception",
				   "Resrved Exception",
				   "Resrved Exception",
				   "Resrved Exception",
				   "Resrved Exception",
				   "Resrved Exception",
				   "Resrved Exception",
				   "Resrved Exception",
				   "Resrved Exception",
				   "Resrved Exception"};

void __do_exception(struct cpu_regs_s *regs)
{ 

	cpu_spinlock_lock(&exception_lock.val);
 
	except_dmsg("Exception has occured [ %s ]\n", exception_name[regs->int_nr]);
	except_dmsg("ret_cs %x\tret_eip %x\teflags %x\told_ss %x\told_esp %x\t err_code %x\n",
		    regs->ret_cs,regs->ret_eip,regs->eflags, regs->old_ss, regs->old_esp, regs->err_code);
  
	except_dmsg("gs %x\tfs %x\tes %x\tds %x\n", regs->gs, regs->es, regs->ds);

	except_dmsg("ebp %x\tebx %x\tedx %x\tecx %x\teax %x\n",
		    regs->ebx, regs->edx, regs->ecx, regs->eax);

	cpu_spinlock_unlock(&exception_lock.val);
  
	while(1);
}
