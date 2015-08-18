/*
 * kern/keysdb.c - fixed radix-cache implementation
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

#include <errno.h>
#include <libk.h>
#include <kdmsg.h>
#include <bits.h>
#include <page.h>
#include <ppm.h>
#include <pmm.h>
#include <thread.h>
#include <task.h>
#include <kmem.h>
#include <keysdb.h>

#define KEYS_PER_REC        64
#define KEYS_PER_REC_LOG2   6

struct keys_record_s
{
	void *tbl[KEYS_PER_REC];
};

typedef struct keys_record_s keys_record_t;

KMEM_OBJATTR_INIT(keysrec_kmem_init)
{
	attr->type   = KMEM_KEYSREC;
	attr->name   = "KCM KEYSREC";
	attr->size   = sizeof(struct keys_record_s);
	attr->aligne = 0;
	attr->min    = CONFIG_KEYREC_MIN;
	attr->max    = CONFIG_KEYREC_MAX;
	attr->ctor   = NULL;
	attr->dtor   = NULL;
	return 0;
}

error_t keysdb_init(struct keysdb_s *db, uint_t key_width)
{
	struct page_s *page;
	kmem_req_t req;

	req.type  = KMEM_PAGE;
	req.size  = 0;
	req.flags = AF_USER | AF_ZERO;
  
	db->records_nr   = PMM_PAGE_SIZE / sizeof(keys_record_t*);
	db->records_bits = bits_log2(db->records_nr);

	assert((db->records_bits + KEYS_PER_REC_LOG2) == key_width);

	db->keys_per_record = KEYS_PER_REC;
	db->key_width = key_width;
	page = kmem_alloc(&req);
  
	if(page == NULL) return ENOMEM;
  
	db->root = ppm_page2addr(page);
	db->page = page;

	return 0;
}

error_t keysdb_destroy(struct keysdb_s *db)
{
	kmem_req_t req;
	uint_t i;

	req.type = KMEM_KEYSREC;

	for(i=0; i < db->records_nr; i++)
	{
		if(db->root[i] == NULL)
			continue;

		req.ptr = db->root[i];
		kmem_free(&req);
	}

	req.type = KMEM_PAGE;
	req.ptr = db->page;
	kmem_free(&req);

	return 0;
}

void keysdb_record_print(keys_record_t *rec, uint_t index, uint_t size)
{
	uint_t i;

	assert(rec != NULL);
    
	printk(INFO, "[ %d ] [rec @0x%x] [", index, rec);
      
	for(i=0; i < size; i++)
		printk(INFO, "%x, ", rec->tbl[i]);

	printk(INFO, "]\n");
}

void keysdb_print(struct keysdb_s *db)
{
	uint_t i;
 
	printk(INFO, "%s: started [ keys per record %d ] :\n", __FUNCTION__, db->keys_per_record);

	for(i=0; i < db->records_nr; i++)
	{
		if(db->root[i] == NULL)
			continue;
    
		keysdb_record_print(db->root[i], i, db->keys_per_record);
	}
}


static inline void keysdb_index(struct keysdb_s *db, uint_t key, uint_t *rec_index, uint_t *key_index)
{
	uint_t bits;

	bits       = db->key_width - db->records_bits;
	*rec_index = key >> bits;
	*key_index = key & ((1 << bits) - 1);
}

error_t keysdb_insert_seq(struct keysdb_s *db, uint_t key, void *value)
{
	kmem_req_t req;
	keys_record_t *rec;
	uint_t record_index;
	uint_t key_index;

	keysdb_index(db, key, &record_index, &key_index);
  
	if(db->root[record_index] == NULL)
	{
		req.type = KMEM_KEYSREC;
		req.size = sizeof(keys_record_t);
		req.flags = AF_USER;
		db->root[record_index] = kmem_alloc(&req);
    
		if(db->root[record_index] == NULL) 
			return ENOMEM;
    
		memset(db->root[record_index], 0, req.size);
   	}
  
	rec = db->root[record_index];
  
	if(rec->tbl[key_index] != NULL) 
		return EEXIST;
	
	rec->tbl[key_index] = value;
	return 0;
}

error_t keysdb_insert(struct keysdb_s *db, uint_t key, void *value)
{
	kmem_req_t req;
	keys_record_t *rec;
	uint_t record_index;
	uint_t key_index;
	bool_t isAtomic;
	bool_t isAllocated;
	volatile keys_record_t **ptr;

	keysdb_index(db, key, &record_index, &key_index);

	isAllocated = false;
	isAtomic    = false;
	rec         = NULL;
	ptr         = (volatile keys_record_t**)&db->root[record_index];

	while((*ptr == NULL) && (isAtomic == false))
	{
		if(rec == NULL)
		{
			req.type = KMEM_KEYSREC;
			req.size = sizeof(keys_record_t);
			req.flags = AF_USER;
			rec = kmem_alloc(&req);

			if(rec == NULL)
				return ENOMEM;

			memset(rec, 0, req.size);
			isAllocated = true;
		}

		isAtomic = cpu_atomic_cas((void*)&db->root[record_index], 0, (uint_t)rec);
	}

	if(isAllocated && (db->root[record_index] != rec))
	{
		req.ptr = rec;
		kmem_free(&req);
	}
  
	rec = db->root[record_index];

	if(rec->tbl[key_index] != NULL)
		return EEXIST;

	rec->tbl[key_index] = value;
	return 0;
}


error_t keysdb_bind(struct keysdb_s *db, uint_t key_start, uint_t keys_nr, void *value)
{
	error_t err;
	uint_t i;

	err = 0;

	for(i = 0; i < keys_nr; i++)
	{
		err = keysdb_insert(db, key_start + i, value);
		if(err != 0) break;
	}

	while((err != 0) && (i != 0))
	{
		i--;
		keysdb_remove(db, key_start + i);
	}

	return err;
}

error_t keysdb_unbind(struct keysdb_s *db, uint_t key_start, uint_t keys_nr)
{
	uint_t i;

	for(i = 0; i < keys_nr; i++)
		(void)keysdb_remove(db, key_start + i);

	return 0;
}


void* keysdb_remove(struct keysdb_s *db, uint_t key)
{
	void *value;
	uint_t record_index;
	uint_t key_index;
	keys_record_t *rec;
  
	keysdb_index(db, key, &record_index, &key_index);
  
	rec = db->root[record_index];
 
	if(rec == NULL) return NULL;

	value = rec->tbl[key_index];
	rec->tbl[key_index] = NULL;
	cpu_wbflush();

	return value;
}

void* keysdb_lookup(struct keysdb_s *db, uint_t key)
{
	uint_t record_index;
	uint_t key_index;
	keys_record_t *rec;
	void *val;

	keysdb_index(db, key, &record_index, &key_index);

	rec = db->root[record_index]; 
	val = (rec == NULL) ? NULL : rec->tbl[key_index];

	return val;
}
