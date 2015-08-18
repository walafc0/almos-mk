/*
   This file is part of MutekP.
  
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
#include <stdint.h>
#include <errno.h>
#include "ush.h"

int mkfifo_func(void *param)
{
  return ENOSYS;
#if 0
  char *path_name;
  int8_t err;
  uint32_t argc;
  uint32_t i;

  arg  = (struct ms_argument_s *) param;
  argc = arg->argc;
  argv = &arg->argv[0];
  err  = 0;
  
  for(i=0; i< argc; i++)
  {
    path_name = argv[i];
   
    if((err=mkfifo(path_name,0)))
      return err;
  }
  return 0;
#endif
}
