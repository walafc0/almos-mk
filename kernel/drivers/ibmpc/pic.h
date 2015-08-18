/*
 * pic.h - Programmable Interrupt Controller driver interface
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

#ifndef _PIC_ICU_H_
#define _PIC_ICU_H_

#define PIC_IRQ_MAX          64    // IRQ input lines (see __kentry.S)

/* PIC mapped registers offset */
#define PIC_CMD_REG          0	        /* Command Register */
#define PIC_STATUS_REG       0		/* Status Register  */
#define PIC_IMR_REG	     1          /* Interrupt Mask Register */
#define PIC_DATA_REG         1		/* Data Register */

struct irq_action_s;
struct device_s;
extern struct irq_action_s *irq_vector[];

void ibmpc_pic_init(struct device_s *pic, void *base, uint_t irq);
void ibmpc_pic_bind(struct device_s *pic, struct device_s *dev);

#endif	/* _PIC_ICU_H_ */
