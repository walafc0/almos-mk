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

#include <cpu.h>
#include <string.h>
#include <stdint.h>
#include <kminiShell.h>
#include <vfs.h>
#include <sgiImg.h>
#include <sys-vfs.h>

#ifdef BUFFER_SIZE
#undef BUFFER_SIZE //512 //2048
#define BUFFER_SIZE 512
#endif

error_t eomtkp_func(void *param)
{
  char *src_name;
  error_t err;
  uint16_t x_size;
  uint16_t y_size;
  ssize_t size;
  int src_fd;
  int dst_fd;
  int tmp_fd;
  clock_t tm_start;		/* start of command */
  clock_t tm_header;		/* image's header paressing */
  clock_t tm_end;		/* end of command */
  clock_t tm_tmp;
  ms_args_t *args;
  struct sgi_info_s *sgi_info;

  sys_clock(&tm_start);

  args  = (ms_args_t *) param;
  src_name = args->argv[1];
  err = 0;
  memset(buffer,0,BUFFER_SIZE);

  if((dst_fd = sys_open("/DEV/SCREEN",VFS_O_WRONLY,0)) == -1)
    return -1;
      
  if((src_fd = sys_open(src_name,VFS_O_RDONLY,0)) == -1){
    sys_close(dst_fd);
    ksh_print("error while opinig %s\n",src_name);
    return -2;
  }
  
  if((size = sys_read(src_fd, buffer,sizeof(*sgi_info))) < 0)
  {
    sys_close(dst_fd);
    sys_close(src_fd);
    return -3;
  }

  sgi_info = (struct sgi_info_s*) buffer;
  x_size = SGI_TO_LE2(sgi_info->x_size);
  y_size = SGI_TO_LE2(sgi_info->y_size);

  if((tmp_fd = sys_open("/DEV/SCREEN",VFS_O_WRONLY,0)) == -1)
    return -1;
  
  ksh_print("Image XSIZE %d, YSIZE %d\n", x_size, y_size);
  ksh_print("Image Name: %s\n", sgi_info->img_name);
  
  if((size = sys_lseek(src_fd, sizeof(struct sgi_info_s), VFS_SEEK_SET)) == -1)
    return -4;

  sys_clock(&tm_tmp);
  tm_header = tm_tmp - tm_start;

  size = 0;
  while((size = sys_read(src_fd,buffer,BUFFER_SIZE)) > 0)
  {
    if((size=sys_write(dst_fd,buffer,size)) < 0)
      break;
  }
  
  sys_close(src_fd);
  sys_close(dst_fd);
  
  sys_clock(&tm_end);

  ksh_print("  Command statistics:\n\tStart time: %u\n\tEnd time: %u\n\tElapsed time: %u\n\tHeader time: %u\n",
	 tm_start,
	 tm_end,
	 tm_end - tm_start,
	 tm_header);
  return size;
}
