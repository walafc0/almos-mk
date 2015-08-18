/*
 * kern/kmagics.h - magic numbers used to identify kernel objects
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

#ifndef _KMAGICS_H_
#define _KMAGICS_H_

#define MEM_CHECK_VAL     0xA5A5A5A5
#define SCHED_CHECK_VAL   0xDEADBEEF
#define LIST_NEXT_DEAD    0x0000b0C1
#define LIST_PRED_DEAD    0x0000c0b1
#define LIST_SNEXT_DEAD   0x0000A0A1
#define POBJECT_CREATED   0x0000A5B5
#define SEMAPHORE_ID      0xA0B1C0B3
#define COND_VAR_ID       0xB1CA0BA5
#define KCOND_VAR_ID      0xB3B0A3A3
#define BARRIER_ID        0xCFA5B1A3
#define RWLOCK_ID         0xD1A5B1EF
#define THREAD_ID         0xDEEFBAAD
#define PPM_ID            0xF0F0A5A5
#define ALRM_INFO_ID      0xAF5FABE5

#endif	/* _KMAGICS_H_ */
