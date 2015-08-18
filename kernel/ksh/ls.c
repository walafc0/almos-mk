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
#include <kminiShell.h>
#include <errno.h>

static struct vfs_dirent_s dirent ;

 void show3(void);
 void show1(void);
 void show2(void);

error_t ls_func(void *param)
{
	char *path_name;
	error_t err;
	uint32_t argc;
	uint32_t i;
	uint_t count;
	struct vfs_file_s dir;
	struct ku_obj kuo;
	ms_args_t *args;
	//  char ch;

	args  = (ms_args_t *) param;
	err  = 0;

	if(args->argc == 1)
	{
		argc = 2;
		args->argv[1][0] = '.';
		args->argv[1][1] = 0;
	}
	else
		argc = args->argc;

	for(i=1; i < argc; i++)
	{
		if(argc == 1)
			path_name = ".";
		else
			path_name = args->argv[i];
	 
#if 0 
		ksh_print("calling vfs for path_name |%s|\n", path_name);
#endif

		KK_BUFF(kuo, path_name);
		if((err=vfs_opendir(ms_n_cwd, &kuo,0,&dir)))
			return err;
		
		ksh_print("Name\t\tType\t\tSize\n", path_name);
		ksh_print("----\t\t----\t\t----\n", path_name);
		count = 0;
		
		KK_OBJ(kuo, &dirent);
		while(!(err=vfs_readdir(&dir,&kuo)))
		{
			count++;
			show3();

#if 0      
			ch=getChar();
			
			if(ch == '\n')
			{
	ch=getChar();
	continue;
			} 
			if(ch == 'q') goto LS_END;
#endif
		}
		ksh_print("\n%d found\n",count);
		//  LS_END:
		vfs_closedir(&dir, &count);
		if(err != EEODIR) return err;
	}
	return 0;
}

 void show3(void)
{
	ksh_print("%s\t\t", dirent.d_name);

	switch(dirent.d_attr)
	{
	case VFS_DIR:
		ksh_print("<DIR>");
		break;
	case VFS_FIFO:
		ksh_print("<FIFO>");
		break;
	case VFS_DEV_BLK:
		ksh_print("<DEV-BLK>");
		break;
	case VFS_DEV_CHR:
		ksh_print("<DEV-CHR>");
		break;
	default:
		ksh_print("<RegFile>");
	}
	ksh_print("\n");
}

 void show1(void)
{
	ksh_print("=============================\n");
	ksh_print("Entry name %s\t", dirent.d_name);
	ksh_print("\tType: %s\n", (VFS_IS(dirent.d_attr, VFS_DIR)) ? "<DIR>" : 
	   (VFS_IS(dirent.d_attr, VFS_FIFO)) ? "FIFO file" : "Reg file");
	ksh_print("=============================\n");
}

 void show2(void)
{
	ksh_print("=============================\n");
	ksh_print("Entry name %s\t\t\t\t",
	    dirent.d_name);

	ksh_print("\t\tType: %s\n", (VFS_IS(dirent.d_attr, VFS_DIR)) ? "<DIR>" : 
	   (VFS_IS(dirent.d_attr, VFS_FIFO)) ? "FIFO file" : "Reg file");
	ksh_print("=============================\n");
}
