/*
 * kern/blkio.c - provides per-page buffers I/O
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
#include <errno.h>
#include <scheduler.h>
#include <kmem.h>
#include <thread.h>
#include <device.h>
#include <driver.h>
#include <event.h>
#include <atomic.h>
#include <cluster.h>
#include <page.h>
#include <blkio.h>

static EVENT_HANDLER(blkio_async)
{
	struct blkio_s *blkio;
	struct blkio_s *info;
	struct page_s *page;
	struct thread_s *thread;
	error_t err;
	uint_t irq_state;

	blkio = event_get_argument(event);
	page  = blkio->b_page;
	info  = list_head(&page->root, struct blkio_s, b_list);
	err   = event_get_error(event);

	if(err)
	{
		printk(WARNING, "WARNING: %s: Requested I/O is failed, blk %d\n", 
		       __FUNCTION__, 
		       blkio->b_dev_rq.src);
	}

	err = (err) ? 1 : 0;

	spinlock_lock_noirq(&info->b_ctrl.lock, &irq_state);

	info->b_ctrl.cntr ++;
	info->b_ctrl.error = (info->b_ctrl.error) ? 1 : err;

	if(info->b_ctrl.cntr == info->b_ctrl.count) 
	{
		thread = wakeup_one(&info->b_ctrl.wait, WAIT_ANY);

#if CONFIG_BLKIO_DEBUG    
		if(thread != NULL)
		{
			printk(INFO, "%s: cpu %d, wakeup tid %d on cpu %d [%u]\n",
			       __FUNCTION__,
			       cpu_get_id(),
			       thread->info.order,
			       thread_current_cpu(thread)->gid,
			       cpu_time_stamp());
		}
#endif
	}

	spinlock_unlock_noirq(&info->b_ctrl.lock, irq_state);
	return 0;
}

static void blkio_ctor(struct kcm_s *kcm, void *ptr) 
{
	struct blkio_s *blkio = (struct blkio_s *)ptr;

	spinlock_init(&blkio->b_ctrl.lock, "BlkIO");
	event_set_handler(&blkio->b_dev_rq.event, blkio_async);
	event_set_priority(&blkio->b_dev_rq.event, E_BLK);
	event_set_argument(&blkio->b_dev_rq.event, blkio);
	wait_queue_init(&blkio->b_ctrl.wait, "BLKIO Sync");
}

KMEM_OBJATTR_INIT(blkio_kmem_init)
{
	attr->type = KMEM_BLKIO;
	attr->name = "KCM BLKIO";
	attr->size = sizeof(struct blkio_s);
	attr->aligne = 0;
	attr->min = CONFIG_BLKIO_MIN;
	attr->max = CONFIG_BLKIO_MAX;
	attr->ctor = blkio_ctor;
	attr->dtor = NULL;
	return 0;
}

error_t blkio_init(struct device_s *dev, struct page_s *page, uint_t count) 
{
	kmem_req_t req;
	struct blkio_s *blkio;
	uint_t blkio_nr;

	req.type  = KMEM_BLKIO;
	req.size  = sizeof(*blkio);
	req.flags = AF_KERNEL;
  
	slist_root_init(&page->root);
	blkio_nr = count;

#if CONFIG_BLKIO_DEBUG    
	printk(INFO, "%s: cpu %d, count %d, pg @0x%x, index %d [%d]\n",
	       __FUNCTION__, 
	       cpu_get_id(),
	       count, 
	       page, 
	       page->index,
	       cpu_time_stamp());
#endif

	while(count)
	{
		if((blkio = kmem_alloc(&req)) == NULL) 
		{
			blkio_destroy(page);
			return ENOMEM;
		}
    
		blkio->b_flags = 0;
		blkio->b_page  = page;
		blkio->b_dev   = dev;

		list_push(&page->root, &blkio->b_list);
		count -= 1;
	}

	blkio = list_head(&page->root, struct blkio_s, b_list);

	blkio->b_ctrl.count = blkio_nr;
	blkio->b_ctrl.cntr  = 0;
	blkio->b_ctrl.error = 0;

	PAGE_SET(page, PG_BUFFER);
	return 0;
}

error_t blkio_sync(struct page_s *page, uint_t flags) 
{
	struct slist_entry *iter;
	struct blkio_s *blkio;
	struct blkio_s *info;
	device_request_t *handler;
	error_t err;
	uint_t irq_state;
	uint_t cntr;

	info = list_head(&page->root, struct blkio_s, b_list);
	handler = (flags & BLKIO_RD) ? info->b_dev->op.dev.read : info->b_dev->op.dev.write;
	cntr =  0;
	err =  0;

	list_foreach(&page->root, iter) 
	{
		blkio = list_element(iter, struct blkio_s, b_list);

		if(blkio->b_flags & BLKIO_INIT)
		{
			blkio->b_flags &= ~BLKIO_INIT;
			continue;
		}

		blkio->b_dev_rq.flags = DEV_RQ_NOBLOCK;

#if CONFIG_BLKIO_DEBUG  
		printk(INFO, "%s: cpu %d, Submitting blkio #%d, src %d, dst %x [%d]\n",
		       __FUNCTION__, 
		       cpu_get_id(),
		       cntr, 
		       blkio->b_dev_rq.src, 
		       blkio->b_dev_rq.dst, 
		       cpu_time_stamp());
#endif

		err = handler(blkio->b_dev, &blkio->b_dev_rq);
    
		if(err < 0) 
		{
			err = EIO;
			break;
		}

		cntr ++;
	}

	if(flags & BLKIO_SYNC) 
	{
		spinlock_lock_noirq(&info->b_ctrl.lock, &irq_state);
    
		info->b_ctrl.cntr += (info->b_ctrl.count - cntr);

		if(info->b_ctrl.cntr < info->b_ctrl.count) 
		{
			wait_on(&info->b_ctrl.wait, WAIT_ANY);
			spinlock_unlock_noirq(&info->b_ctrl.lock, irq_state);
			sched_sleep(current_thread);

#if CONFIG_BLKIO_DEBUG
			printk(INFO, "%s: cpu %d, tid %d, blkio ended, err %d\n",
			       __FUNCTION__, 
			       cpu_get_id(), 
			       current_thread->info.order, 
			       info->b_ctrl.error);
#endif

		}
		else
			spinlock_unlock_noirq(&info->b_ctrl.lock, irq_state);

		return (info->b_ctrl.error) ? EIO : 0;
	}

	return err;
}

error_t blkio_destroy(struct page_s *page) 
{
	kmem_req_t req;
	struct slist_entry *iter;

	req.type = KMEM_BLKIO;

	list_foreach(&page->root, iter) 
	{
		req.ptr = list_element(iter, struct blkio_s, b_list);
		kmem_free(&req);
	}

	page->private = 0;
	PAGE_CLEAR(page, PG_BUFFER);
	return 0;
}
