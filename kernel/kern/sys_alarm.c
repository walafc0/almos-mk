/*
 * kern/sys_alarm.c - timed sleep/wakeup
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

#include <types.h>
#include <thread.h>
#include <cluster.h>
#include <task.h>
#include <time.h>
#include <scheduler.h>
#include <cpu.h>
#include <cpu-trace.h>

EVENT_HANDLER(sys_alarm_event_handler)
{
	struct thread_s *thread;

	thread = event_get_argument(event);
	sched_wakeup(thread);
	return 0;
}

int sys_alarm (unsigned nb_sec)
{
	struct thread_s *this;
	struct event_s event;
	struct alarm_info_s info;

	if( nb_sec == 0)
		return 0;

	this = current_thread;
	event_set_handler(&event, &sys_alarm_event_handler);
	event_set_priority(&event, E_FUNC);
	event_set_argument(&event,this);
	info.event = &event;

	alarm_wait(&info, nb_sec * 4);
	sched_sleep(this);
	return 0;
}


