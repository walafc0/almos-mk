/*
 * devfs/devfs-private.h - devfs internal functions and data types
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

#ifndef _DEVFS_PRIVATE_H_
#define _DEVFS_PRIVATE_H_

#include <spinlock.h>
#include <metafs.h>

struct devfs_db_s
{
	spinlock_t lock;
	struct metafs_s root;
};

extern struct devfs_db_s devfs_db;

struct devfs_file_s
{
	struct metafs_iter_s iter;
	struct device_s *dev;
	struct devfs_context_s *ctx;
};

const struct vfs_inode_op_s devfs_i_op;
const struct vfs_file_op_s devfs_f_op;

#endif	/* _DEVFS_PRIVATE_H_ */
