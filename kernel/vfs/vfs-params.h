/*
 * vfs/vfs-params.h - vfs flags and configurations
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

#ifndef _VFS_PARAMS_H_
#define _VFS_PARAMS_H_

#include <config.h>

//#define VFS_MAX_NAME_LENGTH      256//to use 256 fix uspace
#define VFS_MAX_NAME_LENGTH      255
#define VFS_MAX_PATH_DEPTH       12
#define VFS_MAX_PATH             VFS_MAX_NAME_LENGTH * VFS_MAX_PATH_DEPTH + 1
#define VFS_MAX_NODE_NUMBER      40
#define VFS_MAX_FILE_NUMBER      (CONFIG_TASK_FILE_MAX_NR)

#define VFS_DEBUG                CONFIG_VFS_DEBUG

#define VFS_O_PIPE               0x00010000
#define VFS_O_FIFO               0x00030000
#define VFS_O_DIRECTORY          0x00040000
#define VFS_O_APPEND             0x00080000
#define VFS_O_RDONLY             0x00100000
#define VFS_O_WRONLY             0x00200000
#define VFS_O_RDWR               0x00300000
#define VFS_O_CREATE             0x00400000
#define VFS_O_EXCL               0x00800000
#define VFS_O_TRUNC              0x01000000
#define VFS_O_MOUNTPOINT         0X02000000
#define VFS_O_DEV                0x04000000
#define VFS_O_SYNC	         0x08000000

typedef enum
{
	//VFS_EXT2_TYPE = 0,
	VFS_SYSFS_TYPE = 0,
	VFS_DEVFS_TYPE,
	VFS_VFAT_TYPE,
	VFS_PIPE_TYPE,
	VFS_TYPES_NR
}vfs_types_t;

#define VFS_SEEK_SET         0
#define VFS_SEEK_CUR         1
#define VFS_SEEK_END         2

#define VFS_FREE             1
#define VFS_INLOAD           2
#define VFS_DIRTY            4

//////////////////////////////////////
///    keep these flags compact    ///
//////////////////////////////////////
#define VFS_REGFILE          0x0000000
#define VFS_DIR              0x0000001
#define VFS_FIFO             0x0000002
#define VFS_DEV_CHR          0x0000004
#define VFS_DEV_BLK          0x0000008
#define VFS_DEV              0x000000C
#define VFS_SOCK             0x0000010
#define VFS_SYMLNK           0x0000020
//////////////////////////////////////

#define VFS_RD_ONLY          0x0000040
#define VFS_SYS              0x0000080
#define VFS_ARCHIVE          0x0000100
#define VFS_PIPE             0x0000200

#define VFS_IFMT	     0x0170000
#define VFS_IFSOCK	     0x0140000
#define VFS_IFLNK	     0x0120000
#define VFS_IFREG	     0x0100000
#define VFS_IFBLK	     0x0060000
#define VFS_IFDIR	     0x0040000
#define VFS_IFCHR	     0x0020000
#define VFS_IFIFO	     0x0010000

#define VFS_ISUID	     0x0004000
#define VFS_ISGID	     0x0002000
#define VFS_ISVTX	     0x0001000

#define VFS_IRWXU            0x0000700
#define VFS_IRUSR            0x0000400
#define VFS_IWUSR            0x0000200
#define VFS_IXUSR            0x0000100
#define VFS_IRWXG            0x0000070
#define VFS_IRGRP            0x0000040
#define VFS_IWGRP            0x0000020
#define VFS_IXGRP            0x0000010
#define VFS_IRWXO            0x0000007
#define VFS_IROTH            0x0000004
#define VFS_IWOTH            0x0000002
#define VFS_IXOTH            0x0000001

#define VFS_IREAD            VFS_IRUSR
#define VFS_IWRITE           VFS_IWUSR
#define VFS_IEXEC            VFS_IXUSR

#define VFS_SET(state,flag)    (state) |= (flag)
#define VFS_IS(state,flag)     (state) & (flag)
#define VFS_CLEAR(state,flag)  (state) &= ~(flag)


/* Lookup flags */
#define VFS_LOOKUP_FILE		0x1
#define VFS_LOOKUP_LAST		0x2
#define VFS_LOOKUP_OPEN		0x4
#define VFS_LOOKUP_RESTART	0x8
#define VFS_LOOKUP_RETRY	0x10
#define VFS_LOOKUP_PARENT	0x20
#define VFS_LOOKUP_HELD		0x40

/* Context flags*/
#define VFS_FS_LOCAL		0x1

#endif	/* _VFS_PARAMS_H_ */
