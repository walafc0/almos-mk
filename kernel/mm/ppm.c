/*
 * mm/ppm.c - Per-cluster Physical Pages Manager
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

#include <types.h>
#include <list.h>
#include <bits.h>
#include <pmm.h>
#include <page.h>
#include <spinlock.h>
#include <boot-info.h>
#include <ppm.h>
#include <thread.h>
#include <cluster.h>
#include <kdmsg.h>
#include <kmem.h>
#include <task.h>
#include <dqdt.h>
#include <kmagics.h>

static void ppm_init_pages(struct ppm_s *ppm, uint_t cid);

static error_t ppm_init_finalize(struct ppm_s *ppm, struct boot_info_s *info);

static void ppm_free_pages_nolock(struct ppm_s *ppm, struct page_s *page, uint_t order, uint_t index);

static struct page_s* ppm_do_alloc_pages(struct ppm_s *ppm, uint_t order, uint_t flags);

inline void* ppm_page2addr(struct page_s *page)
{
	register struct ppm_s *ppm;
	ppm = page_get_ppm(page);
	return (void*)((page - ppm->pages_tbl) * PMM_PAGE_SIZE + ppm->vaddr_begin);
}

inline struct page_s* ppm_addr2page(struct ppm_s *ppm, void *addr) 
{
	return (((uint_t)addr - ppm->vaddr_begin) >> PMM_PAGE_SHIFT) + ppm->pages_tbl;
}

inline ppn_t ppm_page2ppn(struct page_s *page)
{
	register struct ppm_s *ppm;
	ppm = page_get_ppm(page);
	return (ppn_t) ((page - ppm->pages_tbl) + ppm->ppn_start);
}

inline struct page_s* ppm_ppn2page(struct ppm_s *ppm, ppn_t ppn)
{
	assert(current_cid == pmm_ppn2cid(ppn));
	return &ppm->pages_tbl[ppn - ppm->ppn_start];
}

inline void* ppm_ppn2vma(struct ppm_s *ppm, ppn_t ppn)
{
	assert(current_cid == pmm_ppn2cid(ppn));
	return (void*)(((ppn - ppm->ppn_start) << PMM_PAGE_SHIFT) + ppm->vaddr_begin);
}

inline ppn_t ppm_vma2ppn(struct ppm_s *ppm, void* vma)
{
	return (((uint_t)vma - ppm->vaddr_begin) >> PMM_PAGE_SHIFT) + ppm->ppn_start;
}

//FIXME: using uint64_t  is a good solution ?
//at least set a type for physical addresses: maybe paddr_t ?
error_t ppm_init(struct ppm_s *ppm, 
		 uint64_t begin, 
		 uint64_t limit, 
		 uint_t vaddr_begin,
		 uint_t vaddr_start,
		 struct boot_info_s *info)
{
	register uint_t i;
	register error_t err;

	ppm->signature = PPM_ID;
	spinlock_init(&ppm->lock, "PPM");
	//FIXME: check that the ppn fit in a uint_t
	ppm->ppn_start = (uint_t) (begin >> PMM_PAGE_SHIFT);
	ppm->vaddr_begin = vaddr_begin;
	ppm->vaddr_start = vaddr_start;
  
	for(i=0; i < PPM_MAX_ORDER; i++)
	{
		list_root_init(&ppm->free_pages[i].root);
		ppm->free_pages[i].pages_nr = 0;
	}
  
	//FIXME: check that the value fit in a uint_t
	ppm->pages_nr = (uint_t) (limit - begin) >> PMM_PAGE_SHIFT;
	ppm->free_pages_nr = 0;
	ppm->begin = begin;
  
	spinlock_init(&ppm->wait_lock, "PPM PGWAIT");

	for(i=0; i < PPM_MAX_WAIT; i++)
		wait_queue_init(&ppm->wait_tbl[i], "PPM WAIT TBL");

	err = ppm_init_finalize(ppm, info);

	if(err != 0) return err;

	ppm->uprio_pages_min  = (ppm->free_pages_nr * (CONFIG_PPM_UPRIO_PGMIN)) / 100;
	ppm->kprio_pages_min  = (ppm->free_pages_nr * (CONFIG_PPM_KPRIO_PGMIN)) / 100;
	ppm->urgent_pages_min = (ppm->free_pages_nr * (CONFIG_PPM_URGENT_PGMIN)) / 100;

#if CONFIG_SHOW_ALL_BOOT_MSG
	if(info->local_cluster_id == info->boot_cluster_id)
	{
		boot_dmsg("PPM %d [ %d, %d, %d, %d, %d ]\n", 
			  info->local_cluster_id,
			  ppm->pages_nr,
			  ppm->free_pages_nr,
			  ppm->uprio_pages_min,
			  ppm->kprio_pages_min,
			  ppm->urgent_pages_min);
    
		boot_dmsg("PPM %d pages_tbl [", info->local_cluster_id);

		for(i=0; i < PPM_MAX_ORDER; i++)
			boot_dmsg("%d, ", ppm->free_pages[i].pages_nr);
    
		boot_dmsg("\b\b]\n");

		ppm_print(ppm);
	}
#endif
	return 0;
}

struct page_s* ppm_alloc_pages(struct ppm_s *ppm, uint_t order, uint_t flags)
{
	register struct ppm_s *current_ppm;
	register struct page_s *ptr;
	register uint_t cntr;
	register uint_t threshold;
	register error_t err;
	register uint_t cid;
	register bool_t isInterleave;
	register bool_t isSeqNextCID;
	register bool_t isUseUPRIO;
	struct thread_s *this;
	uint_t onln_clstrs;

	ptr  = NULL;
	cid  = ppm_get_cluster(ppm)->id;
	cntr = AF_GETTTL(flags);
	this = current_thread;
	current_ppm = ppm;
	onln_clstrs = arch_onln_cluster_nr();

	if(flags & AF_AFFINITY)
	{
		isInterleave = (this->info.attr.flags & PT_ATTR_INTERLEAVE_ALL);
		isSeqNextCID = (this->info.attr.flags & PT_ATTR_MEM_CID_RR);
		isUseUPRIO   = (this->info.attr.flags & PT_ATTR_MEM_PRIO);

		if(!isInterleave && (this->info.attr.flags & PT_ATTR_INTERLEAVE_SEQ))
			isInterleave = true;

		if(isUseUPRIO)
			cntr = AF_GETTTL(AF_TTL_HIGH);
	}
	else
	{
		isInterleave = false;
		isSeqNextCID = false;
		isUseUPRIO   = false;
	}

	cntr = (cntr == 0) ? 1 : cntr;

	while(cntr != 0)
	{
		cntr = cntr - 1;

		if(isUseUPRIO)
		{
			/* Assuming the same value of pages_min on all nodes */
			threshold = ppm->uprio_pages_min;
			goto do_alloc;
		}

		if(flags & AF_USR)
		{
			threshold = ppm->kprio_pages_min;

			if(isInterleave)
			{
				cid = this->info.ppm_last_cid % onln_clstrs;
				this->info.ppm_last_cid ++;
				//FIXME
				//current_ppm = &clusters_tbl[cid].cluster->ppm;
				current_ppm = &current_cluster->ppm;
			}

			goto do_alloc;
		}

		if(flags & AF_PRIO)
		{
			threshold = ppm->urgent_pages_min;
			goto do_alloc;
		}

		threshold = 0;

	do_alloc:

		if(current_ppm->free_pages_nr > threshold)
			ptr = ppm_do_alloc_pages(current_ppm, order, flags);

		if(ptr != NULL) return ptr;

		if((cntr == 0) && isUseUPRIO)
		{
			cntr        = AF_TTL_LOW;
			cid         = ppm_get_cluster(ppm)->id;
			current_ppm = ppm;
			isUseUPRIO  = false;
		}
		else if(isSeqNextCID)
		{
			cid         = (cid + 1) % onln_clstrs;
			//FIXME
			//current_ppm = &clusters_tbl[cid].cluster->ppm;
			current_ppm = &current_cluster->ppm;
		}
		else
		{
			struct dqdt_attr_s attr;
			struct ppm_dqdt_req_s req;

			req.threshold = threshold + (1 << order) + threshold/100;
			req.order     = order;
      
			dqdt_attr_init(&attr, &req);
      
			err = dqdt_mem_request(ppm_get_cluster(current_ppm)->levels_tbl[0], &attr);

			if(err == 0)
			{
				//FIXME
				//current_ppm = &attr.cluster->ppm;
				current_ppm = &current_cluster->ppm;
				cid         = attr.cid;
			}
			else
			{
				isUseUPRIO  = false;
				cid         = (cid + 1) % onln_clstrs;
				//FIXME
				//current_ppm = &clusters_tbl[cid].cluster->ppm;
				current_ppm = &current_cluster->ppm;
			}
		}

