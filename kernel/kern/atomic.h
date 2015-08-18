/*
 * kern/atomic.h - atomic operations interface
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

#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include <config.h>
#include <types.h>

struct atomic_s
{
	volatile sint_t val;
	uint_t pad[CONFIG_CACHE_LINE_LENGTH - 1];
}__attribute__ ((packed));

typedef struct atomic_s atomic_t;
typedef uint_t refcount_t;

/** Static initializer  */
#define ATOMIC_INITIALIZER  {.val = 0, .pad = {0}}


inline void   atomic_init(atomic_t *ptr, sint_t val);

inline sint_t atomic_inc(atomic_t *ptr);

inline sint_t atomic_add(atomic_t *ptr, sint_t val);

inline sint_t atomic_dec(atomic_t *ptr);

inline bool_t atomic_cas(atomic_t *ptr, sint_t old, sint_t new);

inline sint_t atomic_get(atomic_t *ptr);

inline sint_t atomic_set(atomic_t *ptr, sint_t val);

inline bool_t atomic_test_set(atomic_t *ptr, sint_t val);

/* Refcount operations */

inline void   refcount_init(refcount_t *ptr); 

inline uint_t refcount_up(refcount_t *ptr);

inline uint_t refcount_down(refcount_t *ptr);

inline uint_t refcount_set(refcount_t *ptr, uint_t new_val);

inline uint_t refcount_get(refcount_t *ptr);

#endif	/* _ATOMIC_H_ */
