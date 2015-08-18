/*
 * kern/rt_timer.h - real-time timer interface
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

#ifndef  _RT_TIMER_H_
#define  _RT_TIMER_H_

#include <types.h>

/** Real-Time timer commands */
#define RT_TIMER_STOP    0x00
#define RT_TIMER_RUN     0x01
#define RT_TIMER_ADJUST  0x02

/** Real-Time timer parameters */
struct rt_params_s
{
	uint_t rt_cmd;
	sint_t rt_value;
	void *pv_data;
};

/** Initialize Real-Time timer source */
error_t rt_timer_init(uint_t period, uint_t withIrq);

/** Read Real-Time timer current value  */
error_t rt_timer_read(uint_t *time);

/** Set Real-Time timer's current value */
error_t rt_timer_set(uint_t time);

/** Set Real-Time timer's period */
error_t rt_timer_setperiod(uint_t period);

/** Send commands to Real-Time timer device */
error_t rt_timer_ctrl(struct rt_params_s *param);

#endif	/* _RT_TIMER_H_ */
