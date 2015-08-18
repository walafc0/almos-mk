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
#include <kminiShell.h>

#if 0
#define ERROR_NR 17

static const   char *msg1 = "NOT FOUND" ;
static const   char *msg2 = "NO MORE MEMROY RESSOURCES" ;
static const   char *msg3 = "NO MORE SPACE" ;
static const   char *msg4 = "I/O ERROR" ;
static const   char *msg5 = "UNKNOWN ERROR" ;
static const   char *msg6 = "INVAILD IN VALUE" ;
static const   char *msg7 = "BAD BLOCK FOUND" ;
static const   char *msg8 = "ALREADY EXIST !" ;
static const   char *msg9 = "IT'S DIRECTORY !" ;
static const   char *msg10 = "BAD FILE DESCRIPTOR" ;
static const   char *msg11 = "IT'S A PIPE OR FIFO" ;
static const   char *msg12 = "SIZE OVERFLOW" ;
static const   char *msg13 = "IS NOT DIRECTORY" ;
static const   char *msg14 = "END OF DIRECTORY" ;
static const   char *msg15 = "NOT IMPLEMENTED BY THAT SUB-SYSTEM" ;
static const   char *msg16 = "NO MORE READERS ON PIPE OR FIFO" ;
static const   char *msg17 = "DIRECTORY IS NOT EMPTY" ;
 
void print_error_msg(error_t err)
{
  const char *msg_tab[18] = {NULL,msg1,msg2,msg3,msg4,msg5,msg6,msg7,msg8,msg9, \
			     msg10,msg11,msg12,msg13,msg14,msg15,msg16,msg17};

  err = (err < 0) ? -err : err;
  
  if((err < 1) || (err > ERROR_NR))
    ksh_print("Invaild error code !!! %d\n", err);
  else
    ksh_print("Error: %s\n",msg_tab[err]);
}

#else
void print_error_msg(error_t err)
{
  ksh_print("Command has failed\n");
}
#endif
