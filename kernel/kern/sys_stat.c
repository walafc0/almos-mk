/*
 * kern/sys_stat.c - stats a file or directory
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


int sys_stat(char *pathname, struct vfs_stat_s *buff, int fd)
{
	struct thread_s *this;
	register error_t err = 0;
	struct vfs_file_s *file;
	struct ku_obj ku_path;
	struct task_s *task;

	file = NULL;
	this = current_thread;
	task = current_task;

	if((buff == NULL) || ((pathname == NULL) && (fd == -1)))
	{
		this->info.errno = EINVAL;
		return -1;
	}

	if(NOT_IN_USPACE((uint_t)buff))
	{
		this->info.errno = EPERM;
		return -1;
	}

	if(pathname == NULL)
	{
		if((fd >= CONFIG_TASK_FILE_MAX_NR) || (task_fd_lookup(task, fd, &file)))
			return EBADFD;
 
		err = vfs_stat(&task->vfs_cwd, NULL, buff, file);
	}
	else
	{
		KU_BUFF(ku_path, pathname);
		rwlock_rdlock(&task->cwd_lock);
		err = vfs_stat(&task->vfs_cwd, &ku_path, buff, NULL);
		rwlock_unlock(&task->cwd_lock);
	}
 
	this->info.errno = err;
	return -1;
}
