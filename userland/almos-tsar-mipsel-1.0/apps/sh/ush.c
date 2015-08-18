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


#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "ush.h"

#define NR_THREADS  1

uint8_t buffer[BUFFER_SIZE];
static ms_args_t args;

void print_args(ms_args_t *args)
{
  int i;

  for(i=0; i < MS_MAX_ARG_LEN; i++)
  {
    printf("idx %d:",i);
    if(args->argv[i] != NULL)
      printf("|%s|\n", args->argv[i]);
    else
      printf("NULL\n");
  }

  printf("argc %d\n", args->argc);
}

typedef struct ksh_builtin_s
{
  char *name;
  char *info;
  ksh_cmd_t *cmd;
}ksh_builtin_t;

static int help_func(void *param);

static ksh_builtin_t builtin_cmds[] = 
  {
    {"cd", "Change Directory", cd_func},
    {"cp", "Copy File", cp_func},
    {"cat", "Show File Contentes", cat_func},
    {"echo", "Echo passed arguments", echo_func},
    {"exec", "Execute New Process", exec_func},
    {"export", "Set the export attribute for variables", export_func},
    {"kill", "Send the specified signal to the specified process", kill_func},
    {"ls", "List Files In Directory", ls_func},
    {"rm", "Remove File", rm_func},
    {"ps", "Print Processes States", ps_func},
    {"printenv", "Print all or part of environment", printenv_func},
    {"clear", "Clear Some Lines", clear_func},
    {"mkdir", "Make New Directory", mkdir_func},
    {"mkfifo", "Make New Named FIFO File", mkfifo_func},
    {"help", "Show Available commands", help_func},
    {NULL,NULL,NULL}
  };

static int help_func(void *param)
{
  int i;

  printf("Available Commands:\n");

  for(i=0; builtin_cmds[i].name != NULL; i++)
    printf("%s: %s\n", builtin_cmds[i].name, builtin_cmds[i].info);

  printf("exit: Exit and synchronise panding disk wirtes if any\n");
  return 0;
}

static int ush_exec_cmd(char *cmd, ms_args_t *args)
{
  int i;
  int err;

  for(i=0; builtin_cmds[i].name != NULL; i++)
  {
    if(!strcmp(cmd, builtin_cmds[i].name))
    {
      if((err=builtin_cmds[i].cmd(args)))
	print_error_msg(err);
      return 0;
    }
  }
  return -1;
}

void* ush(void *arg)
{
  FILE *shrc;
  ssize_t count;
  ssize_t err;
  register char *cmd;
  
  if((cmd = getcwd(&buffer[0], sizeof(buffer))) == NULL)
  {
    fprintf(stderr, "ERROR geting CWD\n");
    pthread_exit((void*)EXIT_FAILURE);
  }
  else
    buffer[PATH_MAX] = 0;
  
  setenv("PWD", &buffer[0], 1);

  shrc = fopen("/etc/shrc", "r");

  if(shrc != NULL)
  {   
          while(1)
          {
                  count = cmdLineAnalyser(shrc, &args);
                  cmd = args.argv[0];

                  if(count < 0) break;

#if 0
                  print_args(&args);
#endif

                  if(cmd[0] == '#')
                          continue;

                  if((err=ush_exec_cmd(cmd, &args)))
                  {
                          if(!strcmp(cmd, "exit"))
                                  return NULL;
                          else
                                  printf("Unknown command %s\n", cmd);
                  }
          }

          fclose(shrc);
  }

  while(1)
  {
          printf("[%s]>",getenv("PWD"));
          fflush(stdout);

          cmdLineAnalyser(stdin, &args);
          cmd = args.argv[0];

#if 0
          print_args(&args);
#endif

          if((err=ush_exec_cmd(cmd, &args)))
          {
                  if(!strcmp(cmd, "exit"))
                          break;
                  else
                          printf("Unknown command %s\n", cmd);
          }
  }

  printf("Finished\n");
  return NULL;
}


#ifndef _ALMOS_
#define pthread_profiling_np()
#define pthread_attr_setcpuid_np(x,y,z)
#endif

int main()
{
  pthread_attr_t attr;
  pthread_t th;
  int err;
  int status = 0;

#if 0
  if (pthread_attr_init (&attr) != 0)
    fprintf (stderr, "Error initialization of thread's attribute \n ");
  
#ifndef _XOPEN_SOURCE    
  pthread_attr_setcpuid_np(&attr, 0, NULL);
#endif

  if((err = pthread_create (&th, &attr, ush, NULL)) != 0)
  {
    fprintf (stderr, "Error creating thread, Error code :%d\n",err);
    pthread_exit ((void *) EXIT_FAILURE);
  }

  if((err = pthread_join(th, (void**)&status)))
    fprintf(stderr, "Error to join thread, %d\n",err);
  
  fprintf(stderr, "main finished\n");
  return status;
#else
  return (int) ush(NULL);
#endif
}
