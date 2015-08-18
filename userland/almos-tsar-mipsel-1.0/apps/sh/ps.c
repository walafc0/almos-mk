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
#include <pthread.h>
#include "ush.h"


int ps_func(void *param)
{
  int err; 
  ms_args_t *args;
  pid_t pid;
  uint_t tid;
  int cmd;

  args = param;
  pid = 0;
  tid = 0;
  
  if((args->argc != 1) && (args->argc != 4))
  {
    fprintf(stderr, "Syntax Error: ps [pid tid [ON|OFF]]\n");
    errno = EINVAL;
    return EINVAL;
  }

  if(args->argc == 1)
    cmd = PT_SHOW_STATE;
  else
  {
    pid = atoi(args->argv[1]);
    tid = atoi(args->argv[2]);

    if(!(strcmp(args->argv[3],"ON")))
      cmd = PT_TRACE_ON;
    else
      cmd = PT_TRACE_OFF;
  }
  
  err = pthread_profiling_np(cmd, pid, (pthread_t)tid);
  errno = err;
  return err;
}
