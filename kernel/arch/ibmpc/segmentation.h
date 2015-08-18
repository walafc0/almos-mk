/*
 * segmentation.h - physical address space known segments
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

//////////////////////////////////////////////
//         Devices mapped segments 
//////////////////////////////////////////////
#define     TIMER_BASE  0xd3200000
#define     TIMER_SIZE  0x00000080

#define     PIC_BASE    0x00000020

#define     TTY_XSIZE   80
#define     TTY_YSIZE   25
#define     TTY_BASE    0x000B8000  
#define     TTY_SIZE    TTY_XSIZE*TTY_YSIZE*2

#define     ATA0_DRIVE0 0x000001F0
#define     ATA0_DRIVE1 0x000101F0

#define     ATA1_DRIVE0 0x00000170
#define     ATA1_DRIVE1 0x00010170
//////////////////////////////////////////////
