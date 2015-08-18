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
  
   UPMC / LIP6 / SOC (c) 2015
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#include <time.h>
#include <cpu-syscall.h>

#ifndef __NR_time
time_t time(time_t *t)
{
  struct timeb tb;
  //if (__unlikely(gettimeofday(&tv, NULL) < 0))
  if (ftime(&tb) < 0)
    tb.time = -1;
  if (t)
    *t = tb.time;
  return tb.time;
}
#endif
