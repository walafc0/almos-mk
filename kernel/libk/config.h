/*
 * config.h - unified kernel configurations file
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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define no    0
#define yes   1

#define DMSG_DEBUG         6
#define DMSG_INFO          5
#define DMSG_WARNING       4
#define DMSG_ASSERT        3
#define DMSG_BOOT          2
#define DMSG_ERROR         1
#define DMSG_NONE          0

//////////////////////////////////////////////
//       CPU RELATED CONFIGURATIONS         //
//////////////////////////////////////////////
#define  MIPS32              0x0001
#define  I386                0x0002
#define  __LITTLE_ENDIAN     0x0001
#define  __BIG_ENDIAN        0x0002
#include <cpu-config.h>


//////////////////////////////////////////////
//      ARCH RELATED CONFIGURATIONS         //
//////////////////////////////////////////////
#include <arch-config.h>

#define IN_USPACE(addr) \
((CONFIG_USR_OFFSET <= ((long)addr)) && (((long)addr) <= CONFIG_USR_LIMIT))

#define NOT_IN_USPACE(addr) (!(IN_USPACE(addr)))

#define IN_KSPACE(addr) (NOT_IN_USPACE(addr))



//////////////////////////////////////////////
//     MEMORY MANAGEMENT CONFIGURATIONS     // 
//////////////////////////////////////////////
#include <mm-config.h>


//////////////////////////////////////////////
//      KERNEL CONFIGURATIONS               //
//////////////////////////////////////////////
#include <kernel-config.h>

//////////////////////////////////////////////
#endif	/* _CONFIG_H_ */
