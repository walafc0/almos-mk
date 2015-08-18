/*
 * soclib_xicu.c - soclib xicu driver
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
#include <system.h>
#include <cpu.h>
#include <device.h>
#include <driver.h>
#include <drvdb.h>
#include <kmem.h>
#include <soclib_xicu.h>
#include <errno.h>
#include <string.h>
#include <cpu-trace.h>
#include <thread.h>
#include <task.h>
#include <vm_region.h>
#include <pmm.h>
#include <vfs.h>

#define DEBUG_XICU 0

/** XICU Priority Decoding Macros */
#define XICU_PRIO_WTI(val)     (((val) >> 24) & 0x1f)
#define XICU_PRIO_HWI(val)     (((val) >> 16) & 0x1f)
#define XICU_PRIO_PTI(val)     (((val) >> 8)  & 0x1f)

#define XICU_PRIO_HAS_CNTR(val) ((val) & 0x8)
#define XICU_PRIO_HAS_WTI(val)  ((val) & 0x4)
#define XICU_PRIO_HAS_HWI(val)  ((val) & 0x2)
#define XICU_PRIO_HAS_PTI(val)  ((val) & 0x1)

#if DEBUG_XICU
#define xicu_dmsg(...) printk(__VA_ARGS__)
#else
#define xicu_dmsg(...) /**/
#endif

struct irq_actions_s;
struct device_s;
struct event_s;

typedef struct xicu_cntr_info_s
{
	struct event_s *event;
	uint32_t value;
	//uint_t signature;
}xicu_cntr_info_t;

struct xicu_context_s
{
	spinlock_t hwi_lock;
	spinlock_t pti_lock;
	spinlock_t cntr_lock;
	uint_t next_cntr;
	BITMAP_DECLARE(bitmap,8);
  
	struct irq_action_s *hwi_irq_vector[XICU_HWI_MAX];
	struct irq_action_s *wti_vector[XICU_WTI_MAX];
	xicu_cntr_info_t cntr_irq_vector[XICU_CNTR_MAX];
};


/** Write a value to specific XICU mapped register */
static inline void xicu_reg_write(volatile void *base, cid_t base_cid, uint_t func, uint_t id, uint32_t val)
{
	volatile uint32_t *_base = base;
	remote_sw((void*)&_base[func << 5 | id], base_cid, val);
}

/** Read the value of specific XICU mapped register */
static inline uint_t xicu_reg_read(volatile void *base, uint_t func, uint_t id)
{
	volatile uint32_t *_base = base;
	return _base[func << 5 | id];
}

static void xicu_set_mask(struct device_s *icu, uint_t mask, uint_t type, uint_t output_irq)
{
	xicu_reg_write(icu->base, icu->cid, type, output_irq, mask);

	if(type == XICU_PTI_TYPE)
		xicu_reg_write(icu->base, icu->cid, XICU_PTI_PER, output_irq, CPU_CLOCK_TICK);
}

static uint_t xicu_get_mask(struct device_s *icu, uint_t type, uint_t output_irq)
{
	return xicu_reg_read(icu->base, type, output_irq);
}

sint_t xicu_get_highest_irq(struct device_s *icu, uint_t type, uint_t output_irq)
{
	uint_t prio = xicu_reg_read(icu->base, XICU_PRIO, output_irq);
  
	switch(type)
	{
	case XICU_HWI_TYPE:
		xicu_dmsg(INFO, "received HWI : out_pu_irq %d \n", output_irq);// for debug
		return (XICU_PRIO_HAS_HWI(prio)) ? XICU_PRIO_HWI(prio) : -1;
	case XICU_PTI_TYPE:
		xicu_dmsg(INFO, "received PTI: out_pu_irq %d \n", output_irq);// for debug
		return (XICU_PRIO_HAS_PTI(prio)) ? XICU_PRIO_PTI(prio) : -1;
	case XICU_WTI_TYPE:
		xicu_dmsg(INFO, "received WTI : out_pu_irq %d \n", output_irq);// for debug
		return (XICU_PRIO_HAS_WTI(prio)) ? XICU_PRIO_WTI(prio) : -1;
	case XICU_CNTR_TYPE:
		xicu_dmsg(INFO, "received CNTR :out_pu_irq %d \n", output_irq);// for debug
		return (XICU_PRIO_HAS_CNTR(prio)) ? xicu_reg_read(icu->base, XICU_CNTR_ACTIVE, output_irq) : -1;
	default:
		return -2;
	}
}

