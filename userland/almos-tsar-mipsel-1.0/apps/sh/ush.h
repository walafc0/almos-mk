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

#ifndef __MINI_SHELL_H__
#define __MINI_SHELL_H__

#include <stdint.h>
#include <stdio.h>

#define MS_MAX_ARG_LEN    20
#define MS_MAX_LINE_LEN   64
#define BUFFER_SIZE       4096
#define MSG_STR_LEN       512

extern uint8_t buffer[];

typedef struct ms_argument_s
{
  uint8_t argc;
  char argv[MS_MAX_ARG_LEN][MS_MAX_LINE_LEN];
}ms_args_t;

void print_error_msg(int err)  ;
char getChar();
void putChar(char ch) ;
ssize_t cmdLineAnalyser(FILE *stream, ms_args_t *args);

typedef int (ksh_cmd_t)(void*);

/* Commands */
int clear_func(void* param);
int rm_func(void *parm) ;
int cd_func(void *parm) ;
int ls_func(void *param) ;
int ps_func(void *param) ;
int mkdir_func(void *param) ;
int cp_func(void *arg) ;
int mkfifo_func(void *param) ;
int cat_func(void *param) ;
int exec_func(void *param);
int pwd_func(void *param);
int printenv_func(void *param);
int echo_func(void *param);
int export_func(void *param);
int kill_func(void* param);
#endif
