/*
 * sys_cond_var: interface to access condition vraibles service
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
#include <semaphore.h>
#include <cond_var.h>

static inline bool_t isBadSem(struct semaphore_s *sem)
{ 
	return vmm_check_object(sem, struct semaphore_s, SEMAPHORE_ID);
}

static inline bool_t isBadCV(struct cv_s *cv)
{ 
	return vmm_check_object(cv, struct cv_s, COND_VAR_ID);
}

int sys_cond_var(struct cv_s **cv, uint_t operation, struct semaphore_s **sem)
{
	kmem_req_t req;
	struct cv_s *icv;
	struct semaphore_s *isem;
	error_t err = EINVAL;
 
	if((err = vmm_check_address("usr cv ptr", current_task,cv,sizeof(struct cv_s*))))
		goto SYS_COND_END;

	if((err = cpu_copy_from_uspace(&icv, cv, sizeof(struct cv_s *))))
		goto SYS_COND_END;

	switch(operation)
	{
	case CV_INIT:
		req.type  = KMEM_CV;
		req.size  = sizeof(*icv);
		req.flags = AF_USER;
    
		if((icv = kmem_alloc(&req)) == NULL)
		{
			err = ENOMEM;
			break;
		}
    
		if((err = cv_init(icv)))
			break;
    
		if((err = cpu_copy_to_uspace(cv, &icv, sizeof(struct cv_s *))))
		{
			req.ptr = icv;
			kmem_free(&req);
		}

		break;

	case CV_WAIT:
		err = vmm_check_address("usr sem ptr", 
					current_task, 
					sem, sizeof(struct semaphore_s*));

		if(err) break;

		if((err = cpu_copy_from_uspace(&isem, sem, sizeof(struct semaphore_s *))))
			break;

		if(isBadSem(isem))
			break;

		if(isBadCV(icv))
			break;
    
		return cv_wait(icv, isem);

	case CV_SIGNAL:
		if(isBadCV(icv))
			break;
    
		return cv_signal(icv);

	case CV_BROADCAST:
		if(isBadCV(icv))
			break;
    
		return cv_broadcast(icv);

	case CV_DESTROY:
		if(isBadCV(icv))
			break;
    
		if((err = cv_destroy(icv)))
			break;
    
		req.type = KMEM_CV;
		req.ptr  = icv;
		kmem_free(&req);
		return 0;

	default:
		err = EINVAL;
	}

SYS_COND_END:
	current_thread->info.errno = err;
	return err;
}

KMEM_OBJATTR_INIT(cv_kmem_init)
{
	attr->type   = KMEM_CV;
	attr->name   = "KCM Condition Variable";
	attr->size   = sizeof(struct cv_s);
	attr->aligne = 0;
	attr->min    = CONFIG_CONDTION_VAR_MIN;
	attr->max    = CONFIG_CONDTION_VAR_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
  
	return 0;
}
