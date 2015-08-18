/*
 * devfs/devfs_file.c - devfs file related operations
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
#include <string.h>
#include <vfs.h>
#include <errno.h>
#include <metafs.h>
#include <thread.h>

#include <devfs.h>
#include <devfs-private.h>


KMEM_OBJATTR_INIT(devfs_kmem_file_init)
{
	attr->type   = KMEM_DEVFS_FILE;
	attr->name   = "KCM DevFs File";
	attr->size   = sizeof(struct devfs_file_s);
	attr->aligne = 0;
	attr->min    = CONFIG_DEVFS_FILE_MIN;
	attr->max    = CONFIG_DEVFS_FILE_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}

VFS_OPEN_FILE(devfs_open)
{
	register error_t err;
	register struct devfs_context_s *ctx;
	register struct devfs_file_s *info;
	register struct device_s *dev;
	struct vfs_inode_s *inode;
	dev_request_t rq;
	kmem_req_t req;

	inode = file->fr_inode;

	info = file->fr_pv;
	ctx  = (struct devfs_context_s *)&inode->i_ctx->ctx_devfs;

	if(!(inode->i_attr & VFS_DIR))
	{
		dev = (struct device_s*)inode->i_pv;
    
		if(dev->type == DEV_INTERNAL)
			return EPERM;

		if(dev->type == DEV_BLK)
			VFS_SET(inode->i_attr,VFS_DEV_BLK);
		else
			VFS_SET(inode->i_attr,VFS_DEV_CHR);
 
		if(dev->op.dev.open != NULL)
		{
			rq.fremote = file;
			if((err=dev->op.dev.open(dev, &rq)))
				return err;
		}

		priv->dev = (void*)dev;

		return 0;
	}

	if(info == NULL)
	{
		req.type  = KMEM_DEVFS_FILE;
		req.size  = sizeof(*info);
		req.flags = AF_KERNEL;
		info      = kmem_alloc(&req);
	}

	if(info == NULL) return ENOMEM;

	metafs_iter_init(&devfs_db.root, &info->iter);
	info->ctx  = ctx;
	file->fr_pv = info;
  
	metafs_print(&devfs_db.root);
	return 0;
}

#define TMP_BUFF_SZ 512
//FIXME:
//add a "while" loop for the case where the
//buffer TMP_BUFF_SZ is smaller than
//buffer->size
VFS_READ_FILE(devfs_read)
{
	register struct device_s *dev;
	dev_request_t rq;
	uint_t count;
        uint8_t buff[TMP_BUFF_SZ];

	dev = (struct device_s*)file->f_private.dev;

	rq.dst   = &buff[0];
	rq.count = TMP_BUFF_SZ;
	rq.flags = 0;
	rq.file  = file;

	if((count = dev->op.dev.read(dev, &rq)) < 0)
		return count;

        buffer->scpy_to_buff(buffer, &buff[0], count);
	return count;
}

//FIXME: To improve this an avoid the extra copy,
//we could set along with the buffer(src and dest) 
//the functions to manipulate them, such as in 
//do_exec.c
VFS_WRITE_FILE(devfs_write)
{
	register struct device_s *dev;
        uint8_t buff[TMP_BUFF_SZ];
	dev_request_t rq;
	
	dev = (struct device_s*)file->f_private.dev;
	
	//FIXME avoid the extra copy ?
	buffer->scpy_from_buff(buffer, (void*)&buff[0], TMP_BUFF_SZ);
	rq.src   = (void*)&buff[0];
	rq.count = buffer->size;
	rq.flags = 0;
	rq.file  = file;
  
	return dev->op.dev.write(dev, &rq);
}

VFS_LSEEK_FILE(devfs_lseek)
{
	register struct device_s *dev;
	dev_request_t rq;

	dev = (struct device_s*)file->fr_inode->i_pv;

	if(dev->op.dev.lseek == NULL)
		return VFS_ENOTUSED;
  
	rq.fremote = file;
	return dev->op.dev.lseek(dev, &rq);
}

VFS_CLOSE_FILE(devfs_close)
{
	register struct device_s *dev;
	dev_request_t rq;

	if(file->fr_inode->i_attr & VFS_DIR)
		return 0;

	dev = (struct device_s*)file->fr_inode->i_pv;

	if(dev->op.dev.close == NULL)
		return 0;
  
	rq.fremote = file;
	return dev->op.dev.close(dev, &rq);
}

VFS_RELEASE_FILE(devfs_release)
{  
	kmem_req_t req;

	if(file->fr_pv == NULL) 
		return 0;
  
	req.type = KMEM_DEVFS_FILE;
	req.ptr  = file->fr_pv;
	kmem_free(&req);

	file->fr_pv = NULL;
	return 0;
}

VFS_READ_DIR(devfs_readdir)
{
	register struct devfs_file_s *info;
	register struct metafs_s *current;
	register struct device_s *current_dev;
  
	info = file->fr_pv;
  
	if(file->fr_pv == NULL)
		return ENOTDIR;

	if((current = metafs_lookup_next(&devfs_db.root, &info->iter)) == NULL)
		return EEODIR;

	current_dev    = metafs_container(current, struct device_s, node);
	dirent->u_attr = (current_dev->type == DEV_BLK) ? VFS_DEV_BLK : VFS_DEV_CHR;

	strcpy((char*)dirent->u_name, metafs_get_name(current));

	dirent->u_ino = (uint_t) current_dev->base_paddr;

	return 0;
}

VFS_MMAP_FILE(devfs_mmap)
{
	register struct device_s *dev;
	dev_request_t rq;
  
	dev = (struct device_s*)file->f_private.dev;

	if(dev->op.dev.mmap == NULL)
		return ENODEV;

	rq.flags  = 0;
	rq.file   = file;
	rq.region = region;

	return dev->op.dev.mmap(dev, &rq);
}

VFS_MMAP_FILE(devfs_munmap)
{
	register struct device_s *dev;
	dev_request_t rq;

	dev = (struct device_s*)file->f_private.dev;

	if(dev->op.dev.munmap == NULL)
		return ENODEV;

	rq.flags  = 0;
	rq.file   = file;
	rq.region = region;

	return dev->op.dev.munmap(dev, &rq);
}


const struct vfs_file_op_s devfs_f_op = 
{
	.open    = devfs_open,
	.read    = devfs_read,
	.write   = devfs_write,
	.lseek   = devfs_lseek,
	.mmap    = devfs_mmap,
	.munmap  = devfs_munmap,
	.readdir = devfs_readdir,
	.close   = devfs_close,
	.release = devfs_release
};
