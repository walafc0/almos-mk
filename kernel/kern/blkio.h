/*
 * kern/blkio.c - Per-page buffers I/O interface
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

#ifndef BLKIO_H_
#define BLKIO_H_

#include <list.h>

#define BLKIO_RD        0x01
#define BLKIO_SYNC      0x02

#define BLKIO_INIT      0x01

struct blkio_s;
struct page_s;

struct blkio_s 
{
	uint_t                b_flags;        // BLKIO_INIT

	struct
	{
		spinlock_t		lock;		
		uint16_t            count;	
		uint16_t            cntr;	        
		error_t             error;	
		struct wait_queue_s wait;

	}                     b_ctrl;         // Only used in head-blkio

	dev_request_t		b_dev_rq;	// request for the block device
	struct page_s*	b_page;		// buffer page descriptor
	struct device_s*	b_dev;		// block device
	struct slist_entry	b_list;	        // next buffer in this page
	void*			b_private;	// data for the I/O completion method
};

/**
 * Macro to set a blkio to be skiped while first sync  
 * 
 * @b          pointer to blkio
 */
#define blkio_set_initial(b)			\
	do{(b)->b_flags |= BLKIO_INIT;}while(0)

/**
 * Initializes the kmem cache for the blkio structure.
 */
KMEM_OBJATTR_INIT(blkio_kmem_init);

/**
 * Initializes blkio structures for a buffer page.
 *
 * @dev		block device
 * @page	buffer page to get blkio structures initialized
 * @count	number of blocks in the page
 * @return	error code, 0 if OK
 */
error_t blkio_init(struct device_s *dev, struct page_s *page, uint_t count);

/**
 * Synchronizes all the buffers in a buffer page.
 *
 * @page	buffer page to be synced with the disk
 * @flags	blkio flags
 * @return	error code, 0 if OK
 */
error_t blkio_sync(struct page_s *page, uint_t flags);

/**
 * Destroys the blkio structures for a buffer page.
 *
 * @page	buffer page to have its blkio structures destroyed
 * @return	error code, 0 if OK
 */
error_t blkio_destroy(struct page_s *page);

#endif /* BLKIO_H_ */
