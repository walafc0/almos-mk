/*
 * atoi.c - convert a string to an integer
 *
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
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

int atoi(const char *nptr)
{
	sint_t num;
	sint_t signe;
  
	while(isspace(*nptr));
  
	signe = (*(nptr) == '-') ? -1 : +1;
	num = 0;

	while(*(nptr))
	{   
		if((*nptr >= '0') && (*nptr <= '9'))
			num = (num * 10) + (*nptr - '0');
    
		nptr ++;
	}
    
	return num * signe;
}
