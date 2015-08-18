/*
 * kern/sys_getcwd.c - get process current work directory
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

#include <libk.h>
#include <vfs.h>
#include <sys-vfs.h>
#include <task.h>
#include <kmem.h>
#include <ppm.h>
#include <thread.h>

/* TODO: user page need to be locked as long as its region */

int sys_getcwd (char *buff, size_t size)
{
	register struct thread_s *this;
	register struct task_s *task;
	register error_t err;
	struct ku_obj ku_buff;
  
	this      = current_thread;
	task      = current_task;

	if((size < VFS_MAX_NAME_LENGTH) || (!buff)) 
	{
		err = ERANGE;
		goto SYS_GETCWD_ERROR;
	}

	if(vmm_check_address("usr cwd buffer", task, buff, size))
	{
		err = EFAULT;
		goto SYS_GETCWD_ERROR;
	}

	KU_SZ_BUFF(ku_buff, buff, size);

	rwlock_rdlock(&task->cwd_lock);

	err = vfs_get_path(&task->vfs_cwd, &ku_buff);

	rwlock_unlock(&task->cwd_lock);


SYS_GETCWD_ERROR:
	this->info.errno = err;
	return (int)buff;
}
