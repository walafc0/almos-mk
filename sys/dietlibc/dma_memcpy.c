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
   Copyright Ghassan Almaless <ghassan.almaless@lip6.fr>
*/

#include <errno.h>
#include <sys/syscall.h>
#include <cpu-syscall.h>
#include <string.h>

void *dma_memcpy (void *src, void *dst, unsigned int size)
{
  void *value = cpu_syscall(src,dst,(void*)size,NULL,SYS_DMA_MEMCPY);
  return (value == NULL) ? dst : NULL;
}

