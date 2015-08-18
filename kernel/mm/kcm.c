/*
 * mm/kcm.c - Per-cluster Kernel Cache Manager
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

#include <list.h>
#include <types.h>
#include <kcm.h>
#include <libk.h>
#include <kdmsg.h>
#include <bits.h>
#include <pmm.h>
#include <ppm.h>
#include <thread.h>
#include <page.h>
#include <cluster.h>

#define KCM_ALIGNE      CONFIG_CACHE_LINE_SIZE		/* minimum 32 */
#define KCM_PAGE_SIZE   PMM_PAGE_SIZE
#define KCM_PAGE_MASK   ~(KCM_PAGE_SIZE - 1)

////////////////////////////////
//     Private Section        //
////////////////////////////////


#define KCM_PAGE_ACTIVE  0x001
#define KCM_PAGE_BUSY    0x002

#define KCM_PAGE_IS(attr,flags) (attr) & (flags)
#define KCM_PAGE_SET(attr,flags) (attr) |= (flags)
#define KCM_PAGE_CLEAR(attr,flags) (attr) &= ~(flags)


typedef struct page_info_s
{
	BITMAP_DECLARE(bitmap,16);
	uint8_t *blk_tbl;
	struct kcm_s *kcm;
	uint8_t current_index;
	uint8_t refcount;
	uint8_t bitmap_size;
	uint8_t flags;
	struct list_entry list;
	struct page_s *page;
} page_info_t;


/** Allocate required block from given active page */
void* get_block(struct kcm_s *kcm, page_info_t *pinfo);

/** Free previously allocated blocks */
error_t put_block(struct kcm_s *kcm, void *ptr);

/** Try to populate freelist */
error_t freelist_populate(struct kcm_s *kcm);

/** Get one page from freelist and populate freelist if necessary */
page_info_t* freelist_get(struct kcm_s *kcm);

/** Free all freelist pages */
void freelist_release(struct kcm_s *kcm);

/** Replace the active page */
page_info_t* compute_active_page(struct kcm_s *kcm);


///////////////////////////////////////////////
//      Public functions implementation      //
///////////////////////////////////////////////

/* TODO: Register the cache into Main Allocator list */

/** Initialize a kernel Cache manager */
error_t kcm_init(struct kcm_s *kcm,
		 char *name,
		 size_t size,
		 uint_t aligne,
		 uint_t free_pages_min,
		 uint_t free_pages_max,
		 kcm_init_destroy_t *obj_init,
		 kcm_init_destroy_t *obj_destroy,
		 kcm_page_alloc_t *page_alloc_func,
		 kcm_page_free_t  *page_free_func)
{
	register uint_t blocks_nr;
	register uint_t remaining;
	page_info_t *pinfo;

	spinlock_init(&kcm->lock, "KCM");
  
	kcm->free_pages_nr   = 0;
	kcm->free_pages_min  = free_pages_min;
	kcm->free_pages_max  = free_pages_max;
	kcm->busy_pages_nr   = 0;
	kcm->active_pages_nr = 0;

	kcm->aligne    = ((aligne != 0) && (aligne < 33)) ? 32 : KCM_ALIGNE;
	kcm->size      = ARROUND_UP(size, kcm->aligne);
	blocks_nr      = KCM_PAGE_SIZE / kcm->size;
	remaining      = KCM_PAGE_SIZE % kcm->size;
	blocks_nr      = (remaining >= sizeof(page_info_t)) ? blocks_nr : blocks_nr - 1;
	kcm->blocks_nr = blocks_nr;
	kcm->name      = name;

	list_root_init(&kcm->freelist);
	list_root_init(&kcm->activelist);
	list_root_init(&kcm->busylist);

	kcm->obj_init    = obj_init;
	kcm->obj_destroy = obj_destroy;

	kcm->page_alloc = page_alloc_func;
	kcm->page_free  = page_free_func;
 
	if((pinfo = freelist_get(kcm)) == NULL)
	{
		spinlock_destroy(&kcm->lock);
		return -1;
	}
  
	list_add(&kcm->activelist, &pinfo->list);
	KCM_PAGE_SET(pinfo->flags, KCM_PAGE_ACTIVE);
	kcm->active_pages_nr ++;
	return 0;
}

