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

#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include "ush.h"

int cd_func(void *param)
{
  ms_args_t *args;
  char * path_name;
  int err;

  args = (ms_args_t *) param;

  if(args->argc == 1)
    path_name = "/";
  else
    path_name = args->argv[1];

  if((err = chdir(path_name)) == -1)
    return -1;
  
  if(!(getcwd(&buffer[0], BUFFER_SIZE)))
  {
    fprintf(stderr, "ERROR geting CWD\n");
    return -1;
  }
  else
    buffer[PATH_MAX] = 0;
  
  setenv("PWD", &buffer[0], 1);
  return 0;
}
