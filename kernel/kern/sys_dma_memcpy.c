/*
 * kern/sys_dma_memcpy.c - exported DMA access to user process
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
#include <event.h>
#include <device.h>
#include <driver.h>
#include <kmem.h>
#include <kdmsg.h>
#include <dma.h>
#include <kcm.h>
#include <thread.h>
#include <cluster.h>
#include <kcm.h>


KMEM_OBJATTR_INIT(dma_kmem_request_init)
{
	attr->type = KMEM_DMA_REQUEST;
	attr->name = "KCM Dev-Request";
	attr->size = sizeof(dev_request_t);
	attr->aligne = 0;
	attr->min  = CONFIG_DMA_RQ_KCM_MIN;
	attr->max  = CONFIG_DMA_RQ_KCM_MAX;
	attr->ctor = NULL;
	attr->dtor = NULL;
  
	return 0;
}

static EVENT_HANDLER(dma_async_request_event)
{
	kmem_req_t req;
	error_t err;
  
	if((err=event_get_error(event)) != 0)
		printk(ERROR, "ERROR: dma_async_request_event: DMA transfare is not well completed, remaining %d\n", err);

	req.type = KMEM_DMA_REQUEST;
	req.ptr  = event_get_argument(event);

	assert(req.ptr != NULL && "Corrupted argument, expected the address of request");

	kmem_free(&req);
	return 0;
}

static inline error_t dma_do_async_request(void *dst, void *src, size_t size)
{
	kmem_req_t req;
	dev_request_t *rq;
  
	req.type  = KMEM_DMA_REQUEST;
	req.size  = sizeof(*rq);
	req.flags = AF_KERNEL;

	if((rq = kmem_alloc(&req)) == NULL)
		return ENOMEM;

	memset(rq, 0, sizeof(*rq));

	rq->src   = src;
	rq->dst   = dst;
	rq->count = size;
	rq->flags = DEV_RQ_NOBLOCK;

	event_set_priority(&rq->event, E_FUNC);
	event_set_handler(&rq->event, &dma_async_request_event);
	event_set_argument(&rq->event, rq);
  
	return __sys_dma->op.dev.write(__sys_dma, rq);
}

static inline error_t dma_do_sync_request(void *dst, void *src, size_t size)
{
	dev_request_t rq;
  
	rq.src   = src;
	rq.dst   = dst;
	rq.count = size;
	rq.flags = 0;
  
	return __sys_dma->op.dev.write(__sys_dma, &rq);
}


error_t dma_memcpy(void *dst, void *src, size_t size, uint_t isAsync)
{
	if(isAsync == DMA_ASYNC)
		return dma_do_async_request(dst, src, size);
	else
		return dma_do_sync_request(dst, src, size);
}


int sys_dma_memcpy(void *src, void *dst, size_t size)
{
	return dma_do_sync_request(dst, src, size);
}
