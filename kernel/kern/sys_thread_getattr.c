/*
 * kern/sys_thread_getattr.c - gets current thread's attributes
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
#include <cpu.h>
#include <cluster.h>
#include <thread.h>

int sys_thread_getattr(pthread_attr_t *attr)
{
	struct thread_s *this;
	struct cpu_s *cpu;
	error_t err;

	this = current_thread;
	cpu  = current_cpu;
	
	if(NOT_IN_USPACE((uint_t)attr))
	{
		err = EPERM;
		goto fail_attr;
	}

	this->info.attr.cid     = cpu->cluster->id;
	this->info.attr.cpu_lid = cpu->lid;
	this->info.attr.cpu_gid = cpu->gid;
  
	err = cpu_copy_to_uspace(attr, &this->info.attr,
				 sizeof(pthread_attr_t));

fail_attr:
	this->info.errno = err;
	return err;
}
