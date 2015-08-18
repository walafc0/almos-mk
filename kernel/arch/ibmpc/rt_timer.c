/*
 * rt_timer.c - real-time timer related operations
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
#include <rt_timer.h>
#include <cpu.h>
#include <device.h>
#include <driver.h>
#include <sysio.h>
#include <thread.h>
#include <kmem.h>

extern struct device_s rt_timer;

static void timer_irq_handler (struct irq_action_s *action)
{
	register struct cpu_s *cpu;
	register struct thread_s *this;
   
	this = current_thread();
	cpu = this->local_cpu;

	if(this->type == PTHREAD)
	{
		MEM_CHECK(this->info.usr_stack_base);
	}
   
	MEM_CHECK(this->info.sys_stack_base);
   
	cpu->alarm_mgr.action.irq_handler(&cpu->alarm_mgr.action);

	cpu->tics_nr ++;
	cpu->tics_count ++;
	sched_clock(this);
}

/** Initialize Real-Time timer source */
error_t rt_timer_init(uint_t period, uint_t withIrq)
{
	rt_timer.irq = 0;
	rt_timer.action.irq_handler = &timer_irq_handler;
	rt_timer.type = DEV_TIMER;

	strcpy(rt_timer.name, "TIMER");
	metafs_init(&rt_timer.node, rt_timer.name);

	return 0;
}

/** Read Real-Time timer current value  */
error_t rt_timer_read(uint_t *time)
{
	*time = current_thread()->local_cpu->tics_nr * 20;
	return 0;
}

/** Set Real-Time timer's current value */
error_t rt_timer_set(uint_t time)
{
	return 0;
}

/** Set Real-Time timer's period */
error_t rt_timer_setperiod(uint_t period)
{
	return 0;
}

/** Send commands to Real-Time timer device */
error_t rt_timer_ctrl(struct rt_params_s *param)
{
	switch(param->rt_cmd)
	{
	case RT_TIMER_STOP:
		//rt_timer.op.timer.stop(&rt_timer);
		return 0;

	case RT_TIMER_RUN:
		//rt_timer.op.timer.run(&rt_timer, (uint_t)rt_timer.data);
		return 0;

	default:
		return -1;
	}
}
