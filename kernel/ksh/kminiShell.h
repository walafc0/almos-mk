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

#ifndef __MINI_SHELL_H__
#define __MINI_SHELL_H__

#include <vfs.h>
#include <kdmsg.h>

#define MS_MAX_ARG_LEN    20
#define MS_MAX_LINE_LEN   64
#define BUFFER_SIZE       4096
#define MSG_STR_LEN       512

extern uint8_t buffer[];
extern kdmsg_channel_t ksh_tty;

typedef struct ms_argument_s
{
  uint_t argc;
  char argv[MS_MAX_ARG_LEN][MS_MAX_LINE_LEN];
}ms_args_t;

extern struct vfs_file_s *root ;
extern struct vfs_file_s *ms_n_cwd ;
extern char ms_pwd[128] ;

void print_error_msg(error_t err)  ;
void ksh_print(const char *fmt, ...);
void ksh_write(void *buff, size_t size);
char getChar() ;
void putChar(char ch) ;
void cmdLineAnalyser(ms_args_t *args);

typedef error_t (ksh_cmd_t)(void*);

/* Commands */
error_t clear_func(void* param);
error_t rm_func(void *parm) ;
error_t cd_func(void *parm) ;
error_t ls_func(void *param) ;
error_t mkdir_func(void *param) ;
error_t cp_func(void *arg) ;
error_t mkfifo_func(void *param) ;
error_t cat_func(void *param) ;
error_t cat2_func(void *param) ;
error_t eomtkp_func(void *param) ;
error_t ps_func(void *param);
error_t show_instrumentation(void* param) ;
error_t exec_func(void *param);
error_t pwd_func(void *param);
error_t ksh_set_tty_func(void *param);
error_t ppm_func(void *param);
error_t ksh_cpu_state_func(void *param);
error_t kill_func(void *param);

#endif
