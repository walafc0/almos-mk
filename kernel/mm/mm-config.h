/*
 * mm/mm-config.h - memory-management related configurations
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

#ifndef _MM_CONFIG_H_
#define _MM_CONFIG_H_

#ifndef _CONFIG_H_
#error This config-file is not to be included directely, use config.h instead
#endif


//////////////////////////////////////////////
//     MEMORY MANAGEMENT CONFIGURATIONS     // 
//////////////////////////////////////////////
#define CONFIG_PPM_MAX_ORDER          10
#define CONFIG_PPM_URGENT_PGMIN       5
#define CONFIG_PPM_KPRIO_PGMIN        15
#define CONFIG_PPM_UPRIO_PGMIN        80
#define CONFIG_KHEAP_ORDER            7
#define CONFIG_VM_REGION_KEYWIDTH     16
#define CONFIG_DMA_RQ_KCM_MIN         2
#define CONFIG_DMA_RQ_KCM_MAX         4
#define CONFIG_TASK_KCM_MIN           2
#define CONFIG_TASK_KCM_MAX           3
#define CONFIG_FDINFO_KCM_MIN         2
#define CONFIG_FDINFO_KCM_MAX         2
#define CONFIG_DEVFS_CTX_MIN          1
#define CONFIG_DEVFS_CTX_MAX          1
#define CONFIG_DEVFS_FILE_MIN         2
#define CONFIG_DEVFS_FILE_MAX         3
#define CONFIG_DEVFS_NODE_MIN         1
#define CONFIG_DEVFS_NODE_MAX         2
#define CONFIG_VFAT_CTX_MIN           1
#define CONFIG_VFAT_CTX_MAX           1
#define CONFIG_VFAT_FILE_MIN          3
#define CONFIG_VFAT_FILE_MAX          3
#define CONFIG_VFAT_NODE_MIN          2
#define CONFIG_VFAT_NODE_MAX          2
#define CONFIG_EXT2_CTX_MIN           1
#define CONFIG_EXT2_CTX_MAX           1
#define CONFIG_EXT2_FILE_MIN          3
#define CONFIG_EXT2_FILE_MAX          3
#define CONFIG_EXT2_NODE_MIN          2
#define CONFIG_EXT2_NODE_MAX          2
#define CONFIG_SYSFS_FILE_MIN         1
#define CONFIG_SYSFS_FILE_MAX         2
#define CONFIG_SYSFS_NODE_MIN         1
#define CONFIG_SYSFS_NODE_MAX         2
#define CONFIG_VFS_CTX_MIN            1
#define CONFIG_VFS_CTX_MAX            1
#define CONFIG_VFS_INODE_MIN          3
#define CONFIG_VFS_INODE_MAX          6
#define CONFIG_VFS_FILE_MIN           3
#define CONFIG_VFS_FILE_MAX           5
#define CONFIG_SEMAPHORE_MIN          2
#define CONFIG_SEMAPHORE_MAX          2
#define CONFIG_CONDTION_VAR_MIN       2
#define CONFIG_CONDTION_VAR_MAX       2
#define CONFIG_BARRIER_MIN            2
#define CONFIG_BARRIER_MAX            2
#define CONFIG_RWLOCK_MIN             2
#define CONFIG_RWLOCK_MAX             2
#define CONFIG_WAITQUEUEDB_MAX        2
#define CONFIG_WAITQUEUEDB_MIN        2
#define CONFIG_MAPPER_MIN             CONFIG_VFS_INODE_MIN
#define CONFIG_MAPPER_MAX             CONFIG_VFS_INODE_MAX
#define CONFIG_RADIX_NODE_MIN         10         
#define CONFIG_RADIX_NODE_MAX         30
#define CONFIG_VM_REGION_MIN          1
#define CONFIG_VM_REGION_MAX          2
#define CONFIG_BLKIO_MIN              4
#define CONFIG_BLKIO_MAX              8
#define CONFIG_KEYREC_MIN             2
#define CONFIG_KEYREC_MAX             2
//////////////////////////////////////////////

#endif	/* _MM_CONFIG_H_ */
