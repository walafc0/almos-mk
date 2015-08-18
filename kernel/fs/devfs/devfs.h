/*
 * devfs/devfs.h - export devfs memory and context related operations
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

#ifndef _DEVFS_H_
#define _DEVFS_H_

struct device_s;

extern const struct vfs_context_op_s devfs_ctx_op;

struct devfs_context_s
{
	struct metafs_s *db;
};

void devfs_root_init(void);
void devfs_register(struct device_s *dev);

KMEM_OBJATTR_INIT(devfs_kmem_context_init);
KMEM_OBJATTR_INIT(devfs_kmem_file_init);


#endif	/* _DEVFS_H_ */
