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

#include <ctype.h>
#include <libk.h>
#include <string.h>
#include <thread.h>
#include <cluster.h>
#include <sys-vfs.h>
#include <driver.h>
#include <device.h>
#include <kminiShell.h>
#include <stdarg.h>

kdmsg_channel_t ksh_tty = {{.id = 0}};
 
char getChar()
{
  struct device_s *tty;
  dev_request_t rq;
  char ch;
  size_t size;
  
  ch = 0;
  size = 0;
  tty = ttys_tbl[ksh_tty.id];

  while(size != 1 && size != -1)
  {
    rq.dst = (uint32_t*) &ch;
    rq.count = 1;
    rq.flags = 0;
    size = tty->op.dev.read(tty, &rq);
  }
  return ch;
}


ssize_t ksh_getLine(char *buff, int count)
{
  struct device_s *tty;
  dev_request_t rq;
  ssize_t size;
 
  tty = ttys_tbl[ksh_tty.id];

  rq.dst = (uint32_t*) buff;
  rq.count = count;
  rq.flags = 0;
  size = tty->op.dev.read(tty, &rq);
  return size;
}

void ksh_write(void *buff, size_t size)
{
  struct device_s *tty;
  dev_request_t rq;
  
  tty = ttys_tbl[ksh_tty.id];
  rq.src = buff;
  rq.count = size;
  rq.flags = 0;
  (void)tty->op.dev.write(tty, &rq);
}

static char ksh_msg_buff[MSG_STR_LEN];

void ksh_print(const char *fmt, ...)
{
  va_list ap;
  int count;

  va_start (ap, fmt);
  count = iprintk(&ksh_msg_buff[0], current_cid, 1, (char *) fmt, ap);
  assert(count < (MSG_STR_LEN-1));
  va_end (ap);
  count = (count < (MSG_STR_LEN -1)) ? count : MSG_STR_LEN - 1;
  ksh_msg_buff[count + 1] = 0;
  ksh_write(ksh_msg_buff, count);
}

void getString(char *buff,size_t size)
{
  char ch = 0;
  char *str = buff;
  char *val = NULL;
  char line[128];
  char *line_ptr = &line[0];
  ssize_t count,i;
  

  while(size > 0)
  {
    count = ksh_getLine(line_ptr,128);
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
  
 GETSTRING_END:
  val = buff;
  *str = 0;
}

void cmdLineAnalyser(ms_args_t *args)
{
  char buff[128];
  char *str = &buff[0];
  char *arg = NULL;
  uint_t i;

  getString(buff,128);
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
}
