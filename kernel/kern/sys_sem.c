/*
 * kern/sys_sem.c - interface to access semaphore service
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

static inline bool_t isBadSem(struct semaphore_s *sem)
{ 
	return vmm_check_object(sem, struct semaphore_s, SEMAPHORE_ID);
}

int sys_sem(struct semaphore_s **sem, uint_t operation, uint_t pshared, int *value)
{
	kmem_req_t req;
	struct semaphore_s *isem;
	struct task_s *task;
	error_t err = EINVAL;
	int val;

	task = current_task;

	if((err = vmm_check_address("usr sem ptr", task, sem, sizeof(struct semaphore_s*))))
		goto SYS_SEM_END;

	if((err = cpu_copy_from_uspace(&isem, sem, sizeof(struct semaphore_s *))))
		goto SYS_SEM_END;
  
	switch(operation)
	{
	case SEM_INIT:
		if((err = vmm_check_address("usr sem op", task, value, sizeof(int*))))
			break;
    
		if((err = cpu_copy_from_uspace(&val, value, sizeof(int*))))
			break;

		req.type  = KMEM_SEM;
		req.size  = sizeof(*isem);
		req.flags = AF_USER;

		if((isem = kmem_alloc(&req)) == NULL)
		{
			err = ENOMEM;
			break;
		}
    
		if((err = sem_init(isem, val, SEM_SCOPE_USR)))
			break;
    
		if((err = cpu_copy_to_uspace(sem,&isem,sizeof(struct semaphore_s*))))
		{
			req.ptr = isem;
			kmem_free(&req);
			break;
		}
		return 0;

	case SEM_GETVALUE:
		if(isBadSem(isem))
			break;
    
		if((err = vmm_check_address("usr sem val", task, value, sizeof(int*))))
			break;

		if((err = sem_getvalue(isem, &val)))
			break;

		if((err = cpu_copy_to_uspace(value, &val, sizeof(int))))
			break;
    
		return 0;
    
	case SEM_WAIT:
		if(isBadSem(isem))
			break;
    
		if((err = sem_wait(isem)))
			break;

		return 0;

	case SEM_TRYWAIT:
		if(isBadSem(isem))
			break;
    
		return sem_trywait(isem);

	case SEM_POST:
		if(isBadSem(isem))
			break;
    
		if((err=sem_post(isem)))
			break;

		return 0;

	case SEM_DESTROY:
		if(isBadSem(isem))
			break;
    
		if((err = sem_destroy(isem)))
			break;
    
		req.type = KMEM_SEM;
		req.ptr  = isem;
		kmem_free(&req);
		return 0;
    
	default:
		err = EINVAL;
	}

SYS_SEM_END:
	current_thread->info.errno = err;
	return -1;
}

KMEM_OBJATTR_INIT(sem_kmem_init)
{
	attr->type   = KMEM_SEM;
	attr->name   = "KCM Semaphore";
	attr->size   = sizeof(struct semaphore_s);
	attr->aligne = 0;
	attr->min    = CONFIG_SEMAPHORE_MIN;
	attr->max    = CONFIG_SEMAPHORE_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}
