/*
 * kern/time.c - thread time related management
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
#include <task.h>
#include <scheduler.h>
#include <time.h>
#include <list.h>
#include <rt_timer.h>
#include <kmagics.h>

error_t alarm_wait(struct alarm_info_s *info, uint_t msec)
{
	register uint_t tm_now;
	struct cpu_s *cpu;
	struct alarm_s *alarm_mgr;
	struct list_entry *iter;
	struct alarm_info_s *current;
	uint_t irq_state;

	cpu       = current_cpu;
	alarm_mgr = &cpu->alarm_mgr;

	cpu_disable_all_irq(&irq_state);

	tm_now          = cpu_get_ticks(cpu) * MSEC_PER_TICK;
	info->signature = ALRM_INFO_ID;
	info->tm_wakeup = tm_now + msec;

	if(list_empty(&alarm_mgr->wait_queue))
		list_add_first(&alarm_mgr->wait_queue, &info->list);
	else
	{
		list_foreach_forward(&alarm_mgr->wait_queue, iter)
		{
			current = list_element(iter, struct alarm_info_s, list);
      
			if(info->tm_wakeup <= current->tm_wakeup) 
			{
				list_add_pred(iter, &info->list);
				goto NEXT_TRAITMENT;
			}
		}
		list_add_last(&alarm_mgr->wait_queue, &info->list);
	}

NEXT_TRAITMENT:
	cpu_restore_irq(irq_state);
	return 0;
}

void alarm_clock(struct alarm_s *alarm, uint_t ticks_nr)
{
	register uint_t tm_msec;
	register struct list_entry *iter;
	register struct alarm_info_s *info;
  
	tm_msec = ticks_nr * MSEC_PER_TICK;

	list_foreach_forward(&alarm->wait_queue, iter)
	{
		info = list_element(iter, struct alarm_info_s, list);

		assert((info->signature == ALRM_INFO_ID) && "Not an ALRM info object");

		if(tm_msec < info->tm_wakeup)
			break;
    
		list_unlink(iter);
		event_send(info->event, current_cpu->gid);
	}
}

error_t alarm_manager_init(struct alarm_s *alarm)
{
	list_root_init(&alarm->wait_queue);
	return 0;
}


#if CONFIG_THREAD_TIME_STAT

inline void tm_sleep_compute(struct thread_s *thread)
{
	uint_t tm_now;

	if(thread->type == TH_IDLE)
		return;

	rt_timer_read(&tm_now);
	thread->info.tm_sleep += (tm_now - thread->info.tm_tmp);
	thread->info.tm_tmp    = tm_now;
}

inline void tm_usr_compute(struct thread_s *thread)
{ 
	uint_t tm_now;

	rt_timer_read(&tm_now);
	thread->info.tm_usr += (tm_now - thread->info.tm_tmp);
	thread->info.tm_tmp  = tm_now;
}

inline void tm_sys_compute(struct thread_s *thread)
{ 
	uint_t tm_now;

	rt_timer_read(&tm_now);
	thread->info.tm_sys += (tm_now - thread->info.tm_tmp);
	thread->info.tm_tmp  = tm_now;
}

inline void tm_wait_compute(struct thread_s *thread)
{
	uint_t tm_now;

	if(thread->type == TH_IDLE)
		return;

	rt_timer_read(&tm_now);
	thread->info.tm_wait += (tm_now - thread->info.tm_tmp);
	thread->info.tm_tmp   = tm_now;
	thread->info.tm_exec  = tm_now;
}

inline void tm_exit_compute(struct thread_s *thread)
{
	uint_t tm_now;
  
	rt_timer_read(&tm_now);
	thread->info.tm_sys += tm_now - thread->info.tm_tmp;
	thread->info.tm_dead = tm_now;
}

inline void tm_born_compute(struct thread_s *thread)
{
	uint_t tm_now;
  
	rt_timer_read(&tm_now);
	thread->info.tm_born = tm_now;
	thread->info.tm_tmp  = tm_now;
	thread->info.tm_exec = tm_now;
}

inline void tm_create_compute(struct thread_s *thread)
{
	rt_timer_read(&thread->info.tm_create);
}


#endif	/* CONFIG_THREAD_TIME_STAT */
