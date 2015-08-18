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

#ifndef _CPU_CONFIG_H_
#define _CPU_CONFIG_H_

//////////////////////////////////////////////
//       CPU RELATED CONFIGURATIONS         //
//////////////////////////////////////////////

#define CONFIG_CPU_IRQ_NR                6      /* From kernel/cpu/mipsel/cpu-config.h */
#define CONFIG_CACHE_LINE_LENGTH         16
#define CONFIG_CACHE_LINE_SIZE           64
#define CONFIG_BACKOFF_SPINLOCK          no
#define CONFIG_CPU_BACKOFF_SPINLOCK      no
#define CONFIG_DIRECT_SPINLOCK           no
#define CONFIG_BROM_START                0x00000000
#define CONFIG_BROM_END                  0x10000
//////////////////////////////////////////////

#endif	/* _CPU_CONFIG_H_ */
