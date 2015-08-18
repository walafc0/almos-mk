/*
 * strchr.c - string compare helper functions
 *
 * Copyright (c) 2007,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS-kernel.
 *
 * ALMOS-kernel is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS-kernel is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS-kernel; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string.h>
#include <types.h>
#include <ctype.h>

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

int strcasecmp (const char *str1, const char *str2)
{
	char c1;
	char c2;

	if((str1 == NULL) || (str2 == NULL))
		return str1 - str2;
  
	do{
		c1 = toupper(*++str1);
		c2 = toupper(*++str2);
	}while(c1 && c1 == c2);
  
	return (c1 - c2);
}

int strncmp(const char *s1, const char *s2, size_t n)
{ 
	if((s1 == NULL) || (s2 == NULL) || (n == 0))
		return s1 - s2;
  
	while (*s1 && (*s1 == *s2) && (n > 1))
	{
		s1 ++;
		s2 ++;
		n--;
	}
  
	return (*s1 - *s2);
}