#if CONFIG_SHOW_PPM_PGALLOC_MSG
		printk(INFO, "INFO: PPM %d [ %d, %d, %d, %d, %d ]\n", 
		       cid,
		       ppm->pages_nr,
		       ppm->free_pages_nr,
		       ppm->uprio_pages_min,
		       ppm->kprio_pages_min,
		       ppm->urgent_pages_min);
#endif
	}

	return ptr;
}

void ppm_free_pages(struct page_s *page)
{
	register struct ppm_s *ppm;
	register uint_t order;
	register uint_t index;
	uint_t irq_state;

	if((order = page_refcount_down(page)) > 1)
		return;

	ppm   = page_get_ppm(page);
	order = page->order;
	index = page - ppm->pages_tbl;
  
	spinlock_lock_noirq(&ppm->lock, &irq_state);
	ppm_free_pages_nolock(ppm, page, order, index);
	spinlock_unlock_noirq(&ppm->lock, irq_state);
}

static void ppm_init_pages(struct ppm_s *ppm, uint_t cid)
{
	register struct page_s *pages_tbl;
	register uint_t reserved_pages_nr;
	register uint_t i;

	pages_tbl      = (struct page_s *) ppm->vaddr_start;
	ppm->pages_tbl = pages_tbl;

	reserved_pages_nr  = (ARROUND_UP(ppm->pages_nr * sizeof(struct page_s), PMM_PAGE_SIZE)) >> PMM_PAGE_SHIFT;
	reserved_pages_nr += (ppm->vaddr_start - ppm->vaddr_begin) >> PMM_PAGE_SHIFT;

	for(i=0; i < ppm->pages_nr; i++)
		page_init(&pages_tbl[i], cid);

	for(i=0; i < reserved_pages_nr; i++)
		page_state_set(&pages_tbl[i], PGRESERVED);
}


