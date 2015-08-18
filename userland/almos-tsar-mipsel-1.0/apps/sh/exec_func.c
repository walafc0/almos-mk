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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "ush.h"
 
int do_exec(char *path_name, char** argv)
{
	int pid;
	int err;

	pid = fork();
  
	if(pid == 0)
	{
#if 0
		while(1)
		{
			sleep(1);
			printf("from pid %d, sleeping ..\n", getpid());
		}
#endif
		err = execv(path_name, argv);

		if(err) perror("exec");

		exit(EXIT_FAILURE);
	}
  
	return 0;
}

int exec_func(void *param)
{
	char *path_name;
	int err;
	uint32_t argc,start,i;
	ms_args_t *args;
	unsigned long fork_flags;
	char *argv[MS_MAX_ARG_LEN];

	args = (ms_args_t *) param;
	argc = args->argc;
	fork_flags = PT_FORK_WILL_EXEC;

	if(argc < 2)
	{
		printf("exec: missing operand\n");
		errno = EINVAL;
		return EINVAL;
	}

	path_name = args->argv[1];
  
	if((path_name[0] == '-') && (argc < 4))
	{
		printf("exec: missing operand while using option %s\n", path_name);
		errno = EINVAL;
		return EINVAL;
	}
  
	if(path_name[0] == '-')
	{
		if((path_name[1] != 'P') && (path_name[1] != 'p'))
		{
			printf("exec: invalid option %c, usage: exec -P N path [arg1][arg2]..[argN]\n", path_name[1]);
			errno = EINVAL;
			return EINVAL;
		}

		path_name = args->argv[2];
		pthread_attr_setforkcpuid_np(atoi(path_name));
		path_name = args->argv[3];
		fork_flags |= PT_FORK_TARGET_CPU;
		start = 3;
	}
	else
	{
		start = 1;
	}

	argc -= start;
	
	for(i=0; i < argc; i++)
	{
		argv[i] = args->argv[start + i];
		//printf("%s: argv[%d]=[%s]\n", __FUNCTION__, i, argv[i]);
	}
	argv[argc] = NULL;
	
	pthread_attr_setforkinfo_np(fork_flags);
	
	err = 0;
	
	if((err = do_exec(path_name, argv)))
		printf("exec: error %d, faild to execute command %s\n", err, path_name);
	
	pthread_attr_setforkinfo_np(PT_FORK_DEFAULT);
	
	return err;
}

