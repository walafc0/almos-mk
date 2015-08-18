/*
 * ctype.c - characters related helper functions
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

#include <types.h>

int isprint(int c)
{
	return (127 - ' ') > (uint8_t)(c - ' ') ? 1 : 0;
}

int isalpha(int c)
{
	return ((uint8_t)((c | 0x20) - 'a')) < 26 ? 1 : 0;
}

int isspace(int c)
{
	return ((c == ' ') || (c - 9) < 5);
}

int toupper(int c)
{
	return isalpha(c) ? c & ~0x20 : c;
}