static error_t ppm_init_finalize(struct ppm_s *ppm, struct boot_info_s *info)
{
	register struct page_s *pages_tbl;
	register struct page_s *page;
	register uint_t cid;
	register uint_t i;
	register uint_t reserved;
  
	cid = info->local_cluster_id;

	ppm_init_pages(ppm, cid);

	for(reserved = info->reserved_start; reserved < info->reserved_end; reserved += PMM_PAGE_SIZE)
	{
		if(cid == info->boot_cluster_id)//MLK: Does the boot cluster differt from others ?
			page = ppm_ppn2page(ppm, ppm_vma2ppn(&current_cluster->ppm, (void*)reserved));
		else
			page = ppm_addr2page(ppm, (void*)reserved);

		page_state_set(page, PGRESERVED);
	}

	pages_tbl = ppm->pages_tbl;

	for(i=0; i < ppm->pages_nr; i++)
	{
		page = &pages_tbl[i];

		if(page_state_get(page) != PGRESERVED)
			ppm_free_pages_nolock(ppm, page, page->order, i);
	}

	return 0;
}

static void ppm_free_pages_nolock(struct ppm_s *ppm, struct page_s *page, uint_t order, uint_t index)
{
	register struct page_s *block;
	register struct page_s *pages_tbl;
	register uint_t block_index;
	register uint_t page_index;
	register uint_t current_order;

	page_index = index;
	pages_tbl  = ppm->pages_tbl;
	page_state_set(page, PGFREE);
	ppm->free_pages_nr += 1 << order;
  
	for(current_order = order; current_order < PPM_LAST_ORDER; current_order ++)
	{
		block_index = page_index ^ (1 << current_order);
		block = pages_tbl + block_index;
    
		if((page_state_get(block) != PGFREE) || (block->order != current_order))
			break;

		list_unlink(&block->list);
		ppm->free_pages[current_order].pages_nr --;
    
		block->order = 0;
		page_index &= block_index;
	}
  
	block        = pages_tbl + page_index;
	block->order = current_order;

	list_add(&ppm->free_pages[current_order].root, &block->list);
	ppm->free_pages[current_order].pages_nr ++;
}

