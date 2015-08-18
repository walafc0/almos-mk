/*
 * arch.c - architecture related configurations
 *
 * Copyright (c) 2007,2008,2009,2010,2011,2012 Ghassan Almaless
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
#define CONFIG_ARCH_NAME             "tsar"
#define CONFIG_THREAD_PAGE_ORDER     0x1
#define RUN_IN_PHYSICAL_MODE         yes
#define CONFIG_PHYSICAL_BITS	     40
#define CONFIG_LOCAL_PHYSICAL_BITS   32//NUMBER of bits that are local to a cluster

/* Kepping the same value for KERNEL_OFFSET allow us to 
 * have the same kernel for both mode of addressing */
#if RUN_IN_PHYSICAL_MODE

 /* kernel space allocated virtual addresses */
 #define CONFIG_KERNEL_OFFSET         0x00000000
 #define CONFIG_KERNEL_LIMIT          0x00003FFF //approximated may require more (if less no problem)
 #define CONFIG_DEVREGION_SIZE        0x00000000 //empty
 /* user space allocated virtual addresses */
 #define CONFIG_USR_OFFSET            0x00004000
 #define CONFIG_USR_LIMIT             0x80000000//0xFFFFFFFF

#else /*!RUN_IN_PHYSICAL_MODE*/

 /* kernel space allocated virtual addresses */
 #define CONFIG_KERNEL_OFFSET         0x00000000
 #define CONFIG_KERNEL_LIMIT          0x3FFFFFFF
 #define CONFIG_DEVREGION_SIZE        0x00000000 //Dynamically allocated from kernel space
 /* drivers are physically accessed !? */
 /* user space allocated virtual addresses */
 #define CONFIG_USR_OFFSET            0x40000000
 #define CONFIG_USR_LIMIT             0xFFFFFFFF

#endif

#define CONFIG_PPN_BITS_NR                20
#define CONFIG_TTY_ECHO_MODE              yes
#define CONFIG_TTY_MAX_DEV_NR             32
#define CONFIG_TTY_BUFFER_DEPTH           40
#define CONFIG_FB_USE_DMA                 no
#define CONFIG_ARCH_HAS_BARRIERS          no
//////////////////////////////////////////////

#endif	/* _ARCH_CONFIG_H_ */
