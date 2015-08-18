/*
 * arch.c - architecture related operations (see kern/hal-arch.h)
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
#include <types.h>
#include <kdmsg.h>
#include <cpu.h>
#include <device.h>
#include <system.h>

error_t arch_cpu_init(struct cpu_s *cpu)
{
	cpu->arch.action = NULL;
	return 0;
}

error_t arch_cpu_set_irq_entry(struct cpu_s *cpu, int irq_nr, struct irq_action_s *action)
{
	assert(cpu->arch.action == NULL && "IRQ entry has been already specified\n");
	cpu->arch.action = action;
	return 0;
}

error_t arch_cpu_get_irq_entry(struct cpu_s *cpu, int irq_nr, struct irq_action_s **action)
{
	if(cpu->arch.action == NULL)
		return EINVAL;

	cpu->arch.action->data = (void*)irq_nr;
	*action = cpu->arch.action;
	return 0;
}
