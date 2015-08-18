/*
 * memcpy.c - architecture independent memory copy function
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
#include <cpu.h>

/* TODO: introduce the possiblity to use architecture specific 
 * more optimized version if any */

void do_memcpy_8(void *dst, void *src, size_t size)
{
	register uint32_t b0;
	register uint32_t b1;
	register uint32_t b2;
	register uint32_t b3;
	register uint32_t b4;
	register uint32_t b5;
	register uint32_t b6;
	register uint32_t b7;

	register uint32_t *src_ptr;
	register uint32_t *dst_ptr;
	register size_t count;
	register size_t i;

	src_ptr = src;
	dst_ptr = dst;
	count   = size;
  
	for(i=0; i < count; i++)
	{
		b0 = src_ptr [0];
		b1 = src_ptr [1];
		b2 = src_ptr [2];
		b3 = src_ptr [3];
		b4 = src_ptr [4];
		b5 = src_ptr [5];
		b6 = src_ptr [6];
		b7 = src_ptr [7];

		dst_ptr[0] = b0;
		dst_ptr[1] = b1;
		dst_ptr[2] = b2;
		dst_ptr[3] = b3;
		dst_ptr[4] = b4;
		dst_ptr[5] = b5;
		dst_ptr[6] = b6;
		dst_ptr[7] = b7;

		if((i & 0x1) == 1)
			cpu_invalid_dcache_line(src_ptr);

		src_ptr += 8;
		dst_ptr += 8;
	}
}


void *memcpy (void *dest, void *src, unsigned long size)
{
	register uint_t i;
	register uint_t isize;
	register uint_t count;

	if(((uintptr_t)dest & 0x3) || ((uintptr_t)src & 0x3))
	{
		for (i = 0; i < size; i++)
			*((char *) dest + i) = *((char *) src + i);
    
		return dest;
	}

#if 1
	isize = size >> 5;
    
	if(isize != 0)
		do_memcpy_8(dest, src, isize);

	count = isize << 5;
	size -= count;

	if(size == 0) return dest;
  
	dest = (uint8_t*) dest + count;
	src  = (uint8_t*) src  + count;
#endif

	isize = size >> 2;

	for(i=0; i< isize; i++)
		*((uint32_t*) dest + i) = *((uint32_t*) src + i);
   
	for(i<<= 2; i< size; i++)
		*((char*) dest + i) = *((char*) src + i);

#if 1
	isize = size >> 6;

	for(i=0; i < isize; i++)
		cpu_invalid_dcache_line(((char *)src + (i << 6)));
#endif

	return dest;
}

