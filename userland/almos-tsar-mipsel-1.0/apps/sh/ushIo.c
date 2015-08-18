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
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "ush.h"


ssize_t getString(FILE *stream, char *buff,size_t size)
{
  char ch = 0;
  char *str = buff;
  char *val = NULL;
  char line[128];
  char *line_ptr = &line[0];
  ssize_t count,bytes, i;
  void *ret;

  memset(&line[0], 0, 128);

  while(size > 0)
  {
    ret = fgets(line_ptr, 128, stream);
    
    if(ret == NULL) return -1;

    count = strlen(line_ptr);
    
    if(size == 128) bytes = count;
    
    i=0;

    while((count > 0) && (size > 0))
    {
      ch=line[i++];
      
      if(ch == '\r')
	continue;

      if(ch == '\n')
	goto GETSTRING_END;

      if(!isprint(ch))
      {
	if((ch == 0x7F) || (ch == 0x08))
	  str--;
	count ++;
	size ++;
	continue;
      }

      *str ++ = ch;
      ch = 0;
      count --;
      size --;
    }
  }

  return -2;
  
 GETSTRING_END:
  val = buff;
  *str = 0;
  return bytes;
}

#ifdef _ALMOS_
#define ms_toupper(x) toupper((x))
#else
#define ms_toupper(x) (x)
#endif

ssize_t cmdLineAnalyser(FILE *stream, ms_args_t *args)
{
  char buff[128];
  char *str = &buff[0];
  char *arg = NULL;
  uint_t i;
  ssize_t count;

  count = getString(stream, buff,128);

  if(count < 0) return count;

  i=0;

  while((*str != '\0') && (i < MS_MAX_ARG_LEN))
  {
    while((*str) && (isspace(*str))) str++;
    
    if(!(*str)) break;
   
    arg = args->argv[i];
 
    while((*str) && (!isspace(*str))) 
      *arg++ = *str ++;

    i++; *arg = 0;
  }

  args->argv[i][0] = 0;
  args->argc = i;
  return count;
}
