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

#include <system.h>
#include <stdint.h>
#include <string.h>
#include <vfs.h>
#include <thread.h>
#include <kminiShell.h>
#include <task.h>

#define NR_THREADS  1

uint8_t buffer[BUFFER_SIZE];

struct vfs_file_s *root ;
struct vfs_file_s *ms_n_cwd ;

ms_args_t args;

void print_args(ms_args_t *args)
{
  int i;

  for(i=0; i < MS_MAX_ARG_LEN; i++)
  {
    ksh_print("idx %d:",i);
    if(args->argv[i] != NULL)
      ksh_print("|%s|\n", args->argv[i]);
    else
      ksh_print("NULL\n");
  }

  ksh_print("argc %d\n", args->argc);
}


typedef struct ksh_builtin_s
{
  char *name;
  char *info;
  ksh_cmd_t *cmd;
}ksh_builtin_t;

static error_t help_func(void *param);


static ksh_builtin_t builtin_cmds[] = 
  {
    {"setty", "Set I/O Terminal Number", ksh_set_tty_func},
    {"cd", "Change Directory", cd_func},
    {"ls", "List Files In Directory", ls_func},
    {"cp", "Copy File", cp_func},
    {"cat", "Show File Contentes", cat_func},
    {"ps", "Show Process (Tasks) State", ps_func},
    {"exec", "Execute New Process", exec_func},
    {"ppm", "Print Phyiscal Pages Manager", ppm_func},
    {"cs", "CPUs Stats", ksh_cpu_state_func},
    {"stat", "Show File System Instrumentation", show_instrumentation},
    {"pwd", "Print Work Directory", pwd_func},
    {"rm", "Remove File", rm_func},
    {"kill", "Send the specified signal to the specified process", kill_func},
    {"eomtkp", "Display SGI Image On Frame-Buffer", eomtkp_func},
    {"clear", "Clear Some Lines", clear_func},
    {"mkdir", "Make New Directory", mkdir_func},
    {"mkfifo", "Make New Named FIFO File", mkfifo_func},
    {"help", "Show Available commands", help_func},
    {NULL,NULL,NULL}
  };

static error_t help_func(void *param)
{
  uint_t i;

  ksh_print("Available Commands:\n");

  for(i=0; builtin_cmds[i].name != NULL; i++)
    ksh_print("%s: %s\n", builtin_cmds[i].name, builtin_cmds[i].info);

  ksh_print("exit: Exit and synchronise panding disk wirtes if any\n");
  return 0;
}


static error_t ksh_exec_cmd(char *cmd, ms_args_t *args)
{
  uint_t i;
  error_t err;

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


void* kMiniShelld(void *arg)
{
  ssize_t err;
  struct task_s *task = current_task;
  register char *cmd;

  memset(&args,0,sizeof(args));
  root = &task->vfs_root;
  ms_n_cwd = &task->vfs_cwd;  

#if 0
  ksh_print("[%s]>",ms_n_cwd->n_name);
  args.argc = 7;
  strcpy(args.argv[0],"exec");
  strcpy(args.argv[1],"/BIN/SHOWIMG.X");
  strcpy(args.argv[2],"-p4");
  strcpy(args.argv[3],"-i");
  strcpy(args.argv[4],"/TMP/LENA.SGI");
  strcpy(args.argv[5],"-o");
  strcpy(args.argv[6],"/DEV/FB0");
  
  ksh_exec_cmd("exec", &args);
#endif

#if 0
  ksh_print("[%s]>",ms_n_cwd->n_name);
  args.argc = 7;
  strcpy(args.argv[0],"exec");
  strcpy(args.argv[1],"/BIN/HELLO.X");
  strcpy(args.argv[2],"-p4");
  strcpy(args.argv[3],"-i");
  strcpy(args.argv[4],"/TMP/LENA.SGI");
  strcpy(args.argv[5],"-o");
  strcpy(args.argv[6],"/DEV/FB0");
  
  ksh_exec_cmd("exec", &args);

  ksh_print("[%s]>",ms_n_cwd->n_name);
  args.argc = 7;
  strcpy(args.argv[0],"exec");
  strcpy(args.argv[1],"/BIN/SHOWIMG.X");
  strcpy(args.argv[2],"-p16");
  strcpy(args.argv[3],"-i");
  strcpy(args.argv[4],"/TMP/IMG.RGB");
  strcpy(args.argv[5],"-o");
  strcpy(args.argv[6],"/DEV/FB0");
  
  ksh_exec_cmd("exec", &args);

#endif

#if 0
  task->vfs_cwd = ms_n_cwd;
  ksh_print("[%s]>",ms_n_cwd->n_name);
  args.argc = 2;
  strcpy(args.argv[0],"cd");
  strcpy(args.argv[1],"/tmp");
  ksh_exec_cmd("cd", &args);

  task->vfs_cwd = ms_n_cwd;
  ksh_print("[%s]>",ms_n_cwd->n_name);
  args.argc = 3;
  strcpy(args.argv[0],"cp");
  strcpy(args.argv[1],"r.txt");
  strcpy(args.argv[2],"f.txt");
  ksh_exec_cmd("cp", &args);
#endif


  while(1)
  {
    task->vfs_cwd = *ms_n_cwd;
    //ksh_print("[%s]>",ms_n_cwd->n_name);FIXME
    ksh_print("[%s]>", __FUNCTION__);
    cmdLineAnalyser(&args);
    cmd = args.argv[0];

#if 0
    print_args(&args);
#endif

    if((err=ksh_exec_cmd(cmd, &args)))
    {
      if(!strcmp(cmd, "exit"))
	break;
      else
	ksh_print("Unknown command %s\n", cmd);
    }
  }

  //bc_dump(&bc);
  //bc_sync(&bc);
  ksh_print("Finished\n");
  return NULL;
}
