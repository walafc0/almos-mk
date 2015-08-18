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
#include "ush.h"
 
int printenv_func(void* param)
{
  char *env;
  ms_args_t *args;

  args  = (ms_args_t *) param;
  env = (char*)getenv(args->argv[1]);

  printf("%s\n", (env == NULL) ? "" : env);
  return 0;
}

#if 0
int export_func(void *param)
{
  char *env;
  ms_args_t *args;

  args  = (ms_args_t *) param;
  
  env = (char*)getenv(args->argv[1]);

  printf("%s\n", (env == NULL) ? "" : env);
  
}
#endif
