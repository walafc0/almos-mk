/*
 * kern/sys_barrier.c - barrier service interface for userland
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
#include <thread.h>
#include <task.h>
#include <kmem.h>
#include <vmm.h>
#include <kmagics.h>
#include <barrier.h>

int sys_barrier(struct barrier_s **barrier, uint_t operation, uint_t count)
{
	kmem_req_t req;
	struct barrier_s *ibarrier;
	error_t err = EINVAL;
  
	if((err = vmm_check_address("usr barrier ptr", 
				    current_task, 
				    barrier,
				    sizeof(struct barrier_s*))))
		goto SYS_BARRIER_END;

	if((err = cpu_copy_from_uspace(&ibarrier, barrier, sizeof(struct barrier_s *))))
		goto SYS_BARRIER_END;

	switch(operation)
	{
	case BARRIER_INIT_PRIVATE:
	case BARRIER_INIT_SHARED:
		req.type  = KMEM_BARRIER;
		req.size  = sizeof(*ibarrier);
		req.flags = AF_USER;

		if((ibarrier = kmem_alloc(&req)) == NULL)
		{
			err = ENOMEM;
			break;
		}
    
		if((err = barrier_init(ibarrier, count, operation)))
			break;
    
		if((err = cpu_copy_to_uspace(barrier, &ibarrier, sizeof(struct barrier_s *))))
		{
			req.ptr = ibarrier;
			kmem_free(&req);
		}
		break;

	case BARRIER_WAIT:

		if((err = vmm_check_object(ibarrier, struct barrier_s, BARRIER_ID)))
			break;

		err = barrier_wait(ibarrier);
		break;

	case BARRIER_DESTROY:
    
		if((err = vmm_check_object(ibarrier, struct barrier_s, BARRIER_ID)))
			break;
    
		if((err = barrier_destroy(ibarrier)))
			break;
   
		req.type = KMEM_BARRIER;
		req.ptr  = ibarrier;
		kmem_free(&req);
		return 0;

	default:
		err = EINVAL;
	}

SYS_BARRIER_END:
	current_thread->info.errno = err;
	return err;
}

KMEM_OBJATTR_INIT(barrier_kmem_init)
{
	attr->type   = KMEM_BARRIER;
	attr->name   = "KCM Barrier";
	attr->size   = sizeof(struct barrier_s);
	attr->aligne = 0;
	attr->min    = CONFIG_BARRIER_MIN;
	attr->max    = CONFIG_BARRIER_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}
