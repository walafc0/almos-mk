/*
 * bits.c - bits manipulation helper functions
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
#include <bits.h>
#include <kdmsg.h>

/** Find the index of first set bit starting from index or -1 if no bit found */
sint_t bitmap_ffs2(bitmap_t *bitmap, uint_t index, size_t size)
{
	register uint_t i,j;
  
	i = index / __CPU_WIDTH;
	j = index % __CPU_WIDTH;
 
	size <<= 3;			/* convert size from bytes to bits */
  
	if(j != 0)
	{
		for(; j < __CPU_WIDTH; j++)
		{
			if(bitmap[i] & (1UL << j))
				return (i*__CPU_WIDTH + j);
		}
    
		i++;
	}

	for(; i < size/__CPU_WIDTH; i++)
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


/** Find the index of first clear bit starting from index or -1 if no bit found */
sint_t bitmap_ffc2(bitmap_t *bitmap, uint_t index, size_t size)
{
	register uint_t i,j;
  
	i = index / __CPU_WIDTH;
	j = index % __CPU_WIDTH;
 
	size <<= 3;			/* convert size from bytes to bits */
  
	if(j != 0)
	{
		for(; j < __CPU_WIDTH; j++)
		{
			if((bitmap[i] & (1UL << j)) == 0)
				return (i*__CPU_WIDTH + j);
		}
    
		i++;
	}

	for(; i < size/__CPU_WIDTH; i++)
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


/** Set specific range of bits, range is [index ... (index + len)[ */
void bitmap_set_range(bitmap_t *bitmap, register sint_t index, register sint_t len)
{
	register uint_t i;
	register uint_t val;
  
	i = index / __CPU_WIDTH;
	index %= __CPU_WIDTH;
  
	while((len > 0))
	{
		if((len + index) >= __CPU_WIDTH)
		{
			val = (index == 0) ? -1UL : (1UL << (__CPU_WIDTH - index)) - 1UL;
			bitmap[i] |= (val << index);
			i++;
			len -= (__CPU_WIDTH - index);
			index = 0;
		}
		else
		{
			bitmap[i] |= (((1UL << len ) - 1) << index);
			break;
		}
	}
}


/** Clear specific range of bits, range is [index ... (index + len)[ */
void bitmap_clear_range(bitmap_t *bitmap, register sint_t index, register sint_t len)
{
	register uint_t i;
	register uint_t val;

	i = index / __CPU_WIDTH;
	index %= __CPU_WIDTH;

	while((len > 0))
	{
		if((len + index) >= __CPU_WIDTH)
		{
			val = (index == 0) ? -1UL : (1UL << (__CPU_WIDTH - index)) - 1UL;
			bitmap[i] &= ~(val << index);
			i++;
			len -= (__CPU_WIDTH - index);
			index = 0;
		}
		else
		{
			if(len)
			{
				bitmap[i] &= ~(((1UL << len ) - 1UL) << index);
				break;
			}
		}
	}
}