static struct page_s* ppm_do_alloc_pages(struct ppm_s *ppm, uint_t order, uint_t flags)
{
	struct page_s *block;
	struct page_s *remaining_block;
	register size_t current_size;
	register uint_t current_order;
	uint_t irq_state;

	assert(ppm->signature == PPM_ID);

	block = NULL;
	spinlock_lock_noirq(&ppm->lock, &irq_state);

	for(current_order = order; current_order < PPM_MAX_ORDER; current_order ++)
	{
		if(!(list_empty(&ppm->free_pages[current_order].root)))
		{
			block = list_first(&ppm->free_pages[current_order].root, struct page_s, list);
			list_unlink(&block->list);
			break;
		}
	}

	if(block == NULL)
		goto PPM_ALLOC_END;
  
	ppm->free_pages_nr -= 1 << order;
	ppm->free_pages[current_order].pages_nr --;  
	current_size = 1 << current_order;

	while(current_order > order)
	{
		current_order --;
		current_size >>= 1;
    
		remaining_block = block + current_size;
		remaining_block->order = current_order;

		list_add(&ppm->free_pages[current_order].root, &remaining_block->list);
		ppm->free_pages[current_order].pages_nr ++;
	}
  
	//PAGE_CLEAR(block, PG_FREE);
	page_state_set(block, PGINVALID);
	page_refcount_up(block);
	block->order = order;
  
PPM_ALLOC_END:
	spinlock_unlock_noirq(&ppm->lock, irq_state);
	return block;
}

///////////// private functions //////////////

#undef print

#if 0
#define  print(...) printk(DEBUG, __VA_ARGS__)
#else
#define  print(...) boot_dmsg(__VA_ARGS__) 
#endif

void ppm_print(struct ppm_s *ppm)
{
	uint_t i;
	struct list_entry *item;
	struct page_s *page;

	spinlock_lock(&ppm->lock);

	print("%s: Begin addr 0x%x:0x%x , Start addr 0x%x\nFree pages number %d\n",
	      __FUNCTION__, 
	      (uint_t) ppm->begin, 
		current_cid,
	      ppm->vaddr_start, 
	      ppm->free_pages_nr);
	
	//ppm_assert_private(ppm);
	
	for(i=0; i < PPM_MAX_ORDER; i++)
	{
		print("Order %d:\n  pages_nr %d\n  [",i,ppm->free_pages[i].pages_nr);
		
		list_foreach(&ppm->free_pages[i].root, item)
		{
			page = list_element(item, struct page_s, list);
			print("%d, ", page - ppm->pages_tbl);
		}
    
		print("\b\b]\n",NULL);
	}
	
	spinlock_unlock(&ppm->lock);
}


void ppm_assert_order(struct ppm_s *ppm)
{
	int order;
	struct list_entry *iter;
	struct page_s *page;
	
	for(order=0; order < PPM_MAX_ORDER; order++)
	{
		if(list_empty(&ppm->free_pages[order].root))
			continue;
		
		list_foreach(&ppm->free_pages[order].root, iter)
		{
			page = list_element(iter, struct page_s, list);

			if(page->order != order)
				goto PRIVATE_ERROR;
		}
	}
	return;
	
PRIVATE_ERROR:
	print("%s: Private inconsistency at order %d, page %d\n", 
	      __FUNCTION__, 
	      order, 
	      page - ppm->pages_tbl);
}

///////////////////// End Of File ///////////////////////
