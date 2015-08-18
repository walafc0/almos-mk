/*
 * kern/sys_read.c - read bytes from an opened file
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

int sys_read (uint_t fd, void *buf, size_t count)
{
	struct ku_obj kub;
	ssize_t err;
	struct thread_s *this;
	struct task_s *task;
	struct vfs_file_s *file;

	file = NULL;
	this = current_thread;
	task = current_task;

	if((fd >= CONFIG_TASK_FILE_MAX_NR) || (task_fd_lookup(task, fd, &file)))
	{
		this->info.errno = EBADFD;
		return -1;
	}

	KU_SLICE_BUFF(kub, buf, count);
	if((err = vfs_read(file, &kub)) < 0)
	{
		this->info.errno = -err;
		printk(INFO, "INFO: sys_read: Error %d\n", this->info.errno);
		return -1;
	}
   
	return err;
}
