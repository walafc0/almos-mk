/*
 * __boot.S - kernel multiboot header
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

.section .boot,"ax",@progbits
.global _start, start
.extern kboot_entry
	
#define MULTIBOOT_HEADER_MAGIC  0x1BADB002
#define MULTIBOOT_HEADER_FLAGS	0x00000003
#define CHECKSUM -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
	
_start:
	jmp start

/* -------- The Multiboot header -------- */
.align 4
multiboot_header:
.int MULTIBOOT_HEADER_MAGIC
.int MULTIBOOT_HEADER_FLAGS
.int CHECKSUM     
/* ----- Multiboot Header Ends Here ----- */
.extern tempGdt
.extern sys_stack
start:
	pushl 	%ebx
	call 	kboot_entry
	cli 
	hlt

	
.section .bss
.global __boot_stack
.space	4096
__boot_stack: