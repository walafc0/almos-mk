/*
 * kern/sys-vfs.h - exported file management services
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

#ifndef _SYS_VFS_H_
#define _SYS_VFS_H_

#include <types.h>
#include <vfs.h>

struct vfs_usp_dirent_s;

/* Files related system call */
int sys_open (char *pathname, uint_t flags, uint_t mode);
int sys_stat (char *path, struct vfs_stat_s *buf, int fd);
int sys_creat (char *pathname, uint_t mode);
int sys_read (uint_t fd, void *buf, size_t count);
int sys_write (uint_t fd, void *buf, size_t count);
int sys_lseek (uint_t fd, off_t offset, int whence);
int sys_unlink (char *pathname);
int sys_close (uint_t fd);

/* Directories related system call */
int sys_opendir (char *pathname);
int sys_mkdir (char *pathname, uint_t mode);
int sys_rmdir (char *pathname);
int sys_chdir (char *pathname);
int sys_readdir (uint_t fd, struct vfs_usp_dirent_s *dirent);
int sys_closedir (uint_t fd);
int sys_getcwd (char *buff, size_t size);

/* Common */
int sys_chmod (char *pathname, uint_t mode);
int sys_fsync (uint_t fd);

/* Anonymous/Named pipes related system call */
int sys_pipe (uint_t *pipefd);
int sys_mkfifo (char *pathname, uint_t mode);

#endif
