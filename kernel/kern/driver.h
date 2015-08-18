/*
 * kern/driver.h - kernel definition of a device driver
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

#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <types.h>
#include <list.h>
#include <event.h>

struct device_s;
struct vfs_file_s;
struct vm_region_s;
struct irq_action_s;

typedef struct dev_params_s
{
	uint_t type;
	size_t size;
	uint_t xSize;
	uint_t ySize;
	uint_t count;
	uint_t speed;
	uint_t sector_size;
}dev_params_t;

#define DEV_RQ_NOBLOCK   0x01
#define DEV_RQ_KERNEL    0x02
#define DEV_RQ_NESTED    0x04

typedef struct dev_request_s
{
	/* Request information provieded by caller */
	void *src;			
	void *dst;			
	uint32_t count;		
	uint32_t flags;		
	union{
		struct vfs_file_s *file;
		struct vfs_file_remote_s *fremote;
	};
	struct vm_region_s *region;
	struct event_s event;

	/* Common driver specific information */
	error_t err;			
	void *data;
	struct list_entry list;
}dev_request_t;


typedef sint_t (device_params_t)(struct device_s *dev, dev_params_t *params);
typedef sint_t (device_request_t)(struct device_s *dev, dev_request_t *rq);

/* Definition of driver operations for a typical device */
struct dev_op
{
	device_request_t *open;
	device_request_t *read;
	device_request_t *write;
	device_request_t *close;
	device_request_t *lseek;
	device_request_t *mmap;
	device_request_t *munmap;
	device_params_t  *set_params;
	device_params_t  *get_params;
};

typedef error_t (device_init_t)(struct device_s *dev);
typedef error_t (device_action_t)(struct device_s *dev, uint_t flags);

/* Definition of driver operations for a typical Timer device type */
typedef void (timer_run_t)(struct device_s *timer, bool_t withIrq);
typedef void (timer_stop_t)(struct device_s *timer);
typedef uint_t (timer_get_value_t)(struct device_s *timer);
typedef uint_t (timer_set_value_t)(struct device_s *timer, uint_t value);
typedef void (timer_set_period_t)(struct device_s *timer, uint_t period);

struct dev_timer_op
{
	timer_run_t *run;
	timer_stop_t *stop;
	timer_get_value_t *get_value;
	timer_set_value_t *set_value;
	timer_set_period_t *set_period;
};

/* Definition of driver operations for a typical ICU device type */
typedef void   (icu_set_mask_t)(struct device_s *icu, uint_t mask, uint_t type, uint_t output_irq);
typedef uint_t (icu_get_mask_t)(struct device_s *icu, uint_t type, uint_t output_irq);
typedef sint_t (icu_get_highest_irq_t)(struct device_s *icu, uint_t type, uint_t output_irq);
typedef error_t (icu_bind_t)(struct device_s *icu, struct device_s *dev);
typedef error_t (icu_bind_wti_t)(struct device_s *icu, struct irq_action_s *dev, uint_t wti_index);

struct dev_icu_op
{
#if CONFIG_XICU_USR_ACCESS
	device_request_t *open;
	device_request_t *read;
	device_request_t *write;
	device_request_t *close;
	device_request_t *lseek;
	device_request_t *mmap;
	device_request_t *munmap;
	device_params_t  *set_params;
	device_params_t  *get_params;
#endif
	icu_set_mask_t *set_mask;
	icu_get_mask_t *get_mask;
	icu_get_highest_irq_t *get_highest_irq;
	icu_bind_wti_t *bind_wti;
	icu_bind_t *bind;
};

//FG
/* Definition of driver operations for a typical IOPIC device type */
typedef void (iopic_bind_wti_and_irq_t)(struct device_s *iopic, struct device_s *dev);
typedef void (iopic_get_status_t)(struct device_s *iopic, uint_t wti_channel, uint_t * status);

struct dev_iopic_op
{
    iopic_get_status_t *get_status;
    iopic_bind_wti_and_irq_t *bind_wti_and_irq;
};

/* Definition of a generic device driver */
struct driver_s
{
	device_init_t   *init;		        /* Mandatory Function */
	device_action_t *suspend;	        /* lowpower, sleep */
	device_action_t *resume;
	device_action_t *destroy;
	union 
	{
		struct dev_op       dev;
		struct dev_icu_op   icu;
		struct dev_timer_op timer;
		struct dev_iopic_op iopic;//FG
	};
	uint_t drvid;			        /* Driver ID */
};

typedef struct driver_s driver_t;

#endif	/* _DRIVER_H_ */
