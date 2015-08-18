/*
 * time.c: thread time related management
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

#ifndef _TIME_H_
#define _TIME_H_

#include <types.h>
#include <list.h>
#include <device.h>

struct event_s;

struct alarm_info_s 
{
	uint_t signature;

	/* Public members */
	struct event_s *event;

	/* Private members */
	uint_t tm_wakeup;
	struct list_entry list;
};

struct alarm_s
{ 
	struct list_entry wait_queue;
};


struct timeb {
	time_t         time;
	unsigned short millitm;
	short          timezone;
	short          dstflag;
};


error_t alarm_manager_init(struct alarm_s *alarm);
error_t alarm_wait(struct alarm_info_s *info, uint_t msec);

void alarm_clock(struct alarm_s *alarm, uint_t ticks_nr);

int sys_clock (uint64_t *val);
int sys_alarm (unsigned nb_sec);
int sys_ftime(struct timeb *utime);

#if CONFIG_THREAD_TIME_STAT
struct thread_s;
inline void tm_sleep_compute(struct thread_s *thread);
inline void tm_usr_compute(struct thread_s *thread);
inline void tm_sys_compute(struct thread_s *thread);
inline void tm_wait_compute(struct thread_s *thread);
inline void tm_exit_compute(struct thread_s *thread);
inline void tm_born_compute(struct thread_s *thread);
inline void tm_create_compute(struct thread_s *thread);

#else

#define tm_sleep_compute(thread)
#define tm_usr_compute(thread)
#define tm_sys_compute(thread)
#define tm_wait_compute(thread)
#define tm_exit_compute(thread)
#define tm_born_compute(thread)
#define tm_create_compute(thread)

#endif	/* CONFIG_SCHED_STAT */

#endif	/* _TIME_H_ */
