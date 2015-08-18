/*
 * fat32/fat32.h - export memory related function and context operations
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

#ifndef _FAT32_H_
#define _FAT32_H_

#include <kmem.h>
#include <rwlock.h>
#include <atomic.h>

extern const struct vfs_context_op_s vfat_ctx_op;

typedef uint32_t vfat_cluster_t;
typedef uint32_t vfat_sector_t;
typedef uint32_t vfat_offset_t;

struct vfat_context_s
{
	cid_t fat_cid;
	struct device_s *dev;
	struct rwlock_s lock;
	vfat_sector_t fat_begin_lba;
	vfat_sector_t fat_blk_count;
	uint32_t bytes_per_sector;
	uint32_t bytes_per_cluster;
	vfat_cluster_t cluster_begin_lba;
	uint32_t sectors_per_cluster;
	vfat_cluster_t rootdir_first_cluster;
	vfat_sector_t last_allocated_sector;
	vfat_sector_t last_allocated_index;
	struct mapper_s *mapper;
};

KMEM_OBJATTR_INIT(vfat_kmem_context_init);
KMEM_OBJATTR_INIT(vfat_kmem_inode_init);

struct vfat_dirent_s
{
	uint_t entry_index;
	uint_t first_cluster;
};

#endif	/* _FAT32_H_ */
