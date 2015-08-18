/*
 * sysfs/sysfs_file.c - sysfs file related operations
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
#include <device.h>
#include <driver.h>
#include <kmem.h>
#include <kdmsg.h>
#include <string.h>
#include <vfs.h>
#include <errno.h>
#include <metafs.h>
#include <thread.h>
#include <task.h>

#include <sysfs.h>
#include <sysfs-private.h>

KMEM_OBJATTR_INIT(sysfs_kmem_file_init)
{
	attr->type   = KMEM_SYSFS_FILE;
	attr->name   = "KCM SysFs File";
	attr->size   = sizeof(struct sysfs_file_s);
	attr->aligne = 0;
	attr->min    = CONFIG_SYSFS_FILE_MIN;
	attr->max    = CONFIG_SYSFS_FILE_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}


VFS_OPEN_FILE(sysfs_open)
{
	register struct sysfs_file_s *f_info;
	register struct sysfs_node_s *n_info;
	register sysfs_entry_t *entry;
	kmem_req_t req;

	n_info = file->fr_inode->i_pv;
	f_info = file->fr_pv;
  
	if((file->fr_pv == NULL))
	{
		req.type  = KMEM_SYSFS_FILE;
		req.size  = sizeof(*f_info);
		req.flags = AF_KERNEL;

		f_info    = kmem_alloc(&req);
	}

	if(f_info == NULL)
		return ENOMEM;

	f_info->node          = n_info->node;
	entry                 = metafs_container(f_info->node, sysfs_entry_t, node);
	f_info->entry         = entry;
	f_info->current_index = 0;
	file->fr_pv            = f_info;
  
	if(file->fr_inode->i_attr & VFS_DIR)
	{
		metafs_iter_init(f_info->node, &f_info->iter);
		return 0;
	}
  
	if(entry->op.open)
		return entry->op.open(entry, &f_info->rq, &file->fr_offset);
  
	return 0;
}

VFS_READ_FILE(sysfs_read)
{
	register struct sysfs_file_s *info;
	register uint_t size;
	register error_t err;

	//TODO: handle the case where the this fs is
	//accessed remotly  
	assert(file->f_inode.cid == current_cid);

	info = file->f_remote->fr_pv;

	if(info->entry->op.read == NULL)
		return ENOSYS;
  
	if(info->current_index == 0)
	{
		if((err = info->entry->op.read(info->entry, &info->rq, &file->f_remote->fr_offset)))
		{
			printk(INFO, "INFO: sysfs_read: error %d\n", err);
			return err;
		}
	}

	size = buffer->size;
	if(size > info->rq.count)
		size = info->rq.count;
   
	buffer->scpy_to_buff(buffer, &info->rq.buffer[info->current_index], size);
  
	info->rq.count     -= size;
	info->current_index = (info->rq.count) ? info->current_index + size : 0;

	return size;
}

VFS_WRITE_FILE(sysfs_write)
{
	return ENOTSUPPORTED;
}

VFS_LSEEK_FILE(sysfs_lseek)
{
	return ENOTSUPPORTED;
}

VFS_CLOSE_FILE(sysfs_close)
{
	return 0;
}

VFS_RELEASE_FILE(sysfs_release)
{  
	kmem_req_t req;
  
	if(file->fr_pv == NULL)
		return 0;
  
	req.type   = KMEM_SYSFS_FILE;
	req.ptr    = file->fr_pv;
	file->fr_pv = NULL;
	kmem_free(&req);

	return 0;
}

VFS_READ_DIR(sysfs_readdir)
{
	register struct sysfs_file_s *info;
	register struct metafs_s *current;
  
	info = file->fr_pv;

	if((current = metafs_lookup_next(info->node, &info->iter)) == NULL)
		return EEODIR;

	dirent->u_attr = (metafs_hasChild(current)) ? VFS_DIR : 0;
	strcpy((char*)dirent->u_name, metafs_get_name(current));
  
	//FIXME: better inode number
	dirent->u_ino = (uint_t) current;

	return 0;
}

const struct vfs_file_op_s sysfs_f_op = 
{
	.open    = sysfs_open,
	.read    = sysfs_read,
	.write   = sysfs_write,
	.lseek   = sysfs_lseek,
	.readdir = sysfs_readdir,
	.close   = sysfs_close,
	.release = sysfs_release,
};
