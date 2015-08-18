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
  
   UPMC / LIP6 / SOC (c) 2007 - 2009
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

void *memcpy (void *dest, void *src, unsigned long size)
{
   register unsigned int i;
   register unsigned int isize;
   
   if(((int)dest & 0x3) || ((int)src & 0x3))
   {
     for (i = 0; i < size; i++)
       *((char *) dest + i) = *((char *) src + i);
     
     return dest;
   }
   
   isize = size >> 2;

   for(i=0; i< isize; i++)
     *((unsigned int*) dest + i) = *((unsigned int*) src + i);
   
   for(i<<= 2; i< size; i++)
     *((char*) dest + i) = *((char*) src + i);

   return dest;
}

