/*
 * arch-config.h - architecture related configurations
 * 
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012 UPMC Sorbonne Universites
 *
 * This file is part of ALMOS.
 *
 * ALMOS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.0 of the License.
 *
 * ALMOS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ALMOS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _ARCH_CONFIG_H_
#define _ARCH_CONFIG_H_

#ifndef _CONFIG_H_
#error This config-file is not to be included directely, use config.h instead
#endif

/////////////////////////////////////////////////////
//          ARCH RELATED CONFIGURATIONS            //
/////////////////////////////////////////////////////
#define CONFIG_USR_OFFSET                 0x00004000
#define CONFIG_USR_LIMIT                  0x70000000
#define CONFIG_KERNEL_OFFSET              0x80000000
#define CONFIG_KERNEL_LIMIT               0xFFFFF000
#define CONFIG_DEVREGION_OFFSET           0xFF800000
#define CONFIG_MAX_CLUSTER_NR             256
#define CONFIG_MAX_CPU_PER_CLUSTER_NR     4
#define CONFIG_PPN_BITS_NR                20
#define CONFIG_TTY_ECHO_MODE              yes
#define CONFIG_TTY_MAX_DEV_NR             32
#define CONFIG_TTY_BUFFER_DEPTH           40
#define CONFIG_ARCH_HAS_BARRIERS          0
/////////////////////////////////////////////////////


#endif	/* _ARCH_CONFIG_H_ */
