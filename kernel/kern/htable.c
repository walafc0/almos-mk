/*
 * kern/htable.c - chained hash table
 * 
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012,2013,2014,2015 UPMC Sorbonne Universites
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

#include <htable.h>
#include <kmem.h>
#include <ppm.h>
#include <pmm.h>
#include <thread.h>

error_t hhalloc(struct hheader_s *hd, uint_t flags)
{
	uint_t i;
	kmem_req_t req;
	struct page_s *page;

	req.type          = KMEM_PAGE;
	req.size          = 0;
	req.flags         = flags;
	
	if(!(page = kmem_alloc(&req)))
		return ENOMEM;

	hd->nb_buckets = PMM_PAGE_SIZE/sizeof(struct list_entry);
	hd->buckets = ppm_page2addr(page);
	for(i=0; i < hd->nb_buckets; i++)
	{
		list_root_init(&hd->buckets[i]);
	}

	return 0;
}

error_t hhinit(struct hheader_s *hd, hhash_ft *hhash, hcompare_t *hcompare)
{

	hd->hhash = hhash;
	hd->hcompare = hcompare;

	return 0;
}

error_t hninit(struct hnode_s *hn)
{
	hn->hash = (uint_t) -1;
	list_entry_init(&hn->list);

	return 0;
}

struct hnode_s* __hfind(struct hheader_s *hd, hash_t hval, void *key)
{
	struct hnode_s *hn;
	struct list_entry *lh, *iter;

	lh = &hd->buckets[hval % hd->nb_buckets];

	list_foreach(lh, iter)
	{
		hn = list_element(iter, struct hnode_s, list);
		if(hn->hash == hval)
		{
			if(hd->hcompare(hn, key))
				return hn;
		}
	}

	return NULL;

}

struct hnode_s* hfind(struct hheader_s *hd, void *key)
{
	struct hnode_s *hn;
	hash_t hval = hd->hhash(key);

	if(!(hn =__hfind(hd, hval, key)))
		return false;

	return hn;
}

error_t hinsert(struct hheader_s *hd, struct hnode_s *hn, void *key)
{
	struct list_entry *lh;
	hash_t hval = hd->hhash(key);

	if(__hfind(hd, hval, key))
        {
                htbl_dmsg(1, "%s: Cannot insert <0x%x, %u> in hash table on cluster %u\n",
                                __FUNCTION__, key, hval, current_cid);
		return 1;
        }

	lh = &hd->buckets[hval % hd->nb_buckets];

	list_add_first(lh, &hn->list);

	hn->hash = hval;
        htbl_dmsg(1, "%s: <0x%x, %u> was correctly inserted in hash table on cluster %u\n",
                        __FUNCTION__, key, hval, current_cid);

	return 0;
}

error_t hremove(struct hheader_s *hd, void *key)
{
	struct hnode_s *hn;
	hash_t hval = hd->hhash(key);

	if(!(hn =__hfind(hd, hval, key)))
		return 1;

	list_unlink(&hn->list);

	return 0;
}

hash_t htable_int_default(void* key)
{
        hash_t *res = key;
        return((hash_t)( ((*res)*1103515245+12345) / 65536) % 32768);
}
