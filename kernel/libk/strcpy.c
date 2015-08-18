/*
 * strcpy.c - string copy helper functions
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

char* strcpy (char *dest, char *src)
{
	char *src_ptr = src;
	char *dst_ptr = dest;

	if(!dest || !src)
		return dest;

	while(*src_ptr)
		*(dst_ptr++) = *(src_ptr++);
  
	*dst_ptr = 0;
	return dest;
}


char* strncpy(char *dest, char *src, size_t n)
{
	size_t i;

	for (i = 0; (i < n) && (src[i] != '\0') ; i++)
		dest[i] = src[i];

	for (; i < n; i++)
		dest[i] = '\0';
  
	return dest;
}
