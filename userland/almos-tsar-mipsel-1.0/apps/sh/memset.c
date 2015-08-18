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
#include <stdint.h>

void *_memset_ (void *s, int c, unsigned int size)
{
  register char *ptr = s;
  register unsigned int *iptr;
  register unsigned int val;
  register unsigned int isize;
  register unsigned int iamount;

  while(((uint32_t) ptr & 0x3) && size && size--)
    *(ptr++) = (char) c;

  if(!size) return s;

  val = c << 24 | c << 16 | c << 8 | (c & 0xFF); 
  isize = size >> 2;
  iamount = isize << 2;
  iptr = (unsigned int*) ptr;
  
  while(isize--)
    *(iptr++) = val;
 
  size = size - iamount;
  ptr = (char*) iptr;
  
  while(size--)
    *(ptr++) = (char) c;

  return s;
}

char tab[13];

int main()
{
  _memset_(tab, 'A', sizeof(tab));
  tab[sizeof(tab)-1] = 0;
  printf("tab %s\n",tab);
  return 0;
}
