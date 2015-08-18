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

#include <kminiShell.h>
#include <list.h>
#include <thread.h>
#include <task.h>
#include <signal.h>

error_t kill_func(void *param)
{
  ms_args_t *args;
  uint32_t argc;
  uint_t sig;
  pid_t pid;
  cid_t location;
  error_t err;

  args  = (ms_args_t *) param;
  argc = args->argc;
  err = 0;

  if(argc != 3)
  {
    ksh_print("Missing signal number or process pid\n");
    return EINVAL;
  }

  sig = atoi(args->argv[1]);
  pid = atoi(args->argv[2]);

  if((sig == 0)  || (sig >= SIG_NR))
  {
    ksh_print("Unknown signal number: %d\n", sig);
    return EINVAL;
  }

  location = task_whereis(pid);

  if((err = signal_rise(pid, location, sig)))
    ksh_print("Failed to rise signal %d on task %d\n", sig, pid);

  return err;
}
