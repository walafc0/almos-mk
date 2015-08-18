/*
 * cpu-io.c - cpu io access
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
#include <cpu-io.h>

void cpu_io_out8(uint_t port, uint8_t value)
{
	__asm__ volatile 
		("movl     %%eax,   %%ecx   \n"
		 "outb     %%al,    %%dx    \n"
		 "jmp      1f               \n"
		 "1:                        \n"
		 "movl     %%ecx,   %%eax   \n"
		 : : "a" (value), "d" (port));
}

void cpu_io_out16(uint_t port, uint16_t value)
{
	__asm__ volatile 
		("outw     %%ax,    %%dx    \n"
		 "jmp      1f               \n"
		 "1:                        \n"
		 : : "a" (value), "d" (port));
}

void cpu_io_out32(uint_t port, uint32_t value)
{
	__asm__ volatile 
		("outl     %0,    %1  \n"
		 "jmp      1f               \n"
		 "1:                        \n"
		 : : "a" (value), "d" ((uint16_t)port));
}


uint8_t cpu_io_in8(uint_t port)
{
	register uint8_t value;
  
	__asm__ volatile 
		("inb      %%dx,     %%al   \n"
		 : "=a" (value) : "d" (port));
  
	return value;
}

uint16_t cpu_io_in16(uint_t port)
{
	register uint16_t value;
  
	__asm__ volatile 
		("inw      %%dx,     %%ax   \n"
		 : "=a" (value) : "d" (port));
  
	return value;
}

uint32_t cpu_io_in32(uint_t port)
{
	register uint32_t value;
  
	__asm__ volatile 
		("inl      %1,     %0   \n"
		 : "=a" (value) : "d" ((uint16_t)port));
  
	return value;
}
