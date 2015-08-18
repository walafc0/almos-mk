/*
   This file is part of MutekP.
  
   MutekP is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   MutekP is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with MutekP; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
   UPMC / LIP6 / SOC (c) 2008
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#ifndef _STAT_H_
#define _STAT_H_

#include <sys/types.h>

typedef uint32_t mode_t;

#define S_IFMT	        0x0170000
#define S_IFSOCK	0x0140000
#define S_IFLNK		0x0120000
#define S_IFREG		0x0100000
#define S_IFBLK		0x0060000
#define S_IFDIR		0x0040000
#define S_IFCHR		0x0020000
#define S_IFIFO		0x0010000

#define S_ISUID		0x0004000
#define S_ISGID		0x0002000
#define S_ISVTX		0x0001000

#define S_IRWXU         0x0000700
#define S_IRUSR         0x0000400
#define S_IWUSR         0x0000200
#define S_IXUSR         0x0000100

#define S_IRWXG         0x0000070
#define S_IRGRP         0x0000040
#define S_IWGRP         0x0000020
#define S_IXGRP         0x0000010

#define S_IRWXO         0x0000007
#define S_IROTH         0x0000004
#define S_IWOTH         0x0000002
#define S_IXOTH         0x0000001

#define S_IREAD         S_IRUSR
#define S_IWRITE        S_IWUSR
#define S_IEXEC         S_IXUSR

#define S_ISLNK(n)	(((n) & S_IFMT) == S_IFLNK)
#define S_ISREG(n)	(((n) & S_IFMT) == S_IFREG)
#define S_ISDIR(n)	(((n) & S_IFMT) == S_IFDIR)
#define S_ISCHR(n)	(((n) & S_IFMT) == S_IFCHR)
#define S_ISBLK(n)	(((n) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(n)	(((n) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(n)	(((n) & S_IFMT) == S_IFSOCK)


struct stat 
{
  dev_t     st_dev;     /* ID of device containing file */
  ino_t     st_ino;     /* inode number */
  mode_t    st_mode;    /* protection */
  nlink_t   st_nlink;   /* number of hard links */
  uid_t     st_uid;     /* user ID of owner */
  gid_t     st_gid;     /* group ID of owner */
  dev_t     st_rdev;    /* device ID (if special file) */
  uint64_t  st_size;    /* total size, in bytes */
  blksize_t st_blksize; /* blocksize for file system I/O */
  blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
  time_t    st_atime;   /* time of last access */
  time_t    st_mtime;   /* time of last modification */
  time_t    st_ctime;   /* time of last status change */
};

int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);
int lstat(const char *path, struct stat *buf);
int chmod(const char *path, mode_t mode);

int mkdir(const char *pathname, mode_t mode);
int mkfifo(const char *pathname, mode_t mode);

#endif
