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
#include <system.h>
#include <device.h>

error_t ksh_set_tty_func(void *param)
{
  ms_args_t *args ;
  sint_t id = -1;

  args  = (ms_args_t *) param;

  if(args->argc == 2)
    id = atoi(args->argv[1]);
  
  if(id == -1)
  {
    ksh_print("%s: Unknown TTY id\n", __FUNCTION__);
    return EINVAL;
  }

  if((id >= TTY_DEV_NR) || (ttys_tbl[id] == NULL))
  {
    ksh_print("%s: Invalid TTY id: %d\n", __FUNCTION__, id);
    return EINVAL;
  }

  ksh_tty.id = id;
  return 0;
}
