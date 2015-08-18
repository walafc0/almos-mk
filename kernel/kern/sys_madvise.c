/*
 * kern/sys_madvise.c - process memory management related advises
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
#include <errno.h>
#include <bits.h>
#include <vmm.h>
#include <thread.h>
#include <task.h>
#include <cluster.h>
#include <page.h>
#include <ppm.h>
#include <pmm.h>

/* TODO: don't use locks to lookup region as if address has been accessed it must be in kyesdb */
static error_t check_args(struct vmm_s *vmm, uint_t start, uint_t len, struct vm_region_s **reg)
{
	error_t err;
	struct vm_region_s *region;
  
	err = 0;
	rwlock_rdlock(&vmm->rwlock);

	region = vmm->last_region;
  
	if((start >= region->vm_limit) || (start < region->vm_start))
	{
		region = vm_region_find(vmm, start);
  
		if((region == NULL) || (start < region->vm_start))
			err = EINVAL;
	}

	if((err == 0) && ((start + len) >= region->vm_limit))
		err = EINVAL;
  
	rwlock_unlock(&vmm->rwlock);

	*reg = region;
	return 0;
}


/* TODO: compute other advices */
int sys_madvise(void *start, size_t length, uint_t advice)
{
	error_t err;
	struct vmm_s *vmm;
	struct vm_region_s *region;
	struct thread_s *this;

	err = 0;
	this = current_thread;

	if((start == NULL) || ((uint_t)start & PMM_PAGE_MASK))
	{
		err = EINVAL;
		goto SYS_MADVISE_ERR;
	}

	if(NOT_IN_USPACE((uint_t)start + length))
	{
		err = EPERM;
		goto SYS_MADVISE_ERR;
	}
   
	vmm = &this->task->vmm;
	err = check_args(vmm, (uint_t)start, length, &region);
  
	if(err) goto SYS_MADVISE_ERR;
  
	if((region->vm_flags & VM_REG_DEV) || (region->vm_flags & VM_REG_SHARED))
		return 0;

	switch(advice)
	{
	case MADV_NORMAL:
	case MADV_RANDOM:
	case MADV_SEQUENTIAL:
	case MADV_WILLNEED:
		err = vmm_madvise_willneed(vmm, (uint_t)start, (uint_t)length);
		break;
	case MADV_DONTNEED:
		return 0;
    
	case MADV_MIGRATE:
		err = vmm_madvise_migrate(vmm, (uint_t)start, (uint_t)length);
		break;
  
	default:
		err = EINVAL;
	}

SYS_MADVISE_ERR:
	this->info.errno = err;
	return err;
}
