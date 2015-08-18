/*
 * kern/sys_creat.c - create a file
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

#include <config.h>
#include <vfs.h>
#include <sys-vfs.h>
#include <spinlock.h>
#include <task.h>
#include <thread.h>

int sys_creat (char *pathname, uint_t mode)
{
	register error_t err = 0;
	register uint32_t flags;
	register struct thread_s *this;
	register struct task_s *task;
	struct vfs_file_s *file;
	struct ku_obj ku_path;
	uint_t fd = 0;

	this = current_thread;
	task = current_task;
   
	if((err = task_fd_get(task, &fd, &file)))
	{
		this->info.errno = ENFILE;
		return -1;
	}

	flags = 0;
	KU_BUFF(ku_path, pathname);
	rwlock_rdlock(&task->cwd_lock);

	if((err = vfs_create(&task->vfs_cwd, &ku_path, flags, mode, file)))
	{
		this->info.errno = (err < 0 ) ? -err : err;
		task_fd_put(task, fd);
		rwlock_unlock(&task->cwd_lock);
		return -1;
	}
	rwlock_unlock(&task->cwd_lock);

	return fd;
}
