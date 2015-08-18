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


#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ush.h"


int cat_func(void *param)
{
  char **argv;
  char *path_name;
  int8_t err;
  ssize_t size;
  uint32_t argc;
  uint32_t i;
  int fd;
  ms_args_t *args;
  // char ch;

  args  = (ms_args_t*) param;
  argc = args->argc;
  err = 0;

  for(i=1; i< argc; i++)
  {
    path_name = args->argv[i];
    
    if((fd=open(path_name,O_RDONLY,0)) == -1)
      return -1;
    
    while((size = read(fd,&buffer[0],BUFFER_SIZE-1)) > 0)
    { 
      write(STDOUT_FILENO, &buffer[0], size);
      //memset(buffer,0, BUFFER_SIZE);
      //if((ch=getChar()) == 'q') break;
    }
    close(fd);
   
    if(size < 0) return size;
  }
  return 0;
}
