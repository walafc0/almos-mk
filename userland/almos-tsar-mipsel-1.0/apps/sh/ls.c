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

#define _BSD_SOURCE

#include <dirent.h>
#include <stdio.h>
#include <stdint.h>
#include "ush.h"

static struct dirent *entry;
static struct ms_argument_s *arg;

static void show(void);

int ls_func(void *param)
{
  char *path_name;
  int8_t err;
  uint32_t argc;
  uint32_t i;
  uint32_t count;
  DIR *dir;
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
    
    if((dir=opendir(path_name)) == NULL)
      return -1;
    
#ifdef _DIRENT_HAVE_D_SIZE
    printf("Name\t\tType\t\tSize\n", path_name);
    printf("----\t\t----\t\t----\n", path_name);
#else
    printf("Name\t\tType\n", path_name);
    printf("----\t\t----\n", path_name);
#endif

    count = 0;
    
    while((entry=readdir(dir)) != NULL)
    {
      count++;
      show();

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
    printf("\n%d found\n",count);
    //  LS_END:
    closedir(dir);
  }
  return 0;
}

/*
void show3(void)
{
  printf("%s\t\t%s%d\t\t%d\n", entry->d_name, (entry->d_type == DT_DIR) ? "<DIR>" : ((entry->d_type == DT_FIFO) ? "<FIFO>" : "<RegFile>"), entry->d_type, entry->d_size);
}
*/

static void show(void)
{
  printf("%s\t\t", entry->d_name);

  switch(entry->d_type)
  {
  case DT_DIR:
    printf("<DIR>");
    break;
  case DT_FIFO:
    printf("<FIFO>");
    break;
  case DT_BLK:
    printf("<DEV-BLK>");
    break;
  case DT_CHR:
    printf("<DEV-CHR>");
    break;
  default:
    printf("<RegFile>");
  }

#ifdef _DIRENT_HAVE_D_SIZE
  printf("\t\t%d\n", entry->d_size);
#else
  printf("\n");
#endif
}
