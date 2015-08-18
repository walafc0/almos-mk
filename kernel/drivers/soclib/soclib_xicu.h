/*
 * soclib_xicu.c - soclib xicu driver
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

#ifndef _SOCLIB_XICU_H_
#define _SOCLIB_XICU_H_

#include <spinlock.h>
#include <bits.h>

#define XICU_HWI_MAX    32
#define XICU_PTI_MAX    32
#define XICU_WTI_MAX    32
#define XICU_CNTR_MAX   32
#define XICU_CNTR_MASK  0xFFFFFFFF

#define XICU_HWI_TYPE    XICU_MSK_HWI
#define XICU_PTI_TYPE    XICU_MSK_PTI
#define XICU_WTI_TYPE    XICU_MSK_WTI
#define XICU_CNTR_TYPE   XICU_MSK_CNTR

/** Number of IRQ per processor**/
#define OUTPUT_IRQ_PER_PROC     4

/** XICU Mapped Functions  */
#define XICU_WTI_REG            0
#define XICU_PTI_PER            1
#define XICU_PTI_VAL            2
#define XICU_PTI_ACK            3
#define XICU_MSK_PTI            4
#define XICU_MSK_PTI_ENABLE     5
#define XICU_MSK_PTI_DISABLE    6
#define XICU_PTI_ACTIVE         6
#define XICU_MSK_HWI            8
#define XICU_MSK_HWI_ENABLE     9
#define XICU_MSK_HWI_DISABLE    10
#define XICU_HWI_ACTIVE         10
#define XICU_MSK_WTI            12
#define XICU_MSK_WTI_ENABLE     13
#define XICU_MSK_WTI_DISABLE    14
#define XICU_WTI_ACTIVE         14
#define XICU_PRIO               15
#define XICU_CNTR_REG           16
#define XICU_MSK_CNTR           17
#define XICU_MSK_CNTR_ENABLE    17
#define XICU_MSK_CNTR_DISABLE   18
#define XICU_CNTR_ACTIVE        18


sint_t  soclib_xicu_barrier_init(struct device_s *xicu, struct event_s *event, uint_t count);
sint_t  soclib_xicu_barrier_wait(struct device_s *xicu, uint_t barrier_id);
error_t soclib_xicu_barrier_destroy(struct device_s *xicu, uint_t barrier_id);
error_t soclib_xicu_ipi_send(struct device_s *xicu, uint_t port, uint32_t val);

extern driver_t soclib_xicu_driver;

#endif	/* _SOCLIB_ICU_H_ */
