/*
 * sysfs/sysfs_node.c - sysfs node related operations
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
#include <kmem.h>
#include <string.h>
#include <kdmsg.h>
#include <vfs.h>
#include <cpu.h>

#include <sysfs.h>
#include <sysfs-private.h>

KMEM_OBJATTR_INIT(sysfs_kmem_node_init)
{
	attr->type   = KMEM_SYSFS_NODE;
	attr->name   = "KCM SysFs Node";
	attr->size   = sizeof(struct sysfs_node_s);
	attr->aligne = 0;
	attr->min    = CONFIG_SYSFS_NODE_MIN;
	attr->max    = CONFIG_SYSFS_NODE_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}


VFS_INIT_NODE(sysfs_init_node)
{
	register struct sysfs_node_s *node_info;
	kmem_req_t req;
  
	node_info = inode->i_pv;

	if(inode->i_pv == NULL)
	{
		req.type  = KMEM_SYSFS_NODE;
		req.size  = sizeof(*node_info);
		req.flags = AF_KERNEL;

		node_info = kmem_alloc(&req);
	}

	if(node_info == NULL)
		return ENOMEM;
  
	inode->i_pv = node_info;
	return 0;
}


VFS_RELEASE_NODE(sysfs_release_node)
{
	kmem_req_t req;

	req.type   = KMEM_SYSFS_NODE;
	req.ptr    = inode->i_pv;
	kmem_free(&req);
	inode->i_pv = NULL;
	return 0;
}


VFS_CREATE_NODE(sysfs_create_node)
{ 
	return ENOTSUPPORTED;
}


error_t sysfs_inode_create(struct vfs_dirent_s *dirent, 
				struct vfs_inode_s *parent,
				struct metafs_s *meta, size_t size)
{
	struct vfs_inode_s *inode;

	inode = vfs_inode_new(dirent->d_ctx);//allocate inode
	if(!inode) return ENOMEM;

	inode->i_links  = 1;
	inode->i_attr = dirent->d_attr;
	((struct sysfs_node_s *)inode->i_pv)->node = meta;
	inode->i_size = size;
	inode->i_parent = parent;
	inode->i_pcid = cpu_get_cid();
	inode->i_number = vfs_inum_new(dirent->d_ctx);
	
	vfs_icache_add(inode);

	dirent->d_inode.cid = cpu_get_cid();
	dirent->d_inode.ptr = inode;

	return 0;
}

VFS_LOOKUP_NODE(sysfs_lookup_node)
{
	register struct sysfs_node_s *parent_info;
	register struct metafs_s *meta;
	register uint_t hasChild;
	size_t size;
  
	size = 0;
	parent_info = parent->i_pv;
  
	sysfs_dmsg(1, "DEBUG: sysfs_lookup: %s, attr %x\n", dirent->d_name, dirent->d_attr);

	if((meta = metafs_lookup(parent_info->node, dirent->d_name)) == NULL)
		return VFS_NOT_FOUND;

	hasChild = metafs_hasChild(meta);

	if(hasChild && !(dirent->d_attr & VFS_DIR))
		return EISDIR;
  
	if(!(hasChild) && (dirent->d_attr & VFS_DIR))
		return ENOTDIR;

	if(hasChild)
		dirent->d_attr |= VFS_DIR;
	else
		size = SYSFS_BUFFER_SIZE;

	sysfs_inode_create(dirent, parent, meta, size);
  
	return VFS_FOUND;
}


VFS_WRITE_NODE(sysfs_write_node)
{
	return 0;
}

VFS_UNLINK_NODE(sysfs_unlink_node)
{
	return ENOTSUPPORTED;
}

const struct vfs_inode_op_s sysfs_i_op = 
{
	.init    = sysfs_init_node,
	.create  = sysfs_create_node,
	.lookup  = sysfs_lookup_node,
	.write   = sysfs_write_node,
	.release = sysfs_release_node,
	.unlink  = sysfs_unlink_node,
	.stat    = NULL
};
