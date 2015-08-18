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

#include <fat32.h>
#include <fat32-private.h>
#include <kminiShell.h>


error_t show_instrumentation(void *param)
{
 #if CONFIG_VFAT_INSTRUMENT && CONFIG_BC_INSTRUMENT
  ksh_print("Total reads: %d\nTotal writes: %d\nDevBlok read %d\n\nHit: %d\nMiss %d\nDelayed Write %d\n",
	   rd_count, wr_count, blk_rd_count,hit_count, miss_count, dw_count);
#else
  ksh_print("Total reads: N/A\nTotal writes: N/A\nDevBlok read N/A\n\nHit: N/A\nMiss N/A\nDelayed Write N/A\n");
#endif

  return 0;
}
