/*
 * arch.c - architecture related configurations
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

#ifndef _ARCH_CONFIG_H_
#define _ARCH_CONFIG_H_

#ifndef _CONFIG_H_
#error This config-file is not to be included directely, use config.h instead
#endif

//////////////////////////////////////////////
//      ARCH RELATED CONFIGURATIONS         //
//////////////////////////////////////////////
#define CONFIG_USR_OFFSET       0x00C00000
#define CONFIG_USR_LIMIT        0x01F00000
#define CONFIG_CPU_BITS_NR               2
#define CONFIG_CLUSTER_BITS_NR           2
#define CONFIG_CLUSTER_NR                1
#define CONFIG_CPU_PER_CLUSTER_NR        1
#define CONFIG_CPU_NR                    1
#define CONFIG_IO_CLUSTER_ID             0
#define CONFIG_BOOT_CLUSTER_ID           0
#define CONFIG_BOOT_CPU_ID               0
#define CONFIG_TTY_DEV_NR                1
#define CONFIG_DMSG_KLOG_TTY             0
#define CONFIG_DMSG_BOOT_TTY             0
#define CONFIG_DMSG_ISR_TTY              0
#define CONFIG_DMSG_EXCEPT_TTY           0
#define CONFIG_KSH_TTY                   0
#define CONFIG_TIMER_WITH_IRQ            yes
#define CONFIG_TTY_ECHO_MODE             yes
#define CONFIG_FB_USE_DMA                no
//////////////////////////////////////////////


#endif	/* _ARCH_CONFIG_H_ */
