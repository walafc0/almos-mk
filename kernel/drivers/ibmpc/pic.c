/*
 * pic.h - Programmable Interrupt Controller driver implementation
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
#include <cpu.h>
#include <device.h>
#include <driver.h>
#include <cpu-io.h>
#include <kmem.h>
#include <string.h>
#include <kdmsg.h>
#include <pic.h>

#define PIC_PRIMARY_PIO    0x20
#define PIC_SECONDARY_PIO  0xA0

struct irq_action_s *irq_vector[PIC_IRQ_MAX];

static void pic_set_mask(struct device_s *pic, uint_t mask, uint_t type, uint_t output_irq)
{
	switch(type)
	{
	case 0:
		cpu_io_out8(PIC_PRIMARY_PIO + PIC_IMR_REG, ~(mask & 0xFF));
		break;
	case 1:
		cpu_io_out8(PIC_SECONDARY_PIO + PIC_IMR_REG, ~(mask & 0xFF));
		break;
	default:
		printk(ERROR, "ERROR: Un expected mask value, PIC has 16 IRQ line only [ type 0 & 1 ]");
	}
}

static uint_t pic_get_mask(struct device_s *icu, uint_t type, uint_t output_irq)
{
	register uint_t mask = ~0;
  
	switch(type)
	{
	case 0:
		mask = cpu_io_in8(PIC_PRIMARY_PIO + PIC_IMR_REG);
		break;
	case 1:
		mask = cpu_io_in8(PIC_SECONDARY_PIO + PIC_IMR_REG);
		break;
	default:
		printk(ERROR, "ERROR: Un expected mask value, PIC has 16 IRQ line only [ type 0 & 1 ]");
	}
  
	return ((~mask) & 0xFF);
}

static sint_t pic_get_highest_irq(struct device_s *icu, uint_t type, uint_t output_irq)
{
	return -1;
}

static void pic_default_irq_handler(struct irq_action_s *action)
{
	register unsigned int irq_num = (unsigned int) action->data;
	register unsigned int cpu_id  = cpu_get_id();
  
	isr_dmsg(WARNING, "WARNING: No ISR registered handler for IRQ %d on CPU %d\n",irq_num, cpu_id);
}

static void pic_ICWs()
{
	/* ICW1 */
	cpu_io_out8(PIC_PRIMARY_PIO + PIC_CMD_REG, 0x11); /* Put Primary PIC in Initialization mode with ICW4 enabled */
	cpu_io_out8(PIC_SECONDARY_PIO + PIC_CMD_REG, 0x11); /* Same ICW1 as Primary */

	/* ICW2 */
	cpu_io_out8(PIC_PRIMARY_PIO + PIC_DATA_REG, 0x20); /* Set Primary PIC's interrupt vector base number */
	cpu_io_out8(PIC_SECONDARY_PIO + PIC_DATA_REG, 0x28); /* Set Secondary PIC's interrupt vector base number */

	/* ICW3 */
	cpu_io_out8(PIC_PRIMARY_PIO + PIC_DATA_REG, 0x4); /* Informe Primary that Secondary is connected to line 2 */
	cpu_io_out8(PIC_SECONDARY_PIO + PIC_DATA_REG, 0x2); /* Inform Secondary of it's identification */

	/* ICW4 */
	cpu_io_out8(PIC_PRIMARY_PIO + PIC_DATA_REG, 0x1); /* Put Primary in x86 mode */
	cpu_io_out8(PIC_SECONDARY_PIO + PIC_DATA_REG, 0x1); /* Put Primary in x86 mode */

	/* CLEAR DATA REGISTERS */
	cpu_io_out8(PIC_PRIMARY_PIO + PIC_DATA_REG, 0);
	cpu_io_out8(PIC_SECONDARY_PIO + PIC_DATA_REG, 0);
}


void pic_irq_handler(struct irq_action_s *action)
{
	register uint_t irq = (uint_t)action->data;
	irq_vector[irq]->data = (void*)irq;
	irq_vector[irq]->irq_handler(irq_vector[irq]);
}

void ibmpc_pic_init(struct device_s *pic, void *base, uint_t irq)
{
	register uint_t _irq;
	struct irq_action_s *action_ptr;

	pic->base = base;
	pic->type = DEV_ICU;
	pic->action.irq_handler = &pic_irq_handler;

	strcpy(pic->name, "PIC");
	metafs_init(&pic->node, pic->name);
  
	pic->op.icu.set_mask = &pic_set_mask;
	pic->op.icu.get_mask = &pic_get_mask;
	pic->op.icu.get_highest_irq = &pic_get_highest_irq;

	for(_irq=0; _irq < PIC_IRQ_MAX; _irq++)
	{
		action_ptr = kbootMem_calloc(sizeof(*action_ptr));
		action_ptr->dev = pic;
		action_ptr->irq_handler = &pic_default_irq_handler;
		action_ptr->data = (void*) _irq;   
		irq_vector[_irq] = action_ptr;
	}
  
	pic_ICWs();
}

void ibmpc_pic_bind(struct device_s *pic, struct device_s *dev)
{
	assert(dev->irq < PIC_IRQ_MAX && "Unexpected IRQ, connot bind this device");

	if(dev->irq < PIC_IRQ_MAX)
	{
		irq_vector[dev->irq] = &dev->action;
		metafs_register(&pic->node, &dev->node);
	}
}
