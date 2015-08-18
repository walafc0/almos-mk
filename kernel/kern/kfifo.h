/*
 * kern/kfifo.h - kernel lock-free generic FIFO
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

#ifndef _KFIFO_H_
#define _KFIFO_H_

#include <errno.h>
#include <types.h>

/////////////////////////////////////////////////////////////
//                     Public Section                      //
/////////////////////////////////////////////////////////////

/* Single Writer Single Reader */
#define KFIFO_NA

/* Multiple Writer Single Reader */
#define KFIFO_MW

/* Single Writer Multiple Reader */
#define KFIFO_MR

/* Muliple Writer Multiple Reader */	
#define KFIFO_MWMR

/* Kernel generic lock-free FIFO type */
struct kfifo_s;

/**
 * Get one element from @kfifo generic fifo
 *  
 * @param   kfifo     : pointer to the buffer.
 * @param   value     : where to store gotten value.
 * 
 * @return  0 on success, EAGAIN if the buffer is empty or 
 *          EBUSY if a contention has been detected.
 */
static inline error_t kfifo_get(struct kfifo_s *kfifo, void **value);

/**
 * Put one element into @kfifo generic fifo
 *  
 * @param   kfifo     : pointer to the buffer.
 * @param   value     : the value to be stored.
 * 
 * @return  0 on success, EAGAIN if the buffer is empty or 
 *          EBUSY if a contention has been detected.
 */
static inline error_t kfifo_put(struct kfifo_s *kfifo, void *value);

/**
 * Query if @kfifo is empty
 *
 * @param   kfifo     : pointer to the buffer.
 * @return  true if the buffer is empty, false otherwise.
 */
static inline bool_t kfifo_isEmpty(struct kfifo_s *kfifo);

/**
 * Query if @kfifo is full
 *
 * @param   kfifo     : pointer to the buffer.
 * @return  true if the buffer is full, false otherwise.
 */
static inline bool_t kfifo_isFull(struct kfifo_s *kfifo);

/**
 * Initialize @kfifo and set its access mode
 * 
 * @param   kfifo     : pointer to the buffer.
 * @param   size      : In number of elements.
 * @param   mode      : On of the available aaccess modes (KFIFO_XX).
 *
 * @return  ENOMEM in case of no memory ressources, 0 otherwise.
 */
error_t kfifo_init(struct kfifo_s *kfifo, size_t size, uint_t mode);

/**
 * Destroy @kfifo
 * It will free its all internal resources.
 *
 * @param     kfifo    : Buffer descriptor
 */
void kfifo_destroy(struct kfifo_s *llfb);


/////////////////////////////////////////////////////////////
//                    Private Section                      //
/////////////////////////////////////////////////////////////

/* Single Writer Single Reader */
#undef  KFIFO_NA
#define KFIFO_NA       0x0	

#undef  KFIFO_MW
#define KFIFO_MW       0x1	

#undef  KFIFO_MR
#define KFIFO_MR       0x2

#undef  KFIFO_MWMR
#define KFIFO_MWMR     0x3

#define KFIFO_MASK     0x3

#define KFIFO_PUT(n)  error_t (n) (struct kfifo_s *kfifo, void *val)
#define KFIFO_GET(n)  error_t (n) (struct kfifo_s *kfifo, void **val)

typedef KFIFO_PUT(kfifo_put_t);
typedef KFIFO_GET(kfifo_get_t);

struct kfifo_s
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
  
	struct
	{
		kfifo_put_t *put;
		kfifo_get_t *get;
	}ops;

	size_t  size;
	void    **tbl;
};

static inline error_t kfifo_get(struct kfifo_s *kfifo, void **value)
{
	return kfifo->ops.get(kfifo,value);
}

static inline error_t kfifo_put(struct kfifo_s *kfifo, void *value)
{
	return kfifo->ops.put(kfifo,value);
}

static inline bool_t kfifo_isEmpty(struct kfifo_s *kfifo)
{
	return (kfifo->rdidx == kfifo->wridx) ? true : false;
}

static inline bool_t kfifo_isFull(struct kfifo_s *kfifo)
{
	return (kfifo->wridx == ((kfifo->rdidx + 1) % kfifo->size)) ? true : false;
}

#if CONFIG_KFIFO_DEBUG
void kfifo_print(struct kfifo_s *kfifo);
#endif

#endif	/* _KFIFO_H_ */
