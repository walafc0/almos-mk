/*
 * kern/keysdb.h - fixed radix-cache interface
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

#ifndef _KEYS_DB_H_
#define _KEYS_DB_H_

#include <types.h>
#include <kmem.h>

//////////////////////////////////////////////////
//               Public Section                 //
//////////////////////////////////////////////////

struct keysdb_s;

error_t keysdb_init(struct keysdb_s *db, uint_t key_width);

error_t keysdb_destroy(struct keysdb_s *db);

error_t keysdb_insert(struct keysdb_s *db, uint_t key, void *value);

error_t keysdb_bind(struct keysdb_s *db, uint_t key_start, uint_t keys_nr, void *value);

error_t keysdb_unbind(struct keysdb_s *db, uint_t key_start, uint_t keys_nr);

void* keysdb_remove(struct keysdb_s *db, uint_t key);

void* keysdb_lookup(struct keysdb_s *db, uint_t key);

KMEM_OBJATTR_INIT(keysrec_kmem_init);

//////////////////////////////////////////////////
//               Private Section                //
//////////////////////////////////////////////////

struct keys_record_s;

struct keysdb_s
{
	struct keys_record_s **root;
	uint_t records_nr;
	uint_t records_bits;
	uint_t key_width;
	uint_t keys_per_record;
	struct page_s *page;
};

void keysdb_print(struct keysdb_s *db);

#endif	/* _KEYS_DB_H_ */

