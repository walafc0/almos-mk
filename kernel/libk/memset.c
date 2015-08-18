/*
 * memset.c - architecture independent memory set function
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

/* TODO: introduce the possiblity to use architecture specific 
 * more optimized version if any */

void do_memset_8(void *dst, uint32_t val, size_t size)
{
	volatile uint32_t *dst_ptr;
	register uint32_t *limit_ptr;
  
	dst_ptr   = dst;
	limit_ptr = (uint32_t*)((uint8_t*)dst + (size << 5));
  
	while(dst_ptr != limit_ptr)
	{
		dst_ptr[0] = val;
		dst_ptr[1] = val;
		dst_ptr[2] = val;
		dst_ptr[3] = val;
		dst_ptr[4] = val;
		dst_ptr[5] = val;
		dst_ptr[6] = val;
		dst_ptr[7] = val;

		dst_ptr += 8;
	}
}

#include <kdmsg.h>
#include <cpu.h>
void *memset(void *s, int c, unsigned int size)
{
	register uint8_t  *ptr = s;
	register uint32_t *wptr;
	register uint32_t val;
	register uint32_t isize;
	register uint32_t count;
  
	c&=0xFF;
	ptr = s;

	while(((uint32_t) ptr & 0x3) && size && size--)
		*(ptr++) = (uint8_t) c;

	if(size == 0) return s;

	val = c << 24 | c << 16 | c << 8 | (c & 0xFF);
	isize = size >> 5;

	if(isize != 0)
		do_memset_8(ptr, val, isize);

	count = isize << 5;
	size -= count;
  
	if(size == 0) return s;

	wptr = (uint32_t*)(ptr + count);
	isize = size >> 2;
	count = isize;

	while(isize--)
		*(wptr++) = val;

	size = size - (count << 2);
	ptr = (uint8_t*)wptr;

	while(size--)
		*(ptr++) = (uint8_t)c;
  
	return s;
}
