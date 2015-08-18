/*
 * string.h - string related helper functions
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

#ifndef _STRING_H_
#define _STRING_H_

#include <types.h>

void *memcpy (void *dest, void *src, unsigned long size);
void *memset (void *s, int c, unsigned int size);

int strlen(const char *s);
int strnlen (const char *str, int count);
int strcmp (const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
int strcasecmp(const char *s1, const char *s2);
//int strncasecmp(const char *s1, const char *s2, size_t n);
char* strcpy(char *dest, char *src);
char* strncpy(char *dest, char *src, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *t, int c);

#endif	/* _STRING_H_ */
