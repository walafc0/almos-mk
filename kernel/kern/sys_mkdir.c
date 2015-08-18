/*
 * kern/sys_mkdir.c - creates new directory
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

int sys_mkdir (char *pathname, uint_t mode)
{
	register error_t err = 0;
	struct task_s *task = current_task;
	struct ku_obj ku_path;

	KU_BUFF(ku_path, pathname);
	rwlock_rdlock(&task->cwd_lock);

	if((err = vfs_mkdir(&task->vfs_cwd, &ku_path, mode)))
	{
		current_thread->info.errno = (err < 0) ? -err : err;
		rwlock_unlock(&task->cwd_lock);
		return -1;
	}
   
	rwlock_unlock(&task->cwd_lock);
	return 0;
}
