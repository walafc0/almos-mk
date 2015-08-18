/*
 * cpu-config.h - CPU related configurations
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

#ifndef _CPU_CONFIG_H_
#define _CPU_CONFIG_H_

#ifndef _CONFIG_H_
#error This config-file is not to be included directely, use config.h instead
#endif

//////////////////////////////////////////////
//       CPU RELATED CONFIGURATIONS         //
//////////////////////////////////////////////
#define CONFIG_CPU_BYTE_ORDER              __LITTLE_ENDIAN
#define CONFIG_CPU_TYPE                    MIPS32
#define CONFIG_CPU_NAME                    "mipsel"
#define CONFIG_CPU_ABI_NAME                "mips32"
#define CONFIG_CPU_IRQ_NR                  6
#define CONFIG_CACHE_LINE_LENGTH           16
#define CONFIG_CACHE_LINE_SIZE             64
#define CONFIG_CPU_64BITS                  no
#define CONFIG_BACKOFF_SPINLOCK            no
#define CONFIG_CPU_BACKOFF_SPINLOCK        no
#define CONFIG_DIRECT_SPINLOCK             no
//////////////////////////////////////////////

#endif	/* _CPU_CONFIG_H_ */
