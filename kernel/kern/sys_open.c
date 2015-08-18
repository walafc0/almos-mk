/*
 * kern/sys_open.c - open a named file
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

#include <errno.h>
#include <thread.h>
#include <vfs.h>
#include <sys-vfs.h>
#include <task.h>
#include <spinlock.h>
#include <cpu-trace.h>


int sys_open (char *pathname, uint_t flags, uint_t mode)
{
	struct thread_s *this;
	register error_t err = 0;
	struct vfs_file_s *file;   
	struct ku_obj ku_path;
	struct task_s *task;
	uint_t fd = 0;

	file = NULL;
	this = current_thread;
	cpu_trace_write(current_cpu, sys_open);
	task = current_task;

	if((err = task_fd_get(task, &fd, &file)))
	{
		this->info.errno = ENFILE;
		return -1;
	}
   
	if(VFS_IS(flags, VFS_O_DIRECTORY))
		VFS_SET(flags, VFS_DIR);

	KU_BUFF(ku_path, pathname);
	rwlock_rdlock(&task->cwd_lock);
   
	if((err = vfs_open(&task->vfs_cwd, &ku_path, flags, mode, file)))
	{
		rwlock_unlock(&task->cwd_lock);
		this->info.errno = (err < 0 ) ? -err : err;
		task_fd_put(task, fd);
		return -1;
	}
   
	rwlock_unlock(&task->cwd_lock);

	return fd;
}
