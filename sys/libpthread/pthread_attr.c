/*
 * pthread_attr.c - pthread attributes related functions
 * 
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS.
 *
 * ALMOS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <errno.h>
#include <sched.h>
#include <cpu-syscall.h>

#include <pthread.h>

#define PT_DEFAULT_ATTRS (PT_ATTR_AUTO_NXTT | PT_ATTR_AUTO_MGRT | PT_ATTR_MEM_PRIO)

int pthread_attr_init (pthread_attr_t *attr)
{
	attr->flags                      = PT_DEFAULT_ATTRS;
	attr->sched_policy               = SCHED_RR;
	attr->inheritsched               = PTHREAD_EXPLICIT_SCHED;
	attr->stack_addr                 = NULL;
	attr->stack_size                 = 0;//PTHREAD_STACK_SIZE;
	attr->sched_param.sched_priority = -1;
	attr->cpu_gid                    = -1;
	return 0;
}

int pthread_attr_destroy (pthread_attr_t *attr)
{
	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
	if(attr == NULL)
		return EINVAL;
  
	attr->flags = (detachstate == PTHREAD_CREATE_DETACHED) ? __PT_ATTR_DETACH : attr->flags;
	return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
	if((attr == NULL) || (detachstate == NULL))
		return EINVAL;
  
	*detachstate = (attr->flags & __PT_ATTR_DETACH) ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE;
	return 0;
}

int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy)
{
	if((attr = NULL) || (policy < 0) || (policy > 2))
		return EINVAL;

	attr->sched_policy = policy;
	return 0;
}

int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy)
{
	if((attr = NULL) || (policy == NULL))
		return EINVAL;

	*policy = attr->sched_policy;
	return 0;
}

int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param)
{
	if((attr = NULL) || (param == NULL))
		return EINVAL;
  
	attr->sched_param = *param;
	return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param)
{
	if((attr = NULL) || (param == NULL))
		return EINVAL;

	*param = attr->sched_param;
	return 0;
}

int pthread_attr_setinheritsched(pthread_attr_t *attr, int inherit)
{
	if((attr == NULL) || (inherit != PTHREAD_EXPLICIT_SCHED))
		return EINVAL;
  
	return 0;
}

int pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inherit)
{
	if((attr == NULL) || (inherit == NULL))
		return EINVAL;

	*inherit = PTHREAD_EXPLICIT_SCHED;
	return 0;
}

int pthread_attr_setscope(pthread_attr_t *attr, int scope)
{
	if((attr == NULL) || (scope != PTHREAD_SCOPE_SYSTEM))
		return EINVAL;

	return 0;
}

int pthread_attr_getscope(const pthread_attr_t *attr, int *scope)
{
	if((attr == NULL) || (scope == NULL))
		return 0;

	*scope = PTHREAD_SCOPE_SYSTEM;
	return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, unsigned int stacksize)
{
	if((attr == NULL) || (stacksize < PTHREAD_STACK_MIN))
		return EINVAL;

	attr->stack_size = stacksize;
	return 0;
}

int pthread_attr_getstacksize(pthread_attr_t *attr, size_t *stacksize)
{
	if((attr == NULL) || (stacksize == NULL))
		return EINVAL;

	*stacksize = attr->stack_size;
	return 0;
}

int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize)
{
	if((attr == NULL)                    ||
	   ((uint32_t)stackaddr & 0x3)       || 
	   (stacksize < PTHREAD_STACK_MIN))
		return EINVAL;

	attr->stack_addr = stackaddr;
	attr->stack_size = stacksize;
	return 0;
}

int pthread_attr_getstack(pthread_attr_t *attr, void **stackaddr, size_t *stacksize)
{
	if((attr == NULL)                  ||
	   (stackaddr == NULL)             || 
	   (stacksize == NULL))
		return EINVAL;

	*stackaddr = attr->stack_addr;
	*stacksize = attr->stack_size;
	return 0;
}

int pthread_attr_setflags_np(pthread_attr_t *attr, unsigned int flags, unsigned int *old)
{
	if(attr == NULL)
		return EINVAL;

	if(old != NULL)
		*old = attr->flags;

	attr->flags = flags;
	return 0;
}

int pthread_attr_setcpuid_np(pthread_attr_t * attr, int cpu_id, int *old_cpu_id)
{
	if(attr == NULL)
		return EINVAL;

	if(old_cpu_id)
		*old_cpu_id = attr->cpu_gid;

	attr->cpu_gid = cpu_id;
	return 0;
}

int pthread_attr_getcpuid_np(int *cpu_id)
{
	if(cpu_id == NULL) return EINVAL;

	*cpu_id = (int)(((__pthread_tls_t*)cpu_get_tls())->attr.cpu_gid);
	return 0;
}

int pthread_attr_setforkinfo_np(int flags)
{
	register __pthread_tls_t *tls;

	tls = cpu_get_tls();
	__pthread_tls_set(tls,__PT_TLS_FORK_FLAGS,flags);
	return 0;
}

int pthread_attr_setforkcpuid_np(int cpu_id)
{
	register __pthread_tls_t *tls;
	register uint_t flags;

	tls    = cpu_get_tls();
	flags  = __pthread_tls_get(tls,__PT_TLS_FORK_FLAGS);
	flags |= PT_FORK_TARGET_CPU;

	__pthread_tls_set(tls,__PT_TLS_FORK_FLAGS,flags);
	__pthread_tls_set(tls,__PT_TLS_FORK_CPUID,cpu_id);
	return 0;
}

int pthread_attr_getforkcpuid_np(int *cpu_id)
{
	register __pthread_tls_t *tls;
	register uint_t flags;

	if(cpu_id == NULL) 
		return EINVAL;

	tls   = cpu_get_tls();
	flags = __pthread_tls_get(tls,__PT_TLS_FORK_FLAGS);

	if(flags & PT_FORK_TARGET_CPU)
		*cpu_id = (int)__pthread_tls_get(tls,__PT_TLS_FORK_CPUID);
	else
		*cpu_id = -1;

	return 0;
}
