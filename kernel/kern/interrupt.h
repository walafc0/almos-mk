/*
 * kern/interrupt.h - kernel IRQs reservation interface
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

#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <types.h>

#define IRQ_DYN     0x01	/* Dynamic Allocation */
#define IRQ_SHD     0x02	/* Shared IRQ */

/* Struct used from device.h */
struct irq_action_s;

/* Struct used form thread.h */
struct thread_s;

/* IRQ Manager initialize */
error_t irq_manager_init();

/* IRQ Ressources Allocation */
error_t irq_manager_alloc(uint_t irq, uint_t irqs_nr, uint_t flags);

/* IRQ Ressources Liberation */
error_t irq_manager_free(uint_t irq, uint_t irqs_nr);

/* IRQ Handler Registeration */
error_t irq_handler_register(struct irq_action_s *info, uint_t irq, uint_t flags);

/* IRQ Handler Unregisteration */
error_t irq_handler_unregister(uint_t irq);

/* Kernel entry point for interrupt handling */
void do_interrupt(struct thread_s *this, uint_t irq_num);

#endif	/* _INTERRUPT_H_ */
