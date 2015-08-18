/*
 * sys_rmdir: remove a directory
 *
 * Copyright (c) 2015 UPMC Sorbonne Universites
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
#include <vfs.h>
#include <sys-vfs.h>
#include <thread.h>
#include <task.h>


int sys_rmdir (char *pathname)
{
	register struct thread_s *this;
	register struct task_s *task;
	struct ku_obj ku_path;
	error_t err = 0;
	
	this = current_thread;
	task = current_task;

	if(!pathname)
	{
		this->info.errno = EINVAL;
		return -1;
	}

	KU_BUFF(ku_path, pathname);
	rwlock_wrlock(&task->cwd_lock);

	if((err = vfs_rmdir(&current_task->vfs_cwd, &ku_path)))
	{
		current_thread->info.errno = (err < 0) ? -err : err;
		return -1;
	}
	
	rwlock_unlock(&task->cwd_lock); 
	return 0;
}
