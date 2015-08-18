/*
 * devfs/devfs_node.c - devfs node related operations
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
#include <device.h>
#include <vfs.h>
#include <cpu.h>

#include <devfs.h>
#include <devfs-private.h>



VFS_INIT_NODE(devfs_init_node)
{
	inode->i_pv     = NULL;
	return 0;
}


VFS_RELEASE_NODE(devfs_release_node)
{
	inode->i_pv = NULL;
	return 0;
}


VFS_CREATE_NODE(devfs_create_node)
{ 
	return ENOTSUPPORTED;
}

error_t devfs_inode_create(struct vfs_dirent_s *dirent, 
				struct vfs_inode_s *parent,
				struct device_s *dev, size_t size)
{
	struct vfs_inode_s *inode;

	inode = vfs_inode_new(dirent->d_ctx);//allocate inode
	if(!inode) return ENOMEM;

	inode->i_links  = 1;
	inode->i_attr = dirent->d_attr;
	inode->i_pv = (struct device_s*) dev;
	inode->i_size = size;
	inode->i_parent = parent;
	inode->i_pcid = cpu_get_cid();
	inode->i_number = vfs_inum_new(dirent->d_ctx);

	vfs_icache_add(inode);

	VFS_INODE_REF_SET(dirent->d_inode, inode, cpu_get_cid());

	return 0;
}

VFS_LOOKUP_NODE(devfs_lookup_node)
{
	register struct metafs_s *meta_node;
	register struct device_s *dev;
	register error_t err;
	dev_params_t params;

	if(!(parent->i_attr & VFS_DIR))
		return ENOTDIR;
  

	if((meta_node = metafs_lookup(&devfs_db.root, dirent->d_name)) == NULL)
		return VFS_NOT_FOUND;

	dev = metafs_container(meta_node, struct device_s, node);
  
	if((err=dev->op.dev.get_params(dev, &params)))
	{
		printk(ERROR,"ERROR: devfs_lookup_node: error %d while getting device parameters\n", err);
		return err;
	}
  
	switch(dev->type)
	{
	case DEV_BLK:
		dirent->d_attr |= VFS_DEV_BLK;
		break;
	case DEV_CHR:
		dirent->d_attr |= VFS_DEV_CHR;
		break;
	default:
		dirent->d_attr |= VFS_DEV;
	}

	//Do a list of inode within the device struct to avoid 
	//creating each time a new inode.
	devfs_inode_create(dirent, parent, dev, params.size);

	return VFS_FOUND;
}

VFS_STAT_NODE(devfs_stat_node)
{
	struct device_s *dev;
	uint_t mode;

	dev       = (struct device_s*)inode->i_pv;
	mode      = 0;

	inode->i_stat.st_dev     = (uint_t)dev;
	inode->i_stat.st_ino     = (uint_t)dev;
	inode->i_stat.st_nlink   = inode->i_links;
	inode->i_stat.st_uid     = 0;
	inode->i_stat.st_gid     = 0;
	inode->i_stat.st_rdev    = VFS_DEVFS_TYPE;
	inode->i_stat.st_size    = inode->i_size;
	inode->i_stat.st_blksize = 0;
	inode->i_stat.st_blocks  = 0;
	inode->i_stat.st_atime   = 0;
	inode->i_stat.st_mtime   = 0;
	inode->i_stat.st_ctime   = 0;

	if(inode->i_attr & VFS_DIR)
	{
		VFS_SET(mode, VFS_IFDIR);
	}
	else if(inode->i_attr & VFS_FIFO)
	{
		VFS_SET(mode, VFS_IFIFO);
	}
	else if(inode->i_attr & VFS_PIPE)
	{
		VFS_SET(mode, VFS_IFIFO);
	}
	else if(inode->i_attr & VFS_DEV_CHR)
	{
		VFS_SET(mode, VFS_IFCHR);
	}
	else if(inode->i_attr & VFS_DEV_BLK)
	{
		VFS_SET(mode, VFS_IFBLK);
	}
 
	inode->i_stat.st_mode = mode;
	return 0;
}


VFS_WRITE_NODE(devfs_write_node)
{
	return 0;
}

VFS_UNLINK_NODE(devfs_unlink_node)
{
	return ENOTSUPPORTED;
}

const struct vfs_inode_op_s devfs_i_op = 
{
	.init    = devfs_init_node,
	.create  = devfs_create_node,
	.lookup  = devfs_lookup_node,
	.write   = devfs_write_node,
	.release = devfs_release_node,
	.unlink  = devfs_unlink_node,
	.stat    = devfs_stat_node
};
