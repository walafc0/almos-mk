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
   revision 1.2.1 2007/11/11 18:54:23 ghassan
   rename iprintf to __iprintk, it's a copy of iprintf called on system level.
   
   stdio of the ghost of libc
   Developped by Denis Hommais and Frédéric Pétrot
   $Log: iprintf.c,v $
   Revision 1.2  2003/07/01 13:40:46  fred
   Supports the %u and doesnt core anymore on an unrecognized char
   following a %
  
   Revision 1.1.1.1  2002/02/28 12:58:56  disydent
   Creation of Disydent CVS Tree
  
   Revision 1.1  2001/11/22 15:07:36  fred
   Adding the iprintf function that does the formatting, and removes all
   the old stuff hanging around.
  
   $Id: iprintf.c,v 1.2 2003/07/01 13:40:46 fred Exp $
   Made up this file to centralize the printf behavior
*/

#include <stdarg.h>
#include <dmsg.h>
#include "mmu.h"
/* Handling of the printf internals
 * Addr is either the buffer or the tty addrs
 * inc is 0 for a tty and 1 for a buffer
 * Other arguments are self explanatory
 */

static int iprintk (uint64_t addr, int inc, const char *fmt, va_list ap);
int boot_dmsg (uint64_t tty, const char *fmt, ...)
{
   va_list ap;
   int count;

   va_start (ap, fmt);
   
   count = iprintk (tty, 0, (char *) fmt, ap);

   va_end (ap);
   return count;
}


#define SIZE_OF_BUF 64
static int iprintk (uint64_t addr, int inc, const char *fmt, va_list ap)//FG
{
   register char *tmp;
   int val, i = 0, count = 0;
   char buf[SIZE_OF_BUF], *xdigit;

   while (*fmt)
   {
      while ((*fmt != '%') && (*fmt))
      {
         mmu_physical_sb(addr, *fmt++);
	     // *addr = *fmt++;
         addr += inc;
         count++;
      }
      if (*fmt)
      {
         fmt++;
         switch (*fmt)
         {
         case '%':
            mmu_physical_sb(addr, '%');
            //*addr = '%';
            addr += inc;
            count++;
            goto next;
         case 'c':
            mmu_physical_sb(addr, va_arg (ap, int));
            //*addr = va_arg (ap, int);
            addr += inc;
            count++;
            goto next;
         case 'd':
         case 'i':
            val = va_arg (ap, int);
            if (val < 0)
            {
               val = -val;
               mmu_physical_sb(addr, '-');
            //   *addr = '-';
               addr += inc;
               count++;
            }
            tmp = buf + SIZE_OF_BUF;
            *--tmp = '\0';
            do
            {
               *--tmp = val % 10 + '0';
               val /= 10;
            }
            while (val);
            break;
         case 'u':
            val = va_arg (ap, unsigned int);
            tmp = buf + SIZE_OF_BUF;
            *--tmp = '\0';
            do
            {
               *--tmp = val % 10 + '0';
               val /= 10;
            }
            while (val);
            break;
         case 'o':
            val = va_arg (ap, int);
            tmp = buf + SIZE_OF_BUF;
            *--tmp = '\0';
            do
            {
               *--tmp = (val & 7) + '0';
               val = (unsigned int) val >> 3;
            }
            while (val);
            break;
         case 's':
            tmp = va_arg (ap, char *);
            if (!tmp)
               tmp = "empty str";
            break;
         case 'p':
         case 'x':
         case 'X':
            val = va_arg (ap, int);
            tmp = buf + SIZE_OF_BUF;
            *--tmp = '\0';
            if (*fmt != 'X')
               xdigit = "0123456789abcdef";
            else
               xdigit = "0123456789ABCDEF";
            do
            {
               *--tmp = xdigit[val & 15];
               val = (unsigned int) val >> 4;
               i++;
            }
            while (val);
            if (*fmt == 'p')
               while (i < 8)
               {
                  *--tmp = xdigit[0];
                  i++;
               }
            break;
         default:
            mmu_physical_sb(addr, *fmt);
            //*addr = *fmt;
            count++;
            goto next;
         }
         while (*tmp)
         {
            mmu_physical_sb(addr, *tmp++);
            //*addr = *tmp++;
            addr += inc;
            count++;
         }
       next:
         fmt++;
      }
   }
   va_end (ap);
   return count;
}
