/*
 * kern/dma.h - DMA related access
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

#ifndef _DMA_H_
#define _DMA_H_

#include <types.h>
#include <kmem.h>

#define DMA_SYNC    0
#define DMA_ASYNC   1

int sys_dma_memcpy (void *src, void *dst, size_t size);
error_t dma_memcpy (void *dst, void *src, size_t size, uint_t isAsync);

KMEM_OBJATTR_INIT(dma_kmem_request_init);

#endif	/* _DMA_H_ */
