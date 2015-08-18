/*
 * types.h - common kernel types
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

#ifndef _TYPES_H_
#define _TYPES_H_

#include <config.h>
#include <stdint.h>

#ifndef NULL
#define NULL (void*)        0
#endif
#define false               0
#define true                1

/* Pthread related types */
typedef unsigned long pthread_t;
typedef unsigned long pthread_mutexattr_t;
typedef unsigned long pthread_barrier_t;
typedef unsigned long pthread_barrierattr_t;
typedef unsigned long sem_t;
typedef unsigned long pthread_cond_t;
typedef unsigned long pthread_condattr_t;
typedef unsigned long pthread_rwlock_t;
typedef unsigned long pthread_rwlockattr_t;
typedef unsigned long pthread_key_t;
typedef unsigned long uint_t;
typedef signed long sint_t;
typedef unsigned long bool_t;

typedef uint_t time_t;
typedef sint_t off_t;
typedef uint32_t fpos_t;
typedef signed long error_t;

typedef unsigned long pid_t;
typedef unsigned long uid_t;

typedef uint32_t gc_t;

/* Page Related Types */
typedef unsigned long ppn_t;
typedef unsigned long vma_t;
typedef unsigned long pma_t;

/* Time Related Types */
typedef uint64_t clock_t;

/* Aligned Variable */
struct cache_line_s
{
  union
  {
    uint_t values[CONFIG_CACHE_LINE_LENGTH];
    uint_t value;
  };
}__attribute__((packed));

typedef struct cache_line_s cacheline_t;
typedef struct cache_line_s global_t;

#define CACHELINE __attribute__((aligned(CONFIG_CACHE_LINE_LENGTH)))

#endif	/* _TYPES_H_ */
