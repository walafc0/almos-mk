/*
 * devfs/devfs_context.c - devfs context related operations
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

#include <device.h>
#include <driver.h>
#include <kmem.h>
#include <string.h>
#include <vfs.h>
#include <errno.h>
#include <metafs.h>
#include <cpu.h>

#include <devfs.h>
#include <devfs-private.h>


struct devfs_db_s devfs_db;

void devfs_root_init(void)
{
	spinlock_init(&devfs_db.lock, "DevFs DB");

	metafs_init(&devfs_db.root, 
#if CONFIG_ROOTFS_IS_VFAT
		    "DEV"
#else
		    "dev"
#endif
		);
}

void devfs_register(struct device_s *dev)
{
	spinlock_lock(&devfs_db.lock);
	metafs_register(&devfs_db.root, &dev->node);
	spinlock_unlock(&devfs_db.lock);
}


VFS_CREATE_CONTEXT(devfs_create_context)
{
	struct devfs_context_s *ctx;
  
	VFS_SET(context->ctx_flags, VFS_FS_LOCAL);
	context->ctx_type    = VFS_DEVFS_TYPE;
	context->ctx_op      = (struct vfs_context_op_s *) &devfs_ctx_op;
	context->ctx_inode_op = (struct vfs_inode_op_s *) &devfs_i_op;
	context->ctx_file_op = (struct vfs_file_op_s *) &devfs_f_op;

	ctx	= &context->ctx_devfs;
	ctx->db = &devfs_db.root;

	return 0;
}

VFS_DESTROY_CONTEXT(devfs_destroy_context)
{
	return 0;
}

VFS_READ_ROOT(devfs_read_root)
{ 
	root->i_links = 1;
	root->i_size  = 0;
	root->i_attr  = VFS_DIR;
	root->i_number = vfs_inum_new(context);
  
	return 0;
}

VFS_WRITE_ROOT(devfs_write_root)
{
	return 0;
}

KMEM_OBJATTR_INIT(devfs_kmem_context_init)
{
	attr->type   = KMEM_DEVFS_CTX;
	attr->name   = "KCM DevFs CTX";
	attr->size   = sizeof(struct devfs_context_s);
	attr->aligne = 0;
	attr->min    = CONFIG_DEVFS_CTX_MIN;
	attr->max    = CONFIG_DEVFS_CTX_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}


const struct vfs_context_op_s devfs_ctx_op = 
{
	.create     = devfs_create_context,
	.destroy    = devfs_destroy_context,
	.read_root  = devfs_read_root,
	.repli_root = NULL,
	.write_root = devfs_write_root
};
