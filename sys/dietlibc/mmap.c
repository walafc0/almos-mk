/*
   This file is part of AlmOS.
  
   AlmOS is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   AlmOS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with AlmOS; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
   UPMC / LIP6 / SOC (c) 2008
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#include <errno.h>
#include <sys/syscall.h>
#include <cpu-syscall.h>
#include <unistd.h>

typedef struct mmap_attr_s
{
  void *addr;
  uint_t length; 
  uint_t prot;
  uint_t flags;
  uint_t fd;
  off_t offset;
}mmap_attr_t;

void *mmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  mmap_attr_t mattr;
  
  mattr.addr = addr;
  mattr.length = len;
  mattr.prot = prot;
  mattr.flags = flags;
  mattr.fd = fd;
  mattr.offset = offset;
  
  return cpu_syscall(&mattr, NULL, NULL, NULL,SYS_MMAP);
}
