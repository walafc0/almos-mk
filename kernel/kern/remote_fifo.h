/*
 * kern/remote_fifo.h - kernel generic SRMW remote FIFO
 *
 * This interface allows to have a FIFO initialized in one of the 
 * following access modes:
 *
 *   - Single-Writer, Single-Reader
 *   - Multiple-Writers, Single-Reader
 *   - Single-Writer, Multiple-Readers
 *   - Multiple-Writers, Multiple-Readers
 *
 * The access to the FIFO is implemented using a lock-free algorithm.
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

#ifndef _REMOTE_FIFO_H_
#define _REMOTE_FIFO_H_

#include <errno.h>
#include <types.h>
#include <mcs_sync.h>
#include <remote_access.h>

/////////////////////////////////////////////////////////////
//                     Public Section                      //
/////////////////////////////////////////////////////////////

struct remote_fifo_s;

/* Kernel generic lock-free remote FIFO type */

/**
 * Get one element from @remote_fifo generic fifo
 *  
 * @param   remote_fifo     : pointer to the buffer.
 * @param   cid		   : cluster that containt the buffer.
 * @param   item	   : where to store gotten item base addr.
 * 
 * @return  0 on success, EAGAIN if the buffer is empty or 
 *          EBUSY if a contention has been detected.
 */
//get an item base adresse (does not advance rdidx) (on local fifos)
//inline 
error_t remote_fifo_get(struct remote_fifo_s *remote_fifo, void **item);

//signal that an item has been handled (advance rdidx) (on local fifos)
//inline 
void remote_fifo_release(struct remote_fifo_s *remote_fifo);

/**
 * Put one element into @remote_fifo generic fifo
 *  
 * @param   remote_fifo     : pointer to the buffer.
 * @param   cid		   : cluster that containt the buffer.
 * @param   item     : the item to be stored.
 * 
 * @return  0 on success, EAGAIN if the buffer is empty or 
 *          EBUSY if a contention has been detected.
 */
//set an item (advance wridx)
//inline 
error_t remote_fifo_put(struct remote_fifo_s *remote_fifo, cid_t cid, void *item);

/**
 * Query if @remote_fifo is empty
 *
 * @param   remote_fifo     : pointer to the buffer.
 * @param   cid		   : cluster that containt the buffer.
 * @return  true if the buffer is empty, false otherwise.
 */
static inline bool_t remote_fifo_isEmpty(struct remote_fifo_s *remote_fifo, cid_t cid);

/**
 * Query if @remote_fifo is full
 *
 * @param   remote_fifo     : pointer to the buffer.
 * @param   cid		   : cluster that containt the buffer.
 * @return  true if the buffer is full, false otherwise.
 */
static inline bool_t remote_fifo_isFull(struct remote_fifo_s *remote_fifo, cid_t cid);

/**
 * Initialize @remote_fifo and set its access mode
 * 
 * @param   remote_fifo     : pointer to the buffer.
 * @param   slot_nbr      : In number of elements.
 * @param   size      : size of  an elements.
 *
 * @return  ENOMEM in case of no memory ressources, 0 otherwise.
 */
error_t remote_fifo_init(struct remote_fifo_s *remote_fifo, size_t slot_nbr, size_t size);

/**
 * Destroy @remote_fifo
 * It will free its all internal resources.
 *
 * @param     remote_fifo    : Buffer descriptor
 */
void remote_fifo_destroy(struct remote_fifo_s *llfb);


/////////////////////////////////////////////////////////////
//                    Private Section                      //
/////////////////////////////////////////////////////////////

struct remote_fifo_s
{
	union 
	{
		volatile uint_t wridx;
		cacheline_t padding1;
	};
	
	union 
	{
		volatile uint_t rdidx;
		cacheline_t padding2;
	};
	
	void*	tbl;
	size_t	slot_nbr;
	size_t	slot_size;
	size_t	item_size;
	struct	mcs_lock_s lock;
};


//FIXME
static inline bool_t remote_fifo_isEmpty(struct remote_fifo_s *remote_fifo, cid_t cid)
{
	return (remote_lw((void*)&remote_fifo->rdidx, cid) == 
		remote_lw((void*)&remote_fifo->wridx, cid)) ? 
		true : false;
}

static inline bool_t remote_fifo_isFull(struct remote_fifo_s *remote_fifo, cid_t cid)
{
	return (remote_lw((void*)&remote_fifo->wridx, cid) == 
		(remote_lw((void*)&remote_fifo->rdidx, cid) +1) % 
		remote_lw((void*)&remote_fifo->slot_nbr, cid)) ? 
		true : false;
}

#if 0//CONFIG_EVENT_FIFO_DEBUG
void remote_fifo_print(struct remote_fifo_s *remote_fifo);
#endif

#endif	/* _EVENT_FIFO_H_ */
