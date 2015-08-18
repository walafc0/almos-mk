/*
 * kern/htable.h - chained hash table
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


#ifndef _HTABLE_H_
#define _HTABLE_H_

#include <list.h>
#include <atomic.h>

struct hnode_s;
typedef uint_t hash_t;

typedef bool_t hcompare_t(struct hnode_s *hn, void* key);
typedef hash_t hhash_ft(void* key);

struct hheader_s
{
	uint_t nb_buckets;
	hhash_ft *hhash;
	hcompare_t *hcompare;
	struct list_entry *buckets;
};

struct hnode_s
{
	hash_t hash;
	struct list_entry list;	
};

/* Main functions */
error_t hninit(struct hnode_s *hn);
error_t hhalloc(struct hheader_s *hd, uint_t flags);
error_t hhinit(struct hheader_s *hd, hhash_ft *hhash, hcompare_t *hcompare);
struct hnode_s* __hfind(struct hheader_s *hd, hash_t hval, void *key);
error_t hinsert(struct hheader_s *hd, struct hnode_s *hn, void *key);
struct hnode_s* hfind(struct hheader_s *hd, void *key);
error_t hremove(struct hheader_s *hd, void *key);

/* Some useful features */
hash_t htable_int_default(void* key);

#endif /* _HTABLE_H_ */
