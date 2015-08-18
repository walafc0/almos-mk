/*
 * kern/ku_transfert.h - Transfering data between user and kernel spaces.
 * 
 * Copyright (c) 2014 UPMC Sorbonne Universites
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

#include <ku_transfert.h>
#include <cluster.h>
#include <thread.h>
#include <task.h>
#include <pmm.h>
#include <ppm.h>

uint_t kk_scpy_to(struct ku_obj *kuo, void *ptr, uint_t size)
{
	memcpy(kuo->buff, ptr, size);
	return size;
}

uint_t kk_scpy_from(struct ku_obj *kuo, void *ptr, uint_t size)
{
	memcpy(ptr, kuo->buff, size);
	return size;
}

uint_t kk_copy_to(struct ku_obj *kuo, void *ptr)
{
	uint_t count;

	count = kuo->get_size(kuo);

	memcpy(kuo->buff, ptr, count);
	return count;
}

uint_t kk_copy_from(struct ku_obj *kuo, void *ptr)
{
	uint_t count;

	count = kuo->get_size(kuo);

	memcpy(ptr, kuo->buff, count);
	return count;
}

uint_t kk_strlen(struct ku_obj *kuo)
{
	return strlen(kuo->buff);
}


//copy to user (from kernl)
uint_t ku_scpy_to(struct ku_obj *kuo, void *ptr, uint_t size)
{
	if(cpu_copy_to_uspace(kuo->buff, ptr, size)) 
		return 0;
	return size;
}

//copy from user (to kernel)
uint_t ku_scpy_from(struct ku_obj *kuo, void *ptr, uint_t size)
{
	if(cpu_copy_from_uspace(ptr, kuo->buff, size)) 
		return 0;
	return size;
}

//copy to user (from kernl)
uint_t ku_copy_to(struct ku_obj *kuo, void *ptr)
{
	uint_t size;
	size = kuo->get_size(kuo);
	if(cpu_copy_to_uspace(kuo->buff, ptr, size))
		return 0;
	return size;
}

//copy from user (to kernel)
uint_t ku_copy_from(struct ku_obj *kuo, void *ptr)
{
	uint_t size;
	size = kuo->get_size(kuo);

	if(cpu_copy_from_uspace(ptr, kuo->buff, size))
		return 0;
	return size;
}


uint_t ku_strlen(struct ku_obj *kuo)
{
	uint_t count;
	if(cpu_uspace_strlen(kuo->buff, &count))
		return 0;
	return count;
}

uint_t default_gsize(struct ku_obj *kuo)
{
	return kuo->size;
}

//return the number of ppns compsing the buffer
uint_t kk_get_ppn_max(struct ku_obj *kuo)
{
	size_t size;

	size = kuo->get_size(kuo)+PMM_PAGE_SIZE*2;
	return size/PMM_PAGE_SIZE;
}

uint_t ku_get_ppn_max(struct ku_obj *kuo)
{
	return kk_get_ppn_max(kuo);
}

typedef ppn_t __vma2ppn(vma_t addr);


uint_t __get_ppn(struct ku_obj *kuo, ppn_t *ppns, size_t nb_ppn, __vma2ppn v2p)
{
	uint_t offset;
	uint_t size;
	vma_t addr;
	int i;

	addr = (vma_t)kuo->buff;
	size = kuo->get_size(kuo);

	if(size==0)
	{ 
		ppns[0]=0; return 0;
	}

	offset = addr & PMM_PAGE_MASK;
	addr &= ~PMM_PAGE_MASK;
	size -= MIN(size, offset); 
	for(i = 0; i < nb_ppn; i++)
	{
		ppns[i] = v2p(addr);
		if(size < PMM_PAGE_SIZE) break;
		addr += PMM_PAGE_SIZE; 
		size -= PMM_PAGE_SIZE;
	}

	if(i < nb_ppn) ppns[++i] = 0;

	return offset;
}

ppn_t kk_vma2ppn(vma_t addr)
{
	return ppm_vma2ppn(&current_cluster->ppm, (void*)addr);
}

uint_t kk_get_ppn(struct ku_obj *kuo, ppn_t *ppns, size_t nb_ppn)
{
	return __get_ppn(kuo, ppns, nb_ppn, kk_vma2ppn);
}

ppn_t ku_vma2ppn(vma_t addr)
{
	ppn_t ppn;
	ppn = task_vaddr2ppn(current_task, (void*)addr);
	if(!ppn)
	{
		//TODO
		printk(ERROR, "TODO\n");
		while(1);
	}
	return ppn;
}

uint_t ku_get_ppn(struct ku_obj *kuo, ppn_t *ppns, size_t nb_ppn)
{
	return __get_ppn(kuo, ppns, nb_ppn, ku_vma2ppn);
}
