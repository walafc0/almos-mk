/*
 * kern/device.h - kernel definition of a device
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

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <driver.h>
#include <list.h>
#include <metafs.h>
#include <spinlock.h>
#include <kmem.h>

#define DEV_MAX_NAME   12

struct device_s;
struct irq_action_s;

typedef enum
{
	DEV_CHR,
	DEV_BLK,
	DEV_INTERNAL
} dev_type_t;

typedef void (irq_handler_t) (struct irq_action_s *action);

struct irq_action_s
{
	struct device_s *dev;             /*! device identifier */
	irq_handler_t *irq_handler;       /*! pointer to Interrupt Service Routine */
	void *data;                       /*! pointer to an eventual ISR private data */
};

struct device_s
{
	spinlock_t lock;		  /*! lock to guarantee mutual exclusion accesses */
	volatile void *base;              /*! base address of the mapped device */
	dev_type_t type;		  /*! type of device (i.g. DEV_CHR, DEV_BLK ..etc) */
	uint_t cid;			  /*! the cluster containing the device */
	uint_t size;			  /*! size of the mapped device */
	driver_t op;                      /*! set of operations supplied by the device driver */
	struct irq_action_s action;       /*! to manage interrupt generated by the device */
	sint_t irq;                       /*! irq number affected to the device */
	void *data;                       /*! pointer to an eventual driver's private data */
	uint64_t base_paddr;		  /*! physical address where the device is mapped */
	uint64_t mailbox;
	struct device_s *iopic;
	struct list_entry list;
	struct metafs_s node;		  /*! Metafs node associated with this device */
	char name[DEV_MAX_NAME];	  /*! device name */
};

/* TODO: put this devices into a dedicated device tables */
extern struct device_s *__sys_blk;
extern struct device_s *__sys_dma;
extern struct device_s *ttys_tbl[];
/* ----------------------------------------------------- */

struct device_s *device_alloc();

#endif	/* _DEVICE_H_ */