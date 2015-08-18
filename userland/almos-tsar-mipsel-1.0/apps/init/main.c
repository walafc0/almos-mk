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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>

#define SIG1              SIGTERM
#define SIG2              SIGHUP
#define SHELL_BIN_PATH    "/bin/sh"
#define SHELL_HOME_PATH   "/home/root"

typedef void (*sighandler_t)(int);

static void sig_handler(int sig)
{
	printf("Signal %d has been received\n", sig);
	exit(0);
}

int main (int argc, char *argv[], char *envp[])
{
	sighandler_t old;
	pid_t pid;
	int i;
	int err;
	char *version;

	old = signal(SIG1, &sig_handler);

	if(old == SIG_ERR)
	{
		perror("Signal Handler");
		return EXIT_FAILURE;
	}

	old = signal(SIG2,sig_handler);

	if(old == SIG_ERR)
	{
		perror("Signal Handler");
		return EXIT_FAILURE;
	}

	version = getenv("ALMOS_VERSION");

	printf("init: System Revision Info [%s]\n", (version == NULL) ? "Unknown" : version);
	printf("init: Listening to signals %d, %d\n", SIG1, SIG2);
 
	pid = fork();
    
	if(pid == 0)
	{
		err = chdir(SHELL_HOME_PATH);
    
		if(err)
		{
			perror("chdir");
			exit(EXIT_FAILURE);
		}
    
		err = execl(SHELL_BIN_PATH, SHELL_BIN_PATH, NULL);
    
		if(err)
			perror("execl");
    
		exit(EXIT_FAILURE);
	}
	else
		printf("init: Connexion-Shell has been created [pid %d]\n", (int)pid);

	while(1) sleep(10000);
  
	return EXIT_SUCCESS;
}
