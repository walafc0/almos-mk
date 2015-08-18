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

#ifndef _DIRENT_H_
#define _DIRENT_H_

#include <limits.h>
#include <sys/types.h>

#define _DIRENT_HAVE_D_TYPE
//#define _DIRENT_HAVE_D_SIZE

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#define  DT_REG      0x000
#define  DT_DIR      0x001
#define  DT_FIFO     0x002
#define  DT_CHR      0x004
#define  DT_BLK      0x008
#define  DT_SOCK     0x010
#define  DT_LNK      0x020
#define  DT_UNKNOWN  0x040

struct dirent {
  ino_t          d_ino;       /* inode number if any */
  unsigned char  d_type; 
  char           d_name[NAME_MAX];
};

typedef struct libc_dir_s
{
  int fd;
  struct dirent entry;
} DIR;

DIR *opendir(const char *nom);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dir);

#endif
