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
  
   UPMC / LIP6 / SOC (c) 2007-2009
   Copyright Ghassan Almaless <ghassan.almaless@lip6.fr>
*/

#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>

#define __CONFIG_CACHE_LINE_LENGTH 16
#define __CONFIG_CACHE_LINE_SIZE   64

/* General constants and return values */
#ifndef NULL
#define NULL (void*)        0
#endif
#define FALSE               0
#define false               0
#define TRUE                1
#define true                1

#define EXIT_SUCCESS        0
#define EXIT_FAILURE        1

typedef unsigned long uint_t;
typedef signed long sint_t;
typedef unsigned long bool_t;

typedef uint_t time_t;
typedef uint_t dev_t;
typedef uint_t ino_t;
typedef uint_t nlink_t;
typedef int32_t off_t;
typedef uint_t blksize_t;
typedef uint_t blkcnt_t;
typedef uint32_t fpos_t;
typedef signed long error_t;

typedef uint16_t      cid_t;
typedef unsigned long pid_t;
typedef unsigned long uid_t;
typedef uint_t gid_t;

/* Page Related Types */
typedef unsigned long ppn_t;
typedef unsigned long vma_t;
typedef unsigned long pma_t;

/* Time Related Types */
typedef uint64_t clock_t;

/* Aligned Variable */
struct __cache_line_s
{
  uint32_t value;
  uint32_t values[__CONFIG_CACHE_LINE_LENGTH - 1];
}__attribute__((packed));

typedef struct __cache_line_s __cacheline_t;

#define __CACHELINE __attribute__((aligned(__CONFIG_CACHE_LINE_SIZE)))

#endif	/* _TYPES_H_ */
