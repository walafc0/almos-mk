/*
 * soclib_dma.c - soclib DMA driver
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
#include <cpu.h>
#include <device.h>
#include <driver.h>
#include <drvdb.h>
#include <kmem.h>
#include <scheduler.h>
#include <thread.h>
#include <task.h>
#include <list.h>
#include <event.h>
#include <soclib_dma.h>
#include <string.h>

/* DMA mapped registers offset */
#define DMA_SRC_REG          0
#define DMA_DST_REG          1
#define DMA_LEN_REG          2
#define DMA_RESET_REG        3
#define DMA_IRQ_DISABLED     4

struct dma_context_s
{
	struct list_entry request_queue;
	struct wait_queue_s pending;
};

static void dma_start_request(struct device_s *dma, dev_request_t *ptr)
{
	volatile uint32_t *base = dma->base;
	dev_request_t *rq;

	rq = (ptr->flags & DEV_RQ_NESTED) ? ptr->data : ptr;

	base[DMA_SRC_REG]      = (unsigned int)rq->src;
	base[DMA_DST_REG]      = (unsigned int)rq->dst;
	base[DMA_IRQ_DISABLED] = 0;
	base[DMA_LEN_REG]      = rq->count;
}

static void dma_irq_handler(struct irq_action_s *action)
{
	struct dma_context_s *ctx;
	struct device_s *dma;
	dev_request_t *rq;
	dev_request_t *frag;
	dev_request_t *new_rq;
	error_t err;
	volatile uint32_t *base;
	struct thread_s *this;

	dma  = action->dev;
	base = dma->base;
	ctx  = (struct dma_context_s*)dma->data;

	cpu_spinlock_lock(&dma->lock.val);
  
	err = base[DMA_LEN_REG];
	base[DMA_RESET_REG] = 0;

	/* even if this case is realy low probable, we have to check for empty list ! */
	if(list_empty(&ctx->request_queue))
	{
		cpu_spinlock_unlock(&dma->lock.val);
		return;
	}

	rq = list_first(&ctx->request_queue, dev_request_t, list);

	if(rq->flags & DEV_RQ_NESTED)
	{
		frag = rq->data;

		event_set_error(&frag->event, err);
		event_set_senderId(&frag->event, dma);
		event_send(&frag->event, current_cpu->gid);

		if((err == 0) && (frag->data != NULL))
		{
			rq->data = frag->data;
			dma_start_request(dma,frag->data);
			cpu_spinlock_unlock(&dma->lock.val);
			return;
		}
	}

	list_unlink(&rq->list);
  
	if(!(list_empty(&ctx->request_queue)))
	{
		new_rq = list_first(&ctx->request_queue, dev_request_t, list);
		dma_start_request(dma,new_rq);
	}
  
	if(!(rq->flags & DEV_RQ_NOBLOCK))
	{
		rq->err = err;
		wakeup_one(&ctx->pending, WAIT_FIRST);
		cpu_spinlock_unlock(&dma->lock.val);
		return;
	}
  
	cpu_spinlock_unlock(&dma->lock.val);

	this = current_thread;
 
	event_set_error(&rq->event, err);
	event_set_senderId(&rq->event, dma);
	event_send(&rq->event, current_cpu->gid);
}

static void dma_fraglist_destroy(dev_request_t *ptr)
{
	kmem_req_t req;
	dev_request_t *frag;

	req.type = KMEM_DMA_REQUEST;
	frag     = ptr;

	while(frag != NULL);
	{
		req.ptr = frag;
		frag    = frag->data;

		kmem_free(&req);
	}
}

static EVENT_HANDLER(dma_async_frag_event)
{
	dev_request_t *frag;
	kmem_req_t req;
	error_t err;

	frag = event_get_argument(event);
	err  = event_get_error(event);

	assert(frag != NULL && "corrupted frag");

	if(err)
	{
		dma_fraglist_destroy(frag);
		return 0;
	}

	req.type = KMEM_DMA_REQUEST;
	req.ptr  = frag;

	kmem_free(&req);
	return 0;
}

static error_t dma_frag_init(dev_request_t *frag, uint_t src_start, uint_t dst_start, uint_t count)
{
	//FIXME(40) use extent registers
	frag->src   = task_vaddr2paddr(current_task, (void*)src_start);
	frag->dst   = task_vaddr2paddr(current_task, (void*)dst_start);
	frag->count = count;
	frag->flags = 0;

	if((frag->src == NULL) || (frag->dst == NULL))
		return EPERM;

	event_set_priority(&frag->event, E_FUNC);
	event_set_handler(&frag->event, &dma_async_frag_event);
	event_set_argument(&frag->event, frag);
	return 0;
}