void xicu_timer_irq_handler (struct device_s *xicu, uint_t output_irq)
{
	struct cpu_s *cpu;
	uint_t pti_index;

	cpu = current_cpu;
	cpu_clock(cpu);

	pti_index = output_irq / OUTPUT_IRQ_PER_PROC;
	xicu_reg_read(xicu->base, XICU_PTI_ACK, pti_index);
	xicu_reg_write(xicu->base, xicu->cid, XICU_PTI_VAL, pti_index, cpu_get_ticks_period(cpu));
}

static void xicu_default_irq_handler(struct device_s *icu, uint_t irq_num, uint_t output_irq)
{
	isr_dmsg(WARNING, "WARNING: No XICU::HWI registered handler fo IRQ %d on CPU %d\n",irq_num, output_irq);
	xicu_set_mask(icu, xicu_get_mask(icu, XICU_HWI_TYPE, output_irq) & ~(1 << irq_num), XICU_HWI_TYPE, output_irq); 
	isr_dmsg(WARNING, "WARNING: XICU::HWI IRQ %d on CPU %d has been masked\n",irq_num, output_irq);
}

void xicu_irq_handler (struct irq_action_s *action)
{
	register struct device_s *xicu;
	register struct cpu_s *cpu;
	register struct xicu_context_s *ctx;
	register struct irq_action_s *dev_action_ptr;
	register sint_t irq_num;
	register uint_t irq_state;
	register uint_t output_irq;
	register uint_t prio;
  
	cpu_trace_write(current_cpu, xicu_irq_handler);
  
	//assert(current_thread->signature == THREAD_ID);
	cpu        = current_cpu;
	output_irq = cpu->lid * OUTPUT_IRQ_PER_PROC;//FG
	xicu       = action->dev;
	ctx        = xicu->data;
	prio       = xicu_reg_read(xicu->base, XICU_PRIO, output_irq);
  
	if(XICU_PRIO_HAS_WTI(prio))
	{
    
		while((irq_num = xicu_get_highest_irq(xicu, XICU_WTI_TYPE, output_irq)) >= 0)
		{
			xicu_dmsg(INFO, "received wti : wti_nr = %d (out_pu_irq %d) \n", irq_num, output_irq);// for debug

			//FG : comment folowing line if iopic read itself in WTI_REG
			uint32_t ipi_val = xicu_reg_read(xicu->base, XICU_WTI_REG, irq_num);//signal that wti was taken into account
			
			if(ctx->wti_vector[irq_num] == NULL)
			{
				//FG : check what should be done here
				//xicu_dmsg(WARNING, "WARNING : No handler for that wti : cpu %d, wti_nr = %d\n", cpu_get_id(), irq_num);
				cpu_ipi_notify(cpu, ipi_val);
				continue;
			}


			dev_action_ptr = ctx->wti_vector[irq_num];
			dev_action_ptr->irq_handler(dev_action_ptr);
		}
		return;
	}

	if(XICU_PRIO_HAS_PTI(prio))
	{
		xicu_dmsg(INFO, "received timer: proc %d out_pu_irq %d \n", cpu_get_id(), output_irq);// for debug
		xicu_timer_irq_handler(xicu, output_irq);
		return;
	}
    
	if(XICU_PRIO_HAS_HWI(prio))
	{
		while((irq_num = xicu_get_highest_irq(xicu, XICU_HWI_TYPE, output_irq)) >= 0)
		{
			xicu_dmsg(INFO, "received irq : irq_nr = %d (out_pu_irq %d) \n", irq_num, output_irq);// for debug

			if(ctx->hwi_irq_vector[irq_num] == NULL)
			{
				xicu_default_irq_handler(xicu, irq_num, output_irq);
				continue;
			}

			dev_action_ptr = ctx->hwi_irq_vector[irq_num];
			dev_action_ptr->irq_handler(dev_action_ptr);
		}
	}

	if(XICU_PRIO_HAS_CNTR(prio))
	{
		irq_state = xicu_reg_read(xicu->base, XICU_CNTR_ACTIVE, output_irq);
		irq_state &= XICU_CNTR_MASK;
		irq_num = 0;

		while(irq_state)
		{
			if(irq_state & 0x1)
			{
				xicu_dmsg(INFO, "received cntr : irq_nr = %d (out_pu_irq %d) \n", irq_num, output_irq);// for debug

				xicu_reg_write(xicu->base, xicu->cid, XICU_CNTR_REG, irq_num, ctx->cntr_irq_vector[irq_num].value);
	
				if(ctx->cntr_irq_vector[irq_num].event != NULL)
					event_send(ctx->cntr_irq_vector[irq_num].event, cpu->gid);
			}
      
			irq_state >>=1; 
			irq_num ++;
		}

		if(irq_num != 0) return;
	}
    
	cpu->spurious_irq_nr ++;
}

