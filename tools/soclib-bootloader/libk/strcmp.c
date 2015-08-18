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
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
   Copyright Franck Wajsbürt <franck.wajsburt@lip6.fr>
*/

#include <string.h>
#include <types.h>

int strcmp (const char *s1, const char *s2)
{
  if((s1 == NULL) || (s2 == NULL))
    return s1 - s2;
  
   while (*s1 && *s1 == *s2)
   {
     s1 ++;
     s2 ++;
   }
  
   return (*s1 - *s2);
}

int strncmp(const char *s1, const char *s2, size_t n)
{ 
  if((s1 == NULL) || (s2 == NULL) || (n == 0))
    return s1 - s2;
  
  while (*s1 && (*s1 == *s2) && (n != 0))
  {
     s1 ++;
     s2 ++;
     n--;
  }
  
  return (*s1 - *s2);
}
