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

#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <cpu-syscall.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>

struct vfs_dirent_s
{
  uint32_t d_ino;
  uint32_t d_type;
  uint8_t  d_name[NAME_MAX];
};

struct dirent *readdir(DIR *dir)
{
  int retval;
  struct vfs_dirent_s dirent;
  
  if(dir == NULL)
    return NULL;

  retval = (int) cpu_syscall((void*)(dir->fd),(void*)&dirent,NULL,NULL,SYS_READDIR);
  
  if(retval != 0)
    return NULL;
  
  dir->entry.d_ino  = dirent.d_ino;
  dir->entry.d_type = dirent.d_type;
  //dir->entry.d_size = dirent.d_size;
  strcpy(dir->entry.d_name, (char*)dirent.d_name); 
  
  return &dir->entry;
}
