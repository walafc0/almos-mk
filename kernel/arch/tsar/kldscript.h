/*
 * kldscript.h - kernel ldscript
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

#define _CONFIG_H_
#include "arch-config.h"

vaddr = CONFIG_KERNEL_OFFSET;

SECTIONS
{
  .ktext vaddr : {
    __ktext_start = .;
    *(.kentry)
    __kentry_end = .;
    __uspace_start = .;
    *(.uspace)
    __uspace_end   = .;
    __virtual_end  = .;
      *(.text) 
      *(.rodata*) 
      . = ALIGN(0x1000); 
    __ktext_end = .;
  }

  .kdata : {
    __kdata_start = .;
    *(.data*) 
      *(.bss*) 
      *(COMMON*) 
      *(.scommon*) 
      . = ALIGN(0x1000); 
    __kdata_end = .;
    __heap_start = .;
    *(.kheader)//this will be overwriten by the heap manager
  }
}

