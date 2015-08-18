/*
 * soclib_icu.c - soclib Interrupt Control Unit driver
 *
 * Copyright (c) 2007,2008,2009,2010,2011,2012 Ghassan Almaless
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
#include <drvdb.h>
#include <kmem.h>
#include <soclib_icu.h>
#include <errno.h>
#include <string.h>
#include <cpu-trace.h>
#include <thread.h>

// IRQ input lines
#define ICU_IRQ_MAX          32    

/* ICU mapped registers offset */
#define ICU_INPUT_REG        0
#define ICU_MASK_REG         1
#define ICU_MASK_SET_REG     2
#define ICU_MASK_CLEAR_REG   3
#define ICU_HIGHEST_IRQ      4

struct icu_context_s
{
	struct irq_action_s *irq_vector[ICU_IRQ_MAX];
};

static void icu_set_mask(struct device_s *icu, uint_t mask, uint_t type, uint_t output_irq)
{
	volatile uint32_t *base = icu->base;
  
	base[ICU_MASK_SET_REG] = mask;
}


static uint_t icu_get_mask(struct device_s *icu, uint_t type, uint_t output_irq)
{
	volatile uint32_t *base = icu->base;

	return base[ICU_MASK_REG];
}


static sint_t icu_get_highest_irq(struct device_s *icu, uint_t type, uint_t output_irq)
{
	volatile uint32_t *base = icu->base;

	return base[ICU_HIGHEST_IRQ];
}


static void icu_default_irq_handler(struct device_s *icu, uint_t irq_num)
{
	register uint_t cpu_id  = cpu_get_id();
 
	isr_dmsg(WARNING, "WARNING: No ICU registered handler for IRQ %d on CPU %d\n",irq_num, cpu_id);
	icu_set_mask(icu, icu_get_mask(icu, 0, 0) & ~(1 << irq_num), 0, 0); 
	isr_dmsg(WARNING, "WARNING: IRQ %d on CPU %d has been masked\n",irq_num, cpu_id);
}


void icu_irq_handler (struct irq_action_s *action)
{
	register struct device_s *icu;
	register struct icu_context_s *ctx;
	register struct irq_action_s *dev_action_ptr;
	signed char irq_num;

	cpu_trace_write(current_cpu, icu_irq_handler);
  
	icu = action->dev;
	ctx = (struct icu_context_s*) icu->data;

	while((irq_num = icu_get_highest_irq(icu,0,0)) >= 0)
	{
		if(ctx->irq_vector[irq_num] == NULL)
		{
			icu_default_irq_handler(icu, irq_num);
			continue;
		}
		dev_action_ptr = ctx->irq_vector[irq_num];
		dev_action_ptr->irq_handler(dev_action_ptr);
	}
}

error_t soclib_icu_bind(struct device_s *icu, struct device_s *dev)
{
	struct icu_context_s *ctx = (struct icu_context_s*) icu->data;
  
	if(dev->irq >= ICU_IRQ_MAX)
		return ERANGE;

	if(ctx->irq_vector[dev->irq] != NULL)
		return EBUSY;

	ctx->irq_vector[dev->irq] = &dev->action;
	return 0;
}

static uint_t icu_count = 0;

error_t soclib_icu_init(struct device_s *icu)
{
	kmem_req_t req;
	struct icu_context_s *ctx;
  
	icu->type = DEV_INTERNAL;

	icu->action.dev         = icu; 
	icu->action.irq_handler = &icu_irq_handler;
	icu->action.data        = NULL;

	sprintk(icu->name, "icu%d", icu_count++);
	metafs_init(&icu->node, icu->name);
  
	icu->op.icu.set_mask        = &icu_set_mask;
	icu->op.icu.get_mask        = &icu_get_mask;
	icu->op.icu.get_highest_irq = &icu_get_highest_irq;
	icu->op.icu.bind            = &soclib_icu_bind;
	icu->op.drvid               = SOCLIB_ICU_ID;

	req.type  = KMEM_GENERIC;
	req.size  = sizeof(*ctx);
	req.flags = AF_BOOT | AF_ZERO;
  
	if((ctx = kmem_alloc(&req)) == NULL)
		return ENOMEM;
  
	icu->data = (void *) ctx;
	return 0;
}

driver_t soclib_icu_driver = { .init = &soclib_icu_init };
