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
#include <vfs.h>
#include <libk.h>
#include <kminiShell.h>
 
error_t pwd_func(void *param)
{
	error_t err;
	char buff[128];
	struct ku_obj ku_buff;

	KK_SZ_BUFF(ku_buff, buff, 128);
	err = vfs_get_path(ms_n_cwd, &ku_buff);

	ksh_print("%s\n", &buff[0]);

	return err;
}

