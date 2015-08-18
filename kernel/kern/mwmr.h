/*
 * kern/mwmr.h - a basic FIFO buffer
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

#ifndef _MWMR_H_
#define _MWMR_H_

#include <types.h>

struct fifomwmr_s;

struct fifomwmr_s* mwmr_init(int item, int length, int isAtomic);
int mwmr_read(struct fifomwmr_s *fifo, void *buffer, size_t size);
int mwmr_write(struct fifomwmr_s *fifo, void *buffer, size_t size);
void mwmr_destroy(struct fifomwmr_s *fifo);

#endif	/* _MWMR_H_ */
