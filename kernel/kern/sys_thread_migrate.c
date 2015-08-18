/*
 * kern/sys_thread_migrate.c - calls the scheduler to yield current CPU
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
#include <thread.h>
#include <task.h>
#include <scheduler.h>

#define MAX_CPU_NR (CONFIG_MAX_CLUSTER_NR * CONFIG_MAX_CPU_PER_CLUSTER_NR)

int sys_thread_migrate(pthread_attr_t *thread_attr)
{
	struct thread_s *this;
	pthread_attr_t attr;
	uint_t mode;
	sint_t cpu_gid;
	error_t err;

	this = current_thread;

	if(thread_attr == NULL)
	{
		err = EINVAL;
		goto fail_inval;
	}

	if(NOT_IN_USPACE((uint_t)thread_attr))
	{
		err = EPERM;
		goto fail_access;
	}

	if((err = cpu_copy_from_uspace(&attr, thread_attr, sizeof(attr))))
		goto fail_ucopy_attr;

	cpu_gid = (sint_t)cpu_get_id();

	if(attr.cpu_gid == cpu_gid)
	{
		thread_migration_disable(this);
		return 0;
	}

	if((attr.cpu_gid > 0) && (attr.cpu_gid >= MAX_CPU_NR))
	{
		err = EINVAL;
		goto fail_attr_inval;
	}

	cpu_gid = (attr.cpu_gid < 0) ? -1 : (attr.cpu_gid % arch_onln_cpu_nr());

	if(attr.flags & PT_ATTR_AUTO_MGRT)
		this->info.attr.flags |= PT_ATTR_AUTO_MGRT;

#if CONFIG_SHOW_SYSMGRT_MSG
	printk(INFO, "INFO: %s: pid %d, tid %d (%x), cpu %d: asked to migrate to cpu %d [%u]\n",
	       __FUNCTION__,
	       this->task->pid,
	       this->info.order,
	       this,
	       cpu_get_id(),
	       cpu_gid,
	       cpu_time_stamp());
#endif

	cpu_disable_all_irq(&mode);
	thread_clear_cap_migrate(this);
	cpu_restore_irq(mode);

	err = thread_migrate(this, cpu_gid);

	/* Reload this pointer */
	this = current_thread;

	cpu_disable_all_irq(&mode);
	thread_set_cap_migrate(this);
	thread_migration_deactivate(this);
	cpu_restore_irq(mode);

#if CONFIG_SHOW_SYSMGRT_MSG
	printk(INFO, "INFO: %s: pid %d, tid %d (%x), cpu %d, err %d, done [%u]\n", 
	       __FUNCTION__,
	       this->task->pid,
	       this->info.order,
	       current_thread,
	       cpu_get_id(),
	       err,
	       cpu_time_stamp());
#endif

fail_attr_inval:
fail_ucopy_attr:
fail_access:
fail_inval:
	this->info.errno = err;
	return (err) ? -1 : 0;
}
