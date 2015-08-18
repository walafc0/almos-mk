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
  
   UPMC / LIP6 / SOC (c) 2007
*/

/*
 * $Log: sprintf.c,v $
 * Revision 1.1.1.1  2002/02/28 12:58:56  disydent
   Creation of Disydent CVS Tree
 * Revision 1.2  2001/11/22 15:07:38  fred
   Adding the iprintf function that does the formatting, and removes all
   the old stuff hanging around.
 * Revision 1.1.1.1  2001/11/19 16:55:47  pwet
   Changing the CVS tree structure of disydent
 * Revision 1.2  2001/11/14 12:12:26  fred
   Modification for the call to printf and friends.
   This should be reworked over sometime.
 * Revision 1.1.1.1  2001/07/24 13:31:54  pwet
   cvs tree of part of disydent
 * Revision 1.1.1.1  2001/07/20 07:35:57  pwet
   Embedded software cvs tree
 * Revision 1.1  2000/07/24 08:16:05  denis
   First archive of libc
 * Revision 1.1  1998/09/16 16:11:22  pwet
   passage a cvs
 */


#include <stdarg.h>
#include <thread.h>
#include <cluster.h>
#include <libk.h>

int sprintk (char *s, char *fmt, ...)
{
   va_list ap;
   int count;

   va_start (ap, fmt);

   count = iprintk (s, current_cid, 1, fmt, ap);
   *(s + count) = 0;

   va_end (ap);

   return count;
}

