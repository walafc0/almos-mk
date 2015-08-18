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

#include <stdio.h>
#include <vfs.h>
#include <buffer_cache.h>
#include <vfat-private.h>
#include <miniShell.h>


void show_instrumentation(void)
{
  printf("Total reads: %d\nTotal writes: %d\nDevBlok read %d\n\nHit: %d\nMiss %d\nDelayed Write %d\n",
	   rd_count, wr_count, blk_rd_count,hit_count, miss_count, dw_count);
}
