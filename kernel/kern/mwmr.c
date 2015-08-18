/*
 * kern/mwmr.c - a basic FIFO buffer
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

#include <errno.h>
#include <mwmr.h>
#include <kmem.h>
#include <thread.h>
#include <spinlock.h>

struct fifomwmr_s
{
	spinlock_t lock;
	uint_t isAtomic;
	uint_t status;
	uint_t read_ptr;
	uint_t write_ptr;
	char *data;
	uint_t item;
	uint_t length;
};

struct fifomwmr_s* mwmr_init(int item, int length, int isAtomic)
{
	kmem_req_t req;
	struct fifomwmr_s *fifo;
	char *data;

	req.type  = KMEM_GENERIC;
	req.size  = sizeof(*fifo);
	req.flags = AF_BOOT | AF_ZERO;
  
	if((fifo = kmem_alloc(&req)) == NULL)
		return NULL;
  
	req.size = sizeof (unsigned int) * item * length;

	if((data = kmem_alloc(&req)) == NULL)
	{
		printk(WARNING, "WARNING: mwmr_init: MEMORY SHORTAGE\n");
		req.ptr = fifo;
		kmem_free(&req);
		return NULL;
	}

	spinlock_init(&fifo->lock, "MWMR");
	fifo->isAtomic = isAtomic;
	fifo->item     = item;
	fifo->length   = length * item;
	fifo->data     = data;
  
	return fifo;
}

int mwmr_read(struct fifomwmr_s *fifo, void *buffer, size_t size)
{
	register int rnum;                    // number of words to read
	register char *src;
	register int i, size1, size2;
	register char *buff = (char *) buffer;

	if (size == 0)
		return 0;

	if(fifo->isAtomic)
		spinlock_lock(&fifo->lock);

	rnum = (size > fifo->status) ? fifo->status : size;

	if (rnum)
	{
		src = fifo->data + fifo->read_ptr;
		if ((fifo->read_ptr + rnum) <= fifo->length)
			for (i = 0; i < rnum; i++)
				buff[i] = src[i];
		else
		{
			size1 = fifo->length - fifo->read_ptr;
			size2 = rnum - size1;

			for (i = 0; i < size1; i++)
				buff[i] = src[i];

			for (i = 0; i < size2; i++)
				buff[size1 + i] = *(fifo->data + i);
		}

		fifo->status  -= rnum;
		fifo->read_ptr = (fifo->read_ptr + rnum) % fifo->length;
	}

	if(fifo->isAtomic)
		spinlock_unlock(&fifo->lock);

	return rnum;
}

int mwmr_write(struct fifomwmr_s *fifo, void *buffer, size_t size)
{
	register int free_space;
	register int wnum;                    // number of words to write
	register char *dest;
	register int i, size1, size2;
	register char *buff = (char *) buffer;

	if (size == 0)
		return 0;

	if(fifo->isAtomic)
		spinlock_lock (&fifo->lock);

	free_space = fifo->length - fifo->status;
	wnum       = (size > free_space) ? free_space : size;

	if (wnum)
	{
		dest = (fifo->data + fifo->write_ptr);

		if ((fifo->write_ptr + wnum) <= fifo->length)
			for (i = 0; i < wnum; i++)
				dest[i] = buff[i];
		else
		{
			size1 = fifo->length - fifo->write_ptr;
			size2 = wnum - size1;

			for (i = 0; i < size1; i++)
				dest[i] = buff[i];

			for (i = 0; i < size2; i++)
				*(fifo->data + i) = buff[size1 + i];
		}

		fifo->status += wnum;
		fifo->write_ptr = (fifo->write_ptr + wnum) % fifo->length;
	}

	if(fifo->isAtomic)
		spinlock_unlock(&fifo->lock);

	return wnum;
}

void mwmr_destroy(struct fifomwmr_s *fifo)
{
	kmem_req_t req;
  
	if(fifo == NULL) return;

	req.type = KMEM_GENERIC;
	req.ptr  = fifo->data;
	kmem_free(&req);
  
	req.ptr  = fifo;
	kmem_free(&req);
}
