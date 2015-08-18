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

#include <stdint.h>
#include <thread.h>
#include <task.h>
#include <kminiShell.h>

char *envrion[] = { "ALMOS_VERSION=v1", NULL};
 
error_t exec_func(void *param)
{
  char *path_name;
  //error_t err;
  uint32_t argc,i;
  ms_args_t *args;
  char *argv[MS_MAX_ARG_LEN + 1];

  args = (ms_args_t *) param;
  argc = args->argc;

  if(argc < 2)
  {
    ksh_print("exec: missing operand\n");
    return EINVAL;
  }

  path_name = args->argv[1];

  for(i=1; i < argc; i++)
    argv[i-1] = args->argv[i];

  argv[argc - 1] = NULL;
  
#if 0
  for(i=0; i < argc; i++)
    ksh_print("%s: argv[%d]=%x, argv[%d]=%s\n", __FUNCTION__,i, argv[i], i, (argv[i] == NULL) ? "NULL" : argv[i]);
#endif

#if 0
  if((err = do_exec(current_task, path_name, argv, &envrion[0])))
  {
    ksh_print("exec: error %d, faild to execute command %s\n", err, path_name);
    return err;
  }
#endif
  return 0;
}