/** Allocate any size but less than a page size */
void* kcm_alloc(struct kcm_s *kcm, uint_t flags)
{
	page_info_t *pinfo;
	page_info_t *pinfo_new;
	void *ptr = NULL;

	spinlock_lock(&kcm->lock);
    
	pinfo = list_first(&kcm->activelist, page_info_t, list);
  
	if((ptr = get_block(kcm, pinfo)) == NULL)
	{
		if((pinfo_new = compute_active_page(kcm)) != NULL)
			if(pinfo_new != pinfo)
				ptr = get_block(kcm, pinfo_new);
	}
  
	spinlock_unlock(&kcm->lock);
	return ptr;
}


/** Free previous allocated block */
void kcm_free (void *ptr)
{
	page_info_t *pinfo;
	struct kcm_s *kcm;
  
	if(ptr == NULL) return;
	
	pinfo = (page_info_t*)((uint_t)ptr & KCM_PAGE_MASK);
	kcm = pinfo->kcm;

	spinlock_lock(&kcm->lock);
	put_block(kcm, ptr);
	spinlock_unlock(&kcm->lock);
}

/** Shrink buffered pages if any, it should be called periodically */
void kcm_release(struct kcm_s *kcm)
{
	spinlock_lock(&kcm->lock);
	freelist_release(kcm);
	spinlock_unlock(&kcm->lock);
}


/////////////////////////////////////////////
//     Private functions implementation    //
/////////////////////////////////////////////


/** Allocate required blocks number from given active page */
void* get_block(struct kcm_s *kcm, page_info_t *pinfo)
{
	register sint_t index;
 
	if((pinfo->current_index >= kcm->blocks_nr) || 
	   ((index = bitmap_ffs2(pinfo->bitmap, pinfo->current_index, sizeof(pinfo->bitmap))) == -1))
		return NULL;
  
        if ( index >= kcm->blocks_nr )
                return NULL;

	bitmap_clear(pinfo->bitmap, index);
	pinfo->current_index = index + 1;
	pinfo->refcount ++;

	return pinfo->blk_tbl + index * kcm->size;
}


/** Free previously allocated block */
error_t put_block(struct kcm_s *kcm, void *ptr)
{
	page_info_t *pinfo;
	register uint_t index; 
  
	pinfo = (page_info_t*)((uint_t)ptr & KCM_PAGE_MASK);
	index = ((uint8_t*)ptr - pinfo->blk_tbl) / kcm->size;
  
	bitmap_set(pinfo->bitmap, index);
	pinfo->current_index = (index < pinfo->current_index) ? index : pinfo->current_index;
	pinfo->refcount --;
  
	if(KCM_PAGE_IS(pinfo->flags, KCM_PAGE_BUSY))
	{
		KCM_PAGE_CLEAR(pinfo->flags, KCM_PAGE_BUSY);
		list_unlink(&pinfo->list);
		kcm->busy_pages_nr --;
		list_add_last(&kcm->activelist, &pinfo->list);
		kcm->active_pages_nr ++;
		return 0;
	}

	if(!(KCM_PAGE_IS(pinfo->flags, KCM_PAGE_ACTIVE)) && (pinfo->refcount == 0))
	{
		/* TODO: check if it is a remote page and there is more
		 * pages than the low watermark, then return it to its ppm.
		 * Returning back a remote page should be a deferred local event */

		list_unlink(&pinfo->list);
		list_add_first(&kcm->freelist, &pinfo->list);
		kcm->free_pages_nr ++;
		kcm->active_pages_nr --;
	}
  
	return 0;
}

/** Try to populate freelist */
error_t freelist_populate(struct kcm_s *kcm)
{
	register struct page_s *page;
	register page_info_t *ptr;
	register uint8_t *blk_tbl;
	register uint_t i;

	assert(kcm->page_alloc != NULL);

	if((page = kcm->page_alloc(kcm)) == NULL)
		return -1;
 
	ptr = ppm_page2addr(page);

	bitmap_set_range(ptr->bitmap, 0, kcm->blocks_nr);

	ptr->current_index = 0;
	ptr->flags         = 0;
	ptr->refcount      = 0;
	blk_tbl            = (uint8_t*)((uint_t)ptr + (ARROUND_UP(sizeof(page_info_t), kcm->aligne)));
	ptr->blk_tbl       = blk_tbl;
	ptr->kcm           = kcm;
	ptr->page          = page;

	if(kcm->obj_init != NULL)
		for(i=0; i < kcm->blocks_nr; i++)
			kcm->obj_init(kcm, blk_tbl + kcm->size * i);

	list_add(&kcm->freelist, &ptr->list);
	kcm->free_pages_nr ++;
 
	return 0;
}

