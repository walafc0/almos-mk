/*
 * hardware.h - architecture specific macros
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

#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <config.h>
#include <segmentation.h>
#include <types.h>
#include <system.h>
#include <screen.h>
/* Addresses of the peripheral devices */
#define __pic_addr        ((uint_t*)PIC_BASE)
#define __tty_addr        ((uint_t*)TTY_BASE)
#define __fb_addr         ((uint_t*)FB_BASE)
#define __timer_addr      ((uint_t*)TIMER_BASE)
#define __dma_addr        ((uint_t*)DMA_BASE)
#define __blk_addr        ((uint_t*)BD_BASE)

#define IO_CLUSTER_ID     CONFIG_IO_CLUSTER_ID

/* Terminals related macros */
#define TTY_DEV_NR        CONFIG_TTY_DEV_NR
#define SYS_TTY           0
#define TTY_BUFFER_DEPTH  40

/* Timer parameters macros */
#define TIMER_DEV_NR      CONFIG_TIMER_DEV_NR

#endif	/* _HARDWARE_H_ */
