/*
 * kern/sys_rwlock.c - interface to access Read-Write locks service
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
#include <kmem.h>
#include <task.h>
#include <vmm.h>
#include <kmagics.h>
#include <rwlock.h>


static inline bool_t isBadLock(struct rwlock_s *rwlock)
{ 
	return vmm_check_object(rwlock, struct rwlock_s, RWLOCK_ID);
}

int sys_rwlock(struct rwlock_s **rwlock, uint_t operation)
{
	kmem_req_t req;
	struct rwlock_s *irwlock;
	error_t err;
  
	err = vmm_check_address("usr rwlock ptr", 
				current_task, 
				rwlock,
				sizeof(struct rwlock_s*));
	if(err)
		goto SYS_RWLOCK_END;

	if((err = cpu_copy_from_uspace(&irwlock, rwlock, sizeof(struct rwlock_s *))))
		goto SYS_RWLOCK_END;
  
	switch(operation)
	{    
	case RWLOCK_INIT:    
		req.type  = KMEM_RWLOCK;
		req.size  = sizeof(*irwlock);
		req.flags = AF_USER;

		if((irwlock = kmem_alloc(&req)) == NULL)
		{
			err = ENOMEM;
			break;
		}
    
		if((err = rwlock_init(irwlock)))
			break;
    
		if((err = cpu_copy_to_uspace(rwlock, &irwlock, sizeof(struct rwlock_s *))))
		{
			req.ptr = irwlock;
			kmem_free(&req);
		}
		break;

	case RWLOCK_WRLOCK:
		if(isBadLock(irwlock))
			break;
    
		return rwlock_wrlock(irwlock);

	case RWLOCK_RDLOCK:
		if(isBadLock(irwlock))
			break;
    
		return rwlock_rdlock(irwlock);

	case RWLOCK_TRYWRLOCK:
		if(isBadLock(irwlock))
			break;
    
		return rwlock_trywrlock(irwlock);

	case RWLOCK_TRYRDLOCK:
		if(isBadLock(irwlock))
			break;
    
		return rwlock_tryrdlock(irwlock);

	case RWLOCK_UNLOCK:
		if(isBadLock(irwlock))
			break;
    
		if((err = rwlock_unlock(irwlock)))
			break;

	case RWLOCK_DESTROY:
		if(isBadLock(irwlock))
			break;
    
		if((err = rwlock_destroy(irwlock)))
			break;

		req.type = KMEM_RWLOCK;
		req.ptr  = irwlock;
		kmem_free(&req);
		return 0;

	default:
		err = EINVAL;
	}

SYS_RWLOCK_END:
	current_thread->info.errno = err;
	return err;
}

KMEM_OBJATTR_INIT(rwlock_kmem_init)
{
	attr->type   = KMEM_RWLOCK;
	attr->name   = "KCM RWLOCK";
	attr->size   = sizeof(struct rwlock_s);
	attr->aligne = 0;
	attr->min    = CONFIG_RWLOCK_MIN;
	attr->max    = CONFIG_RWLOCK_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
  
	return 0;
}
