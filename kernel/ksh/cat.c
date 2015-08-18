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
#include <string.h>
#include <sys-vfs.h>
#include <kminiShell.h>
 
error_t cat_func(void *param)
{
#if 1
	struct ku_obj ku_buff;
	error_t err;
	ssize_t size;
	uint32_t argc;
	uint32_t i;
	struct vfs_file_s file;
	ms_args_t *args;
	uint_t count;

	args  = (ms_args_t*) param;
	argc = args->argc;
	err = 0;
	
	for(i=1; i< argc; i++)
	{
		
		KK_BUFF(ku_buff, args->argv[i]);
		if((err=vfs_open(ms_n_cwd, &ku_buff,VFS_O_RDONLY,0, &file)))
			return err;
		
		KK_SZ_BUFF(ku_buff, buffer, BUFFER_SIZE);
		while((size = vfs_read(&file, &ku_buff)) > 0)
		{ 
			ksh_write(buffer, size);
			//if((ch=getChar()) == 'q') break;
		}
		
		err = vfs_close(&file, &count);
	 
		if(err)
			printk(WARNING, "WARNING: %s: failed to close file at iteration %d, err %d\n", __FUNCTION__, i, err);

		
		if(size < 0) return size;
	}

	return 0;
#else
	return ENOSYS;
#endif
}

