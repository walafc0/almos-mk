/*
 * tty.c - a basic tty driver
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

#ifndef _TTY_H_
#define _TTY_H_

#include <driver.h>
#include <scheduler.h>
#include <rwlock.h>

#define TTY_STATUS_REG  0
#define TTY_READ_REG    1
#define TTY_WRITE_REG   2

struct device_s;
struct fifomwmr_s;

void ibmpc_tty_init(struct device_s *tty, void *base, uint_t irq, uint_t tty_id);

struct thread_s;

struct tty_context_s
{
	uint_t pos_X, pos_Y;
	uint_t attr;
	uint_t id;
	dev_request_t *pending_rq;
	unsigned int eol;		/* End Of Line */
	struct rwlock_s rwlock;
	struct wait_queue_s wait_queue;
	struct fifomwmr_s *in_channel;
	struct fifiomwmr_s *out_channel;
};

extern uint8_t keyboard_map[];
#endif	/* _TTY_H_ */
