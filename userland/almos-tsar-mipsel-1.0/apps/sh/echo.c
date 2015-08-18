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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ush.h"

 
int echo_func(void *param)
{
  ms_args_t *args;
  char *str;
  uint32_t argc;
  uint32_t i;
  
  args  = (ms_args_t *) param;
  argc = args->argc;
  
  for(i=1; i< argc; i++)
  {
    str = args->argv[i];
    printf("%s ", str);
  }
  
  printf("\n");

  return 0;
}

