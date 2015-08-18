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

error_t cd_func(void *param)
{
	ms_args_t *args ;
	char * path_name;
	struct ku_obj ku_path;
	
	args  = (ms_args_t *) param;

	if(args->argc == 1)
		path_name = "/";
	else
		path_name = args->argv[1];

	
	KK_BUFF(ku_path, path_name);
	return vfs_chdir(&ku_path, ms_n_cwd);
}
