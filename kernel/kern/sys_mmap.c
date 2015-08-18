/*
 * kern/sys_mmap.c - map files, memory or devices into process's 
 * virtual address space
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
#include <vmm.h>

int sys_mmap(mmap_attr_t *mattr)
{
	error_t err;
	uint_t count;
	struct thread_s *this;
	struct task_s *task;
	struct vfs_file_s *file;
	mmap_attr_t attr;
	size_t isize;
	int retval;
 
	this = current_thread;
	task = this->task;
	err  = EINVAL;
	file = NULL;

	if((err = cpu_copy_from_uspace(&attr, mattr, sizeof(mmap_attr_t))))
	{
		printk(INFO, "%s: failed, copying from uspace @%x\n",
		       __FUNCTION__, 
		       mattr);

		this->info.errno = EFAULT;
		return (int)VM_FAILED;
	}

	if((attr.flags  & VM_REG_HEAP)                     ||
	   ((attr.flags & VM_REG_PVSH) == VM_REG_PVSH)     || 
	   ((attr.flags & VM_REG_PVSH) == 0)               ||
	   (attr.length == 0)                              ||
	   (attr.offset & PMM_PAGE_MASK)                   || 
	   ((attr.addr != NULL) && (((uint_t)attr.addr & PMM_PAGE_MASK)          ||
				(NOT_IN_USPACE(attr.length + (uint_t)attr.addr)) ||
				(NOT_IN_USPACE((uint_t)attr.addr)) )))
	{
		printk(INFO, "%s: failed, we don't like flags (%x), length (%d), or addr (%x)\n",
		       __FUNCTION__, 
		       attr.flags, 
		       attr.length, 
		       attr.addr);
     
		this->info.errno = EINVAL;
		return (int)VM_FAILED;
	}
   
	if(attr.flags & VM_REG_ANON)
	{
		attr.offset = 0;
		attr.addr   = (attr.flags & VM_REG_FIXED) ? attr.addr : NULL;
	}
	else
	{      
		/* FIXEME: possible concurent delete of file from another bugy thread closing it */
		if((attr.fd >= CONFIG_TASK_FILE_MAX_NR) || (task_fd_lookup(task, attr.fd, &file)))
		{
			printk(INFO, "%s: failed, bad file descriptor (%d)\n", 
			       __FUNCTION__, 
			       attr.fd);

			this->info.errno = EBADFD;
			return (int)VM_FAILED;
		}
     
		//atomic_add(&file->f_count, 1);
		vfs_file_up(file);//FIXME coalsce access to remote node info
     
		//FIXME: does we really to get the size...
		isize = vfs_inode_size_get_remote(file->f_inode.ptr, file->f_inode.cid);
		if((attr.offset + attr.length) > isize)
		{
			printk(INFO, "%s: failed, offset (%d) + len (%d) >= file's size (%d)\n", 
			       __FUNCTION__, 
			       attr.offset, 
			       attr.length, 
			       isize);

			this->info.errno = ERANGE;
			goto SYS_MMAP_FILE_ERR;
		}

		if(((attr.prot & VM_REG_RD) && !(VFS_IS(file->f_flags, VFS_O_RDONLY)))   ||
		   ((attr.prot & VM_REG_WR) && !(VFS_IS(file->f_flags, VFS_O_WRONLY)))   ||
		   ((attr.prot & VM_REG_WR) && (VFS_IS(file->f_flags, VFS_O_APPEND))))//    ||
			//(!(attr.prot & VM_REG_RD) && (attr.flags & VM_REG_PRIVATE)))
		{
			printk(INFO, "%s: failed, EACCES prot (%x), f_flags (%x)\n", 
			       __FUNCTION__, 
			       attr.prot, 
			       file->f_flags);

			this->info.errno = EACCES;
			goto SYS_MMAP_FILE_ERR;
		}
	}

	retval = (int) vmm_mmap(task,
				file,
				attr.addr,
				attr.length,
				attr.prot,
				attr.flags,
				attr.offset);
   
	if((retval != (int)VM_FAILED) || (attr.flags & VM_REG_ANON))
		return retval;

SYS_MMAP_FILE_ERR:
	printk(INFO, "%s: Failed, Droping file count \n", 
	       __FUNCTION__);
   
	vfs_close(file, &count);

	if(count == 1)
		task_fd_put(task, attr.fd);

	return (int)VM_FAILED;
}
