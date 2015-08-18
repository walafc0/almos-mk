/*
 * libk.h - helper functions
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

#ifndef _LIBK_H_
#define _LIBK_H_

#include <stdarg.h>
#include <types.h>
#include <string.h>
#include <ctype.h>

int iprintk (char *addr, uint_t cid, int inc, const char *fmt, va_list ap);
int sprintk (char *s, char *fmt, ...);
void srand(unsigned int seed);
uint_t rand(void);
int atoi(const char *nptr);

struct task_s;
error_t elf_load_task(char *pathname, struct task_s *task);

//From Linux
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#endif	/* _LIBK_H_ */
