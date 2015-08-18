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
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include "ush.h"

int cp_func(void *param)
{
  const char *src_name;
  const char *dest_name;
  int err;
  uint32_t argc;
  uint32_t i;
  ssize_t size;
  int src_fd;
  int dst_fd;
  ms_args_t *args;

  args  = (ms_args_t *) param;
  argc = args->argc;

  if(argc < 3)
  {
    fprintf(stderr, "Syntax Error: %s src_name dst_name\n", args->argv[0]);
    errno = EINVAL;
    return -1;
  }

  dest_name = args->argv[argc - 1];
  err = 0;
  
  if((dst_fd = open(dest_name,O_WRONLY | O_CREAT,0)) == -1)
  {
    perror("create");
    printf("error while creating %s\n", dest_name);
    return -2;
  }

  argc --;
  memset(buffer,0,BUFFER_SIZE);

  for(i=1; i< argc; i++)
  {
    src_name = args->argv[i];
    
    if((src_fd = open(src_name,O_RDONLY,0)) == -1){
      err = errno;
      perror("open");
      printf("error while opinig %s\n",src_name);
      close(dst_fd);
      unlink(dest_name);
      errno = err;
      return -1;
    }
    size = 0;
    while((size = read(src_fd,buffer,BUFFER_SIZE)) > 0)
    {
      if((size=write(dst_fd,buffer,size)) < 0)
	goto CP_FUNC_ERROR;
    }
    
    if(size < 0) goto CP_FUNC_ERROR;
    close(src_fd);
  }

  close(dst_fd);
  return 0;

 CP_FUNC_ERROR:
  close(src_fd);
  close(dst_fd);
  return size;
}