/** Get one page from freelist and populate freelist if necessary */
page_info_t* freelist_get(struct kcm_s *kcm)
{
	register error_t err;
	page_info_t *ptr;

	if(kcm->free_pages_nr <= kcm->free_pages_min)
	{
		while(kcm->free_pages_nr < ((kcm->free_pages_min + kcm->free_pages_max) / 2))
		{
			if((err = freelist_populate(kcm)) != 0)
				break;
		}
	}

	assert(kcm->free_pages_nr != 0);

	if(kcm->free_pages_nr == 0)
		return NULL;
  
	ptr = list_first(&kcm->freelist, page_info_t, list);
	list_unlink(&ptr->list);
	kcm->free_pages_nr --;

	return ptr;
}

/** Free all free pages in the cache */
void freelist_release(struct kcm_s *kcm)
{
	register uint_t i;
	page_info_t *ptr;
	char *blk_tbl;
	struct list_entry *item;
  
	list_foreach(&kcm->freelist, item)
	{
		if(kcm->free_pages_nr <= kcm->free_pages_min)
			break;

		ptr = list_element(item, page_info_t, list);
		list_unlink(item);
		kcm->free_pages_nr --;
 
		blk_tbl = (char*) ptr + sizeof(page_info_t);
    
		if(kcm->obj_destroy != NULL)
			for(i=0; i < kcm->blocks_nr; i++)
				kcm->obj_destroy(kcm, blk_tbl + kcm->size * i);
    
		kcm->page_free(kcm, ptr->page);
	}
}


/** Replace the active page with one probably more adequate */
page_info_t* compute_active_page(struct kcm_s *kcm)
{
	page_info_t *pinfo;
	struct list_entry *ptr;
	page_info_t *pinfo_new;
  
	pinfo = list_first(&kcm->activelist, page_info_t, list);

	if((ptr = list_next(&kcm->activelist, &pinfo->list)) == NULL)
	{
		if((pinfo_new = freelist_get(kcm)) == NULL)
			return pinfo;
    
		list_add_last(&kcm->activelist, &pinfo_new->list);
		kcm->active_pages_nr ++;
	}
	else
	{
		pinfo_new = list_element(ptr, page_info_t, list);
	}

	KCM_PAGE_CLEAR(pinfo->flags, KCM_PAGE_ACTIVE);
	list_unlink(&pinfo->list);
	kcm->active_pages_nr --;
	KCM_PAGE_SET(pinfo->flags, KCM_PAGE_BUSY);
	list_add_first(&kcm->busylist, &pinfo->list);
	kcm->busy_pages_nr ++;
  
	KCM_PAGE_SET(pinfo_new->flags, KCM_PAGE_ACTIVE);

	return pinfo_new;
}

/* TODO: deal with the caller AF_FLAGS
 * check if the gotten page is a remote one */
struct page_s* kcm_page_alloc(struct kcm_s *kcm)
{
	register struct cluster_s *cluster;
	register struct page_s *page;

	cluster = current_cluster;
  
	page = ppm_alloc_pages(&cluster->ppm, 0, AF_KERNEL);

	if(page == NULL)
	{
		printk(ERROR,"ERROR: %s: faild to allocate page, cpu %d, cluster %d\n", 
		       cpu_get_id(), 
		       cluster->id);

		return NULL;
	}

	return page;
}

void kcm_page_free(struct kcm_s *kcm, struct page_s *page)
{
	assert(page != NULL);
	ppm_free_pages(page);
}


/* ======================================================================================== */

static void print_pinfo(page_info_t *pinfo)
{
	printk(DEBUG, "pinfo:\n  Current_index %d\nRefcount %d\n", 
	       pinfo->current_index,
	       pinfo->refcount);
}


void kcm_print(struct kcm_s *kcm)
{
	struct list_entry *item;

	printk(DEBUG,"+++++++++++++\nkcm [%s] state:\n", kcm->name);

	printk(DEBUG,"\tfree_pages_nr %d\n\tbusy_pages_nr %d\n", 
	       kcm->free_pages_nr, 
	       kcm->busy_pages_nr);
  
	printk(DEBUG,"-----\n\tActive Pages:\n-----\n");

	list_foreach_forward(&kcm->activelist, item)
	{
		print_pinfo(list_element(item, page_info_t, list));
	}
  
	printk(DEBUG,"-----\n+++++++++++++\n");
}