error_t soclib_xicu_bind(struct device_s *icu, struct device_s *dev)
{
	struct xicu_context_s *ctx = icu->data;

	if(dev->irq >= XICU_HWI_MAX)
		return ERANGE;

	if(ctx->hwi_irq_vector[dev->irq] != NULL)
		return EBUSY;

	ctx->hwi_irq_vector[dev->irq] = &dev->action;
	return 0;
}

//init wti_vector binding wti_entry[i] with handler//FG
error_t soclib_xicu_bind_wti(struct device_s *icu, struct irq_action_s *action, uint_t wti_index)
{
	struct xicu_context_s *ctx = icu->data;

	if(wti_index >= XICU_WTI_MAX)
		return ERANGE;

	if(ctx->wti_vector[wti_index] != NULL)
		return EBUSY;

	ctx->wti_vector[wti_index] = action;
	return 0;
}

error_t soclib_xicu_ipi_send(struct device_s *xicu, uint_t port, uint32_t val)
{
	xicu_reg_write(xicu->base, xicu->cid, XICU_WTI_REG, port, val);
	return 0;
}

sint_t soclib_xicu_barrier_init(struct device_s *xicu, struct event_s *event, uint_t count)
{
	sint_t hwid;
	struct xicu_context_s *ctx;

	ctx = xicu->data;

	spinlock_lock(&ctx->cntr_lock);
  
	hwid = bitmap_ffs2(ctx->bitmap, ctx->next_cntr, sizeof(ctx->bitmap));
  
	if(hwid != -1)
	{ 
		bitmap_clear(ctx->bitmap, hwid);
		ctx->next_cntr = hwid + 1;
	}
   
	spinlock_unlock(&ctx->cntr_lock);

	if(hwid != -1)
	{
		ctx->cntr_irq_vector[hwid].event = event;
		ctx->cntr_irq_vector[hwid].value = count;
		xicu_reg_write(xicu->base, xicu->cid, XICU_CNTR_REG, hwid, ctx->cntr_irq_vector[hwid].value);
	}

	return hwid;
}

sint_t  soclib_xicu_barrier_wait(struct device_s *xicu, uint_t barrier_id)
{
	sint_t val;
	struct xicu_context_s *ctx;

	ctx = xicu->data;
	val = xicu_reg_read(xicu->base, XICU_CNTR_REG, barrier_id & XICU_CNTR_MASK);
	return val;
}

error_t soclib_xicu_barrier_destroy(struct device_s *xicu, uint_t barrier_id)
{
	struct xicu_context_s *ctx;

	ctx = xicu->data;
	barrier_id &= XICU_CNTR_MASK;

	spinlock_lock(&ctx->cntr_lock);
	ctx->cntr_irq_vector[barrier_id].event = NULL;
	bitmap_set(ctx->bitmap, barrier_id);
	ctx->next_cntr = (barrier_id < ctx->next_cntr) ? barrier_id : ctx->next_cntr;
	spinlock_unlock(&ctx->cntr_lock);

	return 0;
}


#if CONFIG_XICU_USR_ACCESS
static VM_REGION_PAGE_FAULT(icu_pagefault)
{
	pmm_page_info_t info;
	error_t err;

	if((err = pmm_get_page(&current_task->vmm.pmm, vaddr, &info)))
		return err;

	printk(INFO,"%s: vaddr %x, info.ppn %x, info.attr %x\n",
	       __FUNCTION__,
	       vaddr,
	       info.ppn,
	       info.attr);

	return EFAULT;
}

static struct vm_region_op_s icu_vm_region_op = 
{
	.page_in     = NULL,
	.page_out    = NULL,
	.page_lookup = NULL,
	.page_fault  = icu_pagefault
};

static error_t xicu_mmap(struct device_s *icu, dev_request_t *rq)
{
	struct vfs_file_s *file;
	struct vm_region_s *region;
	struct pmm_s *pmm;
	uint_t size;
	pmm_page_info_t info;
	error_t err;

	file   = rq->file;
	region = rq->region;
	pmm    = &region->vmm->pmm;
	size   = PMM_PAGE_SIZE;
  
	if((region->vm_limit - region->vm_start) != size)
	{
		printk(INFO, "INFO: %s: pid %d, cpu %d: Asked size (%d) which does not mach real one (%d)\n",
		       __FUNCTION__,
		       current_task->pid,
		       cpu_get_id(),
		       region->vm_limit - region->vm_start,
		       size);

		return ERANGE;
	}

	printk(INFO, "INFO: %s: started, file %s, region <0x%x - 0x%x>\n",
	       __FUNCTION__, 
	       file->f_node->n_name, 
	       region->vm_start, 
	       region->vm_limit);
  
	region->vm_flags |= VM_REG_DEV;

	if((err = pmm_get_page(pmm, ((vma_t)icu->base) + PMM_PAGE_SIZE, &info)))
		return err;

	printk(INFO, "INFO: %s: ppn for xicu base %x is %x\n", 
	       __FUNCTION__,
	       ((vma_t)icu->base) + PMM_PAGE_SIZE,
	       info.ppn);
  
	info.attr    = region->vm_pgprot & ~(PMM_CACHED);
	info.cluster = NULL;
 
	if((err = pmm_set_page(pmm, region->vm_start, &info)))
		return err;
  
	region->vm_file = file;
	region->vm_mapper = NULL;

	region->vm_op = &icu_vm_region_op;
	return 0;
}