static error_t dma_fraglist_build(dev_request_t *rq)
{
	kmem_req_t req;
	dev_request_t *frag;
	dev_request_t *parent_frag;
	struct slist_entry *root;
	struct slist_entry *iter;
	uint_t src_start;
	uint_t dst_start;
	uint_t count;
	error_t err;

	if(rq->count == 0)
		return EINVAL;

	src_start = (uint_t)rq->src;
	dst_start = (uint_t)rq->dst;

	if((src_start + rq->count) <= ((src_start & ~PMM_PAGE_MASK) + PMM_PAGE_SIZE) &&
	   (dst_start + rq->count) <= ((dst_start & ~PMM_PAGE_MASK) + PMM_PAGE_SIZE))
	{
		//FIXME(40) use extent registers
		rq->src = task_vaddr2paddr(current_task, rq->src);
		rq->dst = task_vaddr2paddr(current_task, rq->dst);

		if((rq->src == NULL) || (rq->dst == NULL))
			return EPERM;

		return 0;
	}

	if((src_start & PMM_PAGE_MASK) || (dst_start & PMM_PAGE_MASK)) {
		return ENOSYS;
	}

	req.type    = KMEM_DMA_REQUEST;
	req.size    = sizeof(*rq);
	req.flags   = AF_KERNEL;
	root        = (struct slist_entry *)&rq->data;
	iter        = (struct slist_entry *)&rq->list;
	count       = rq->count;
	rq->data    = NULL;
	rq->flags  &= ~DEV_RQ_NESTED;
	parent_frag = rq;

	while(count)
	{
		frag = kmem_alloc(&req);

		if(frag == NULL)
		{
			err = ENOMEM;
			goto fail_nomem;
		}

		if(count >= PMM_PAGE_SIZE)
		{
			err = dma_frag_init(frag, src_start, dst_start, PMM_PAGE_SIZE);

			if(err)
				goto fail_init;

			src_start += PMM_PAGE_SIZE;
			dst_start += PMM_PAGE_SIZE;
			count     -= PMM_PAGE_SIZE;
		}
		else
		{
			err = dma_frag_init(frag, src_start, dst_start, count);

			if(err)
				goto fail_init;

			count -= count;
		}

		parent_frag->data = frag;
		frag->data        = NULL;
		parent_frag       = frag;
	}

	rq->flags |= DEV_RQ_NESTED;
	return 0;

fail_init:
fail_nomem:
	dma_fraglist_destroy(rq->data);
	return err;
}


static sint_t dma_request(struct device_s *dma, dev_request_t *rq)
{
	struct dma_context_s *ctx;
	uint_t irq_state;
	sint_t err;

	rq->err = 0;
	ctx     = (struct dma_context_s*)dma->data;

	err = dma_fraglist_build(rq);

	if(err)
		return err;

	spinlock_lock_noirq(&dma->lock,&irq_state);

	if(list_empty(&ctx->request_queue))
	{
		list_add(&ctx->request_queue, &rq->list);
		dma_start_request(dma, rq);
	}
	else
		list_add_last(&ctx->request_queue, &rq->list);

	if(rq->flags & DEV_RQ_NOBLOCK)
	{
		spinlock_unlock_noirq(&dma->lock, irq_state);
		return 0;
	}

	wait_on(&ctx->pending, WAIT_LAST);
	spinlock_unlock_noirq(&dma->lock,irq_state);
	sched_sleep(current_thread);

	return rq->err;
}

static sint_t dma_open(struct device_s *dma, dev_request_t *rq)
{
	return EPERM;
}

struct device_s *__sys_dma;
static uint_t dma_count = 0;

error_t soclib_dma_init(struct device_s *dma)
{
	kmem_req_t req;
	struct dma_context_s *ctx;

	spinlock_init(&dma->lock,"DevDMA (SoCLib)");
	dma->type = DEV_BLK;

	dma->action.dev         = dma;
	dma->action.irq_handler = &dma_irq_handler;
	dma->action.data        = NULL;

	if(dma_count == 0)
		__sys_dma = dma;

	sprintk(dma->name, 
#if CONFIG_ROOTFS_IS_VFAT
		"DMA%d"
#else
		"dma%d"
#endif
		,dma_count++);

	metafs_init(&dma->node, dma->name);
  
	dma->op.dev.open       = &dma_open;
	dma->op.dev.read       = &dma_request;
	dma->op.dev.write      = &dma_request;
	dma->op.dev.close      = NULL;
	dma->op.dev.lseek      = NULL;
	dma->op.dev.mmap       = NULL;
	dma->op.dev.munmap     = NULL;
	dma->op.dev.set_params = NULL;
	dma->op.dev.get_params = NULL;
	dma->op.drvid          = SOCLIB_DMA_ID;

	req.type  = KMEM_GENERIC;
	req.size  = sizeof(*ctx);
	req.flags = AF_BOOT | AF_ZERO;
  
	if((ctx = kmem_alloc(&req)) == NULL)
		return ENOMEM;

	wait_queue_init(&ctx->pending, dma->name);
	list_root_init(&ctx->request_queue);
	dma->data = (void *)ctx;
	return 0;
}

driver_t soclib_dma_driver = { .init = &soclib_dma_init };
