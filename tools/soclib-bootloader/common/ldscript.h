/*
 modification : entry pointe désormais sur le programme assembleur de boot.c -> reset_almos*/
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
  
   UPMC / LIP6 / SOC (c) 2009
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#include "segmentation_for_ld.h"

ENTRY(boot_cluster_reset)

SECTIONS
{
   .boot BOOT_BASE :
   { 
	__boot_loader_base = . ;
	. = ALIGN(0x1000);
	__kernel_base = . ;
	INCLUDE ".build/kernel-img.ld";
	__kernel_end = . ;
	. = ALIGN(0x1000);
	__boot_base = . ;
	*(.boot) 
	*(.text)
	*(.data*)
	*(.rodata*)
	*(.bss*)
	*(COMMON*)
	*(.scommon*)
	__boot_end = . ; 
	. = ALIGN(0x1000);
	__boot_info_block_base = . ;
	INCLUDE ".build/arch-info.ld";
	__boot_info_block_end = . ;
	__boot_loader_end = . ;
   } 
}
