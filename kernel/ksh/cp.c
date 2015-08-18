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

#include <string.h>
#include <stdint.h>
#include <vfs.h>
#include <kminiShell.h>

error_t cp_func(void *param)
{
	char *src_name;
	char *dest_name;
	error_t err;
	uint32_t argc;
	uint32_t i;
	ssize_t size;
	struct ku_obj kuo;
	struct vfs_file_s src_file;
	struct vfs_file_s dest_file;
	ms_args_t *args;
	uint_t count;			/* FIXME to be correctly used */

	args  = (ms_args_t *) param;
	argc = args->argc;
	dest_name = args->argv[argc - 1];
	err = 0;
	
	KK_BUFF(kuo, dest_name);
	if((err=vfs_open(ms_n_cwd, &kuo, VFS_O_WRONLY | VFS_O_CREATE,0, &dest_file)))
		return err;
	
	argc --;

	for(i=1; i< argc; i++)
	{
		src_name = args->argv[i];
		
		KK_BUFF(kuo, src_name);
		if((err=vfs_open(ms_n_cwd, &kuo,VFS_O_RDONLY,0, &src_file))){
			vfs_close(&dest_file, &count);
			ksh_print("error while opinig %s\n",src_name);
			return err;
		}
		size = 0;
		KK_SZ_BUFF(kuo, buffer, BUFFER_SIZE);
		while((size = vfs_read(&src_file, &kuo)) > 0)
		{
			//KK_SZ_BUFF(kuo, buffer, BUFFER_SIZE);
			if((size=vfs_write(&dest_file, &kuo)) < 0)
			goto CP_FUNC_ERROR;
		}
		
		if(size < 0) goto CP_FUNC_ERROR;
		vfs_close(&src_file, &count);
	}
	vfs_close(&dest_file,&count);
	return 0;

 CP_FUNC_ERROR:
	vfs_close(&src_file,&count);
	vfs_close(&dest_file,&count);
	
	return size;
}
