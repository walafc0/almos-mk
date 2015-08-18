/*
 * mm/ppn.h - physical page number operations
 *
 * Copyright (c) 2015 UPMC Sorbonne Universites
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


#include <atomic.h>
#include <spinlock.h>
#include <types.h>
#include <list.h>
#include <wait_queue.h>
#include <ppn.h>
#include <pmm.h>
#include <page.h>
#include <thread.h>
#include <cluster.h>


cid_t ppn_ppn2cid(ppn_t ppn)
{
	return pmm_ppn2cid(ppn);
}

bool_t ppn_is_local(ppn_t ppn)
{
	return pmm_ppn2cid(ppn) == current_cid;
}

vma_t ppn_ppn2vma(ppn_t ppn)
{
	//assume that this func work with remote ppn
	return (vma_t)pmm_ppn2vma(ppn);
}

void ppn_copy(ppn_t dest, ppn_t src)
{
	void* dest_addr;
	void* src_addr;
	cid_t dest_cid;
	cid_t src_cid;

	dest_cid = pmm_ppn2cid(dest);
	src_cid = pmm_ppn2cid(src);

	//
	dest_addr = (void*)pmm_ppn2vma(dest);
	src_addr = (void*)pmm_ppn2vma(src);

	remote_memcpy(dest_addr, dest_cid, src_addr, src_cid,  PMM_PAGE_SIZE);
}

RPC_DECLARE(__ppn_refcount_down, 
		RPC_RET(RPC_RET_PTR(error_t, err)),
		RPC_ARG(RPC_ARG_VAL(ppn_t, ppn)))
{
	struct page_s *page;
	page = ppm_ppn2page(&current_cluster->ppm, ppn);

	//decrement and, if nessary, free the page
	ppm_free_pages(page);

	*err = 0;
}

void ppn_refcount_down(ppn_t ppn)
{
	error_t err;

	RCPC(ppn_ppn2cid(ppn), RPC_PRIO_PPM, 
		__ppn_refcount_down,
		RPC_RECV(RPC_RECV_OBJ(err)),
		RPC_SEND(RPC_SEND_OBJ(ppn)));
}

RPC_DECLARE(__ppn_refcount_up, 
		RPC_RET(RPC_RET_PTR(error_t, err)),
		RPC_ARG(RPC_ARG_VAL(ppn_t, ppn)))
{
	struct page_s *page;
	page = ppm_ppn2page(&current_cluster->ppm, ppn);

	page_refcount_up(page);

	*err = 0;
}

void ppn_refcount_up(ppn_t ppn)
{
	error_t err;

	RCPC(ppn_ppn2cid(ppn), RPC_PRIO_PPM, 
		__ppn_refcount_up,
		RPC_RECV(RPC_RECV_OBJ(err)),
		RPC_SEND(RPC_SEND_OBJ(ppn)));
}

