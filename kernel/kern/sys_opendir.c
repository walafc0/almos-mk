/*
 * kern/sys_opendir.c - open a directory
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

#include <vfs.h>
#include <sys-vfs.h>
#include <task.h>
#include <thread.h>
#include <spinlock.h>

int sys_opendir (char *pathname)
{
	uint_t mode;
	error_t err;
	uint_t fd;
	struct vfs_file_s *file;
	struct thread_s *this;
	struct ku_obj ku_path;
	struct task_s *task;
   
	file = NULL;
	this = current_thread;
	task = current_task;
	mode = 0;
   
	if((err = task_fd_get(task, &fd, &file)))
	{
		this->info.errno = ENFILE;
		return -1;
	}

	KU_BUFF(ku_path, pathname);
	rwlock_rdlock(&task->cwd_lock);

	if((err = vfs_opendir(&task->vfs_cwd, &ku_path, mode, file)))
	{
		rwlock_unlock(&task->cwd_lock);
		this->info.errno = (err < 0 ) ? -err : err;
		task_fd_put(task,fd);
		return -1;
	}
   
	rwlock_unlock(&task->cwd_lock);

	return fd;
}
