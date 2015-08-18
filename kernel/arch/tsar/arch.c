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
#include <thread.h>
#include <cpu.h>
#include <system.h>
#include <kmem.h>
#include <device.h>
#include <cluster.h>
#include <soclib_xicu.h>
#include <kdmsg.h>
#include <arch.h>

static void cpu_default_irq_handler(struct irq_action_s *action)
{
	unsigned int irq_num = (unsigned int) action->data;

	isr_dmsg(WARNING, "WARNING: No registered handler fo IRQ %d on CPU %d\n", irq_num, cpu_get_id());
	cpu_disable_single_irq(irq_num, NULL);
	isr_dmsg(WARNING, "WARNING: IRQ %d on CPU %d has been masked\n", irq_num, cpu_get_id());
}


error_t arch_cpu_init(struct cpu_s *cpu)
{
	register int i;
	register struct irq_action_s *action_ptr;
	kmem_req_t req;
  
	req.type  = KMEM_GENERIC;
	req.size  = sizeof(*action_ptr);
	req.flags = AF_BOOT | AF_ZERO;

	for(i=1; i < CPU_IRQ_NR; i++)
	{
		if((action_ptr = kmem_alloc(&req)) == NULL)
			return ENOMEM;

		action_ptr->irq_handler = &cpu_default_irq_handler;
		action_ptr->data = (void *) i;
		arch_cpu_set_irq_entry(cpu, i, action_ptr);
	}

	return 0;
}

error_t arch_cpu_set_irq_entry(struct cpu_s *cpu, int irq_nr, struct irq_action_s *action)
{
	cpu->arch.irq_vector[irq_nr] = action;
	return 0;
}

error_t arch_cpu_get_irq_entry(struct cpu_s *cpu, int irq_nr, struct irq_action_s **action)
{
	*action = cpu->arch.irq_vector[irq_nr];
	return 0;
}


error_t arch_set_power_state(struct cpu_s *cpu, arch_power_state_t state)
{
	switch(state)
	{
	case ARCH_PWR_IDLE:
		cpu_power_idle();
		return 0;

	case ARCH_PWR_SLEEP:

	case ARCH_PWR_SHUTDOWN:
		printk(WARNING, "WARNING: Unexpected power state (%d) has been asked for CPU %d, of Cluster %d\n", 
		       state, 
		       cpu->lid, 
		       cpu->cluster->id);
		return 0;
	default:
		printk(ERROR, "ERROR: Unknown power state (%d) has been asked for CPU %d, of Cluster %d\n", 
		       state, 
		       cpu->lid, 
		       cpu->cluster->id);

		return EINVAL;
	}
}

sint_t arch_barrier_init(struct cluster_s *cluster, struct event_s *event, uint_t count)
{
	return soclib_xicu_barrier_init(get_arch_entry(cluster->id)->xicu, event, count);
}

sint_t arch_barrier_wait(struct cluster_s *cluster, uint_t barrier_id)
{
	return soclib_xicu_barrier_wait(get_arch_entry(cluster->id)->xicu, barrier_id);
}

error_t arch_barrier_destroy(struct cluster_s *cluster, uint_t barrier_id)
{
	return soclib_xicu_barrier_destroy(get_arch_entry(cluster->id)->xicu, barrier_id);
}


//FIXME
error_t arch_cpu_send_ipi(gid_t dest, uint32_t val)
{
	uint_t cid;
	uint_t lid;

	cid = arch_cpu_cid(dest);//same number of cpu
	lid = arch_cpu_lid(dest);//same number of cpu

	return soclib_xicu_ipi_send(get_arch_entry(cid)->xicu, lid, val);
}
