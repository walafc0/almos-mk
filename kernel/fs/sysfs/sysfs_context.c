 /*
 * sysfs/sysfs_context.c - sysfs context related operations
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

#include <sysfs.h>
#include <sysfs-private.h>

sysfs_entry_t sysfs_root_entry;

VFS_CREATE_CONTEXT(sysfs_create_context)
{ 
	context->ctx_type    = VFS_SYSFS_TYPE;
	VFS_SET(context->ctx_flags, VFS_FS_LOCAL);
	context->ctx_op      = (struct vfs_context_op_s *) &sysfs_ctx_op;
	context->ctx_inode_op = (struct vfs_inode_op_s *) &sysfs_i_op;
	context->ctx_file_op = (struct vfs_file_op_s *) &sysfs_f_op;
	return 0;
}

VFS_DESTROY_CONTEXT(sysfs_destroy_context)
{
	return 0;
}

VFS_READ_ROOT(sysfs_read_root)
{ 
	struct sysfs_node_s *info;
  
	root->i_links = 1;
	root->i_size  = 0;
	root->i_attr  = VFS_DIR;
	root->i_number = vfs_inum_new(context);

	//info has been allocated by sysfs_init
	info         = root->i_pv;
	info->node   = &sysfs_root_entry.node;
  
	return 0;
}

VFS_REPLI_ROOT(sysfs_repli_root)
{ 
	struct sysfs_node_s *info;

	info         = root->i_pv;
	info->node   = &sysfs_root_entry.node;
  
	return 0;
}

VFS_WRITE_ROOT(sysfs_write_root)
{
	return 0;
}

const struct vfs_context_op_s sysfs_ctx_op = 
{
	.create     = sysfs_create_context,
	.destroy    = sysfs_destroy_context,
	.read_root  = sysfs_read_root,
	.repli_root = sysfs_repli_root,
	.write_root = sysfs_write_root
};
