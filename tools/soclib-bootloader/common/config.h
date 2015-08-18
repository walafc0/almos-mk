/*
   This file is part of AlmOS.
  
   AlmOS is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   AlmOS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with AlmOS; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
   UPMC / LIP6 / SOC (c) 2007, 2008, 2009
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define yes 1
#define no  0

/////////////////////////////////////////////////
//         CPU Related Configurations          //
/////////////////////////////////////////////////
#include <cpu-config.h>
/////////////////////////////////////////////////


/////////////////////////////////////////////////
//           General Configurations            //
/////////////////////////////////////////////////
#define CONFIG_BOOT_USE_DMA             no
#define CONFIG_CDMA                     no
#define CONFIG_BOOT_STACK_SIZE          0x1000
//FG
#define XICU_INDEX                      1
#define HUGE_PAGE_SIZE                  0x200000
/////////////////////////////////////////////////


/////////////////////////////////////////////////
//           DEBUG Configurations              //
/////////////////////////////////////////////////
#define  CONFIG_MEM_DEBUG               no
/////////////////////////////////////////////////

/////////////////////////////////////////////////
//           KERNEL Configurations             //
/////////////////////////////////////////////////
#define  VIRT_KSPACE_SIZE               0x60000000
#define  WITH_VIRTUAL_ADDR              no
/////////////////////////////////////////////////

#endif	/* _CONFIG_H_ */
