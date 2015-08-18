/*
 * soclib_timer.c - soclib timer driver
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

#include <system.h>
#include <cpu.h>
#include <device.h>
#include <driver.h>
#include <thread.h>
#include <kmem.h>
#include <scheduler.h>
#include <list.h>
#include <soclib_timer.h>
#include <drvdb.h>
#include <string.h>
#include <cpu-trace.h>

#define TIMER_RESET_IRQ    0x00
#define TIMER_STOP         0x00
#define TIMER_RUNNING      0x01
#define TIMER_IRQ_ENABLED  0x02

/* TIMER mapped registers offset */
#define TIMER_VALUE_REG      0
#define TIMER_MODE_REG       1
#define TIMER_PERIOD_REG     2
#define TIMER_RESET_IRQ_REG  3

static uint_t timer_get_value(struct device_s *timer)
{
	volatile uint32_t *base = timer->base;

	return base[TIMER_VALUE_REG];
}

static void timer_run(struct device_s *timer, bool_t withIrq)
{
	volatile uint32_t *base = timer->base;
  
	base[TIMER_MODE_REG] = (withIrq) ? TIMER_RUNNING | TIMER_IRQ_ENABLED : TIMER_RUNNING;
}

static void timer_stop(struct device_s *timer)
{
	volatile uint32_t *base = timer->base;
  
	base[TIMER_MODE_REG] = TIMER_STOP;
}

static void timer_set_period(struct device_s *timer, uint_t period)
{
	volatile uint32_t *base = timer->base;
  
	base[TIMER_PERIOD_REG] = period;
}

static void timer_reset_irq(struct device_s *timer)
{
	volatile uint32_t *base = timer->base;
  
	base[TIMER_RESET_IRQ_REG] = TIMER_RESET_IRQ;
}

void timer_irq_handler (struct irq_action_s *action)
{
	register struct cpu_s *cpu;
	register struct device_s *timer;
   
	cpu = current_cpu;

	cpu_trace_write(cpu, timer_irq_handler);
   
	cpu_clock(cpu);
   
	timer = action->dev;
	timer_reset_irq(timer);
	timer_set_period(timer, CPU_CLOCK_TICK);
	timer_run(timer, 1);
}

static uint_t timer_count = 0;

error_t soclib_timer_init(struct device_s *timer)
{
	timer->type = DEV_INTERNAL;

	timer->action.dev         = timer;
	timer->action.irq_handler = &timer_irq_handler;
	timer->action.data        = NULL;
  
	sprintk(timer->name, "timer%d", timer_count++);
	metafs_init(&timer->node, timer->name);
  
	timer->op.timer.run        = &timer_run;
	timer->op.timer.stop       = &timer_stop;
	timer->op.timer.set_period = &timer_set_period;
	timer->op.timer.get_value  = &timer_get_value;
	timer->op.drvid            = SOCLIB_TIMER_ID;
	
	timer->data = NULL;
	return 0;
}

driver_t soclib_timer_driver = { .init = &soclib_timer_init };