static error_t xicu_munmap(struct device_s *icu, dev_request_t *rq)
{
	struct vfs_file_s *file;
	struct vm_region_s *region;
	struct pmm_s *pmm;
	uint_t size;
	pmm_page_info_t info;
	error_t err;

	file   = rq->file;
	region = rq->region;
	pmm    = &region->vmm->pmm;
	size   = PMM_PAGE_SIZE;
  
	if((region->vm_limit - region->vm_start) != size)
	{
		printk(INFO, "INFO: %s: pid %d, cpu %d: Asked size (%d) which does not mach real one (%d)\n",
		       __FUNCTION__,
		       current_task->pid,
		       cpu_get_id(),
		       region->vm_limit - region->vm_start,
		       size);

		return ERANGE;
	}

	printk(INFO, "INFO: %s: started, file %s, region <0x%x - 0x%x>\n",
	       __FUNCTION__, 
	       file->f_node->n_name, 
	       region->vm_start, 
	       region->vm_limit);
    
	info.attr = 0;
	info.ppn  = 0;
	info.cluster = NULL;
  
	if((err = pmm_set_page(pmm, region->vm_start, &info)))
		return err;
  
	return 0;
}


static sint_t xicu_open_op(struct device_s *icu, dev_request_t *rq)
{
	return 0;
}

static sint_t xicu_file_op(struct device_s *icu, dev_request_t *rq)
{
	return -1;
}

static sint_t xicu_params_op(struct device_s *icu, dev_params_t *params)
{
	params->size = PMM_PAGE_SIZE;
	return 0;
}
#endif	/* CONFIG_XICU_USR_ACCESS */

static uint_t xicu_count = 0;

error_t soclib_xicu_init(struct device_s *icu)
{
	kmem_req_t req;
	struct xicu_context_s *ctx;

#if CONFIG_XICU_USR_ACCESS
	icu->type = DEV_CHR;
#else
	icu->type = DEV_INTERNAL;
#endif
	icu->action.dev         = icu;
	icu->action.irq_handler = &xicu_irq_handler;
	icu->action.data        = NULL;

	sprintk(icu->name, "xicu%d", xicu_count++);
	metafs_init(&icu->node, icu->name);

#if CONFIG_XICU_USR_ACCESS
	icu->op.icu.open   = &xicu_open_op;
	icu->op.icu.read   = &xicu_file_op;
	icu->op.icu.write  = &xicu_file_op;
	icu->op.icu.close  = &xicu_file_op;
	icu->op.icu.lseek  = &xicu_file_op;
	icu->op.icu.mmap   = &xicu_mmap;
	icu->op.icu.munmap = &xicu_munmap;
	icu->op.icu.set_params = NULL;
	icu->op.icu.get_params = &xicu_params_op;
#endif

	icu->op.icu.set_mask        = &xicu_set_mask;
	icu->op.icu.get_mask        = &xicu_get_mask;
	icu->op.icu.get_highest_irq = &xicu_get_highest_irq;
	icu->op.icu.bind            = &soclib_xicu_bind;
	icu->op.icu.bind_wti        = &soclib_xicu_bind_wti;
	icu->op.drvid               = SOCLIB_XICU_ID;

	req.type  = KMEM_GENERIC;
	req.size  = sizeof(*ctx);
	req.flags = AF_BOOT | AF_ZERO;

	if((ctx = kmem_alloc(&req)) == NULL)
		return ENOMEM;

	spinlock_init(&ctx->hwi_lock, "DevXICU.HWI");
	spinlock_init(&ctx->pti_lock, "DevXICU.PTI");
	spinlock_init(&ctx->cntr_lock, "DevXICU.CNTR");
	ctx->next_cntr = 0;
	bitmap_set_range(ctx->bitmap, 0, XICU_CNTR_MAX);
	icu->data = ctx;
	return 0;
}

driver_t soclib_xicu_driver = { .init = &soclib_xicu_init };
