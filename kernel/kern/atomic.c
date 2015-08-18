/*
 * kern/atomic.c - atomic operations implementation
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
#include <cpu.h>
#include <atomic.h>
#include <libk.h>
#include <pmm.h>

inline void atomic_init(atomic_t *ptr, sint_t val)
{
	ptr->val = val;
	cpu_wbflush();
}

inline sint_t atomic_inc(atomic_t *ptr)
{
	sint_t old;

#if 0
	uint_t backoff;

	backoff = rand() % 10;
	backoff = backoff * cpu_get_id();

	while (backoff-- != 0);
#endif
  
	old = cpu_atomic_inc((void*)&ptr->val);
	return old;
}

inline sint_t atomic_add(atomic_t *ptr, sint_t val)
{
	return cpu_atomic_add((void*)&ptr->val, val);
}

inline sint_t atomic_dec(atomic_t *ptr)
{
	return cpu_atomic_dec((void*)&ptr->val);
}

inline bool_t atomic_cas(atomic_t *ptr, sint_t old, sint_t new)
{
	return cpu_atomic_cas((void*)&ptr->val, old, new);
}

inline sint_t atomic_get(atomic_t *ptr)
{
	return cpu_atomic_get((void*)&ptr->val);
}

inline sint_t atomic_set(atomic_t *ptr, sint_t val)
{
	sint_t old;
	bool_t isAtomic;

	isAtomic = false;

	while(isAtomic == false)
	{
		old      = atomic_get(ptr);
		isAtomic = atomic_cas(ptr, old, val);
	}

	return old;
}

inline bool_t atomic_test_set(atomic_t *ptr, sint_t val)
{
	sint_t old;
	bool_t isAtomic;

	isAtomic = false;

	while(isAtomic == false)
	{
		old      = atomic_get(ptr);
		if(old)	return 0;
		isAtomic = atomic_cas(ptr, old, val);
	}

	return 1;
}

/* Refcount operations */

inline void refcount_init(refcount_t *ptr)
{
	*ptr = 0;
}

inline uint_t refcount_up(refcount_t *ptr)
{
	return cpu_atomic_inc(ptr);
}

inline uint_t refcount_down(refcount_t *ptr)
{
	return cpu_atomic_dec(ptr);
}

inline uint_t refcount_set(refcount_t *ptr, uint_t new_val)
{
	uint_t old;
  
	old = *ptr;
	*ptr = new_val;
	cpu_wbflush();

	return old;
}

inline uint_t refcount_get(refcount_t *ptr)
{
	return cpu_atomic_get(ptr);
}


