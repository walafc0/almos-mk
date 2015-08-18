/*
 * bits.h - bits manipulation helper functions
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

#ifndef _BITS_H_
#define _BITS_H_

#include <types.h>

#define BITMAP_SIZE(size) ((size / sizeof(uint_t)))
#define __CPU_WIDTH (8*sizeof(uint_t))

#define ARROUND_UP(val, size) (((val) & ((size) -1)) ? ((val) & ~((size)-1)) + (size) : (val))
#define ARROUND_DOWN(val, size)  ((val) & ~((size) - 1))

#define ABS(x) (((x) < 0) ? -(x) : (x))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX(x,y) (((x) < (y)) ? (y) : (x))

/** 
 * Declare bitmap data structure. 
 * Size is expressed in number of bytes, it must be at least 8.
 * Must be used if and only if bitmap exceed 32 bits.
 */

#define BITMAP_DECLARE(name,size) uint_t name[BITMAP_SIZE(size)]

typedef uint_t bitmap_t;

/** Set specific bit */
static inline void bitmap_set(bitmap_t *bitmap, uint_t index)
{
	register uint_t i = index / __CPU_WIDTH;
	index %= __CPU_WIDTH;

	bitmap[i] |= ( 1UL << index);
}

/** Clear specific bit */
static inline void bitmap_clear(bitmap_t *bitmap, uint_t index)
{
	register uint_t i = index / __CPU_WIDTH;
	index %= __CPU_WIDTH;

	bitmap[i] &= ~( 1UL << index);
}

/** Get specific bit state */
static inline int bitmap_state(bitmap_t *bitmap, uint32_t index)
{
	register uint_t i = index / __CPU_WIDTH;
	index %= __CPU_WIDTH;

	return (bitmap[i] & (1UL << index)) && 1;
}

/** Set specific range of bits, range is [index ... (index + len)[ */
void bitmap_set_range(bitmap_t *bitmap, register sint_t index, register sint_t len);

/** Clear specific range of bits, range is [index ... (index + len)[ */
void bitmap_clear_range(bitmap_t *bitmap, register sint_t index, register sint_t len);

/** Find the index of first set bit starting from index or -1 if no bit found */
sint_t bitmap_ffs2(bitmap_t *bitmap, uint_t index, size_t size);

/** Find the index of first clear bit starting from index or -1 if no bit found */
sint_t bitmap_ffc2(bitmap_t *bitmap, uint_t index, size_t size);

/** Find the index of first set bit or -1 if no bit found */
static inline sint_t bitmap_ffs(bitmap_t *bitmap, size_t size)
{
	register uint_t i,j;

	size <<= 3;			/* convert size from bytes to bits */

	for(i = 0; i < size/__CPU_WIDTH; i++)
	{
		if(bitmap[i] != 0)
		{
			for(j = 0; j < __CPU_WIDTH; j++)
			{
				if(bitmap[i] & (1UL << j))
					return (i*__CPU_WIDTH + j);
			}
		}
	}

	return -1;
}

/** Find the index of first clear bit or -1 if no bit found */
static inline sint_t bitmap_ffc(bitmap_t *bitmap, size_t size)
{
	register uint_t i,j;

	size <<= 3;			/* convert size from bytes to bits */

	for(i = 0; i < size/__CPU_WIDTH; i++)
	{
		if(bitmap[i] != (uint_t)-1)
		{
			for(j = 0; j < __CPU_WIDTH; j++)
			{
				if((bitmap[i] & (1UL << j)) == 0)
					return (i*__CPU_WIDTH + j);
			}
		}
	}

	return -1;
}


static inline uint_t bits_nr(uint_t val)
{
	register uint_t i;

	for(i=0; val > 0; i++) 
		val = val >> 1;

	return i;
}

static inline uint_t bits_log2(uint_t val)
{
	return (val == 0) ? 1 : bits_nr(val) - 1;
}

#endif	/* _BITS_H_ */
