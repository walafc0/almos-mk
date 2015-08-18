/*
 * soclib_fb.c - soclib frame-buffer driver
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
#include <device.h>
#include <driver.h>
#include <drvdb.h>
#include <errno.h>
#include <vfs.h>
#include <string.h>
#include <soclib_fb.h>
#include <thread.h>
#include <task.h>
#include <vmm.h>
#include <dma.h>

#define FB_WRITE_REG      0
#define FB_READ_REG       0

struct device_s __fb_screen;

static sint_t fb_read(struct device_s *fb, dev_request_t *rq)
{
	uint8_t *src;
	uint8_t *limit;
	register size_t size;

	assert(rq->file != NULL);
  
	src   = ((uint8_t *) fb->base + rq->file->f_remote->fr_offset);
	limit = (uint8_t*) fb->base + (uint_t) fb->data;

	if(src == limit)
		return 0;

	size = ((src + rq->count) > limit) ? limit - (src + rq->count) : rq->count;

#if CONFIG_FB_USE_DMA
	error_t err;
	void *dst = task_vaddr2paddr(current_task, rq->dst);
	src = task_vaddr2paddr(current_task, src);

	if((err = dma_memcpy(dst, src, size, DMA_ASYNC)))
		printk(ERROR, "ERROR: fb_read: failed to do DMA memcpy request, err %d\n", err);
#else
	memcpy(rq->dst, src, size);
#endif
	return size;
}

static sint_t fb_write(struct device_s *fb, dev_request_t *rq)
{
	uint8_t *dst;
	uint8_t *limit;
	register size_t size;

	assert(rq->file != NULL);
  
	dst = ((uint8_t *) fb->base + rq->file->f_remote->fr_offset);
	limit = (uint8_t*)fb->base + (uint_t)fb->data;

	if(dst == limit)
		return -ERANGE;

	size = ((dst + rq->count) > limit) ? limit - (dst + rq->count) : rq->count;

#if CONFIG_FB_USE_DMA
	error_t err;
	printk(INFO,"%s: dst 0x%x, src 0x%x, size %d\n", __FUNCTION__, dst, rq->src, size);

	if((err = dma_memcpy(dst, rq->src, size, DMA_ASYNC)))
		printk(ERROR, "ERROR: fb_write: failed to do DMA memcpy request, err %d\n", err);
#else
	//printk(INFO,"%s: dst 0x%x, src 0x%x, size %d\n", __FUNCTION__, dst, rq->src, size);
	memcpy(dst, rq->src, size);
#endif
	return rq->count;
}

static error_t fb_lseek(struct device_s *fb, dev_request_t *rq)
{
	return 0;
}

static sint_t fb_get_params(struct device_s *fb, dev_params_t *params)
{
	params->size = (uint_t)fb->data;
	return 0;
}

static sint_t fb_open(struct device_s *fb, dev_request_t *rq)
{
	return 0;
}

static VM_REGION_PAGE_FAULT(fb_pagefault)
{
	return EFAULT;
}

static struct vm_region_op_s fb_vm_region_op = 
{
	.page_in     = NULL,
	.page_out    = NULL,
	.page_lookup = NULL,
	.page_fault  = fb_pagefault
};

static error_t fb_mmap(struct device_s *fb, dev_request_t *rq)
{
	struct vfs_file_s *file;
	struct vm_region_s *region;
	struct pmm_s *pmm;
	uint_t size;
	pmm_page_info_t info;
	vma_t current_vma;
	error_t err;

	file   = rq->file;
	region = rq->region;
	pmm    = &region->vmm->pmm;
	size   = (uint_t) fb->data;
  
	if(size < (region->vm_limit - region->vm_start))
	{
		printk(WARNING, "WARNING: %s: asked size (%d) exceed real one (%d)\n", 
		       __FUNCTION__, region->vm_limit - region->vm_start, size);
		return ERANGE;
	}

	printk(INFO, "INFO: %s: started, file inode %p, region <0x%x - 0x%x>\n",
	       __FUNCTION__, 
	       file->f_inode, 
	       region->vm_start, 
	       region->vm_limit);

	if((err = pmm_get_page(pmm, (vma_t)fb->base, &info)))
		return err;

	region->vm_flags |= VM_REG_DEV;
	current_vma       = region->vm_start;
	info.attr         = region->vm_pgprot & ~(PMM_CACHED);
	info.cluster      = NULL;
	size              = region->vm_limit - region->vm_start; 
	size              = ARROUND_UP(size, PMM_PAGE_SIZE);

	while(size)
	{
		if((err = pmm_set_page(pmm, current_vma, &info)))
			return err;

		info.ppn ++;
		current_vma += PMM_PAGE_SIZE;
		size -= PMM_PAGE_SIZE;
	}
  
	region->vm_file = *file;
	//region->vm_mapper = NULL;

	region->vm_op = &fb_vm_region_op;
	return 0;
}

static error_t fb_munmap(struct device_s *fb, dev_request_t *rq)
{
	struct vfs_file_s *file;
	struct vm_region_s *region;
	struct pmm_s *pmm;
	uint_t size;
	pmm_page_info_t info;
	vma_t current_vma;
	error_t err;

	file   = rq->file;
	region = rq->region;
	pmm    = &region->vmm->pmm;
	size   = (uint_t) fb->data;

	if(size < (region->vm_limit - region->vm_start))
	{
		printk(ERROR, "ERROR: %s: asked size (%d) exceed real one (%d)\n", 
		       __FUNCTION__, region->vm_limit - region->vm_start, size);
		return ERANGE;
	}

	current_vma = region->vm_start;
	info.attr   = 0;
	info.ppn    = 0;
	size        = region->vm_limit - region->vm_start;

	while(size)
	{    
		if((err = pmm_set_page(pmm, current_vma, &info)))
			return err;

		current_vma += PMM_PAGE_SIZE;
		size -= PMM_PAGE_SIZE;
	}
  
	return 0;
}

static uint_t fb_count = 0;

error_t soclib_fb_init(struct device_s *fb)
{  
	fb->type = DEV_CHR;

	fb->op.dev.open       = &fb_open;
	fb->op.dev.read       = &fb_read;
	fb->op.dev.write      = &fb_write;
	fb->op.dev.close      = NULL;
	fb->op.dev.lseek      = &fb_lseek;
	fb->op.dev.mmap       = &fb_mmap;
	fb->op.dev.munmap     = &fb_munmap;
	fb->op.dev.set_params = NULL;
	fb->op.dev.get_params = &fb_get_params;
	fb->op.drvid          = SOCLIB_FB_ID;

	fb->data = (void*)fb->size;
	sprintk(fb->name, 
#if CONFIG_ROOTFS_IS_VFAT
		"FB%d"
#else
		"fb%d"
#endif
		,fb_count++);

	metafs_init(&fb->node, fb->name);
	return 0;
}


driver_t soclib_fb_driver = { .init = &soclib_fb_init };
