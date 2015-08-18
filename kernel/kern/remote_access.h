/*
 * kern/remote_access.h - 
 * 
 * Copyright (c) 2011,2012,2013,2014 UPMC Sorbonne Universites
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

#ifndef _REMOTE_ACCESS_H_
#define _REMOTE_ACCESS_H_ 

#include <types.h>

inline void remote_sw(void* addr, cid_t cid, uint_t val);

inline void remote_sb(void* addr, cid_t cid, char val);

inline uint_t remote_lw(void* addr, cid_t cid);

inline bool_t remote_atomic_cas(void* addr, cid_t cid, uint_t old_val, uint_t val);

inline sint_t remote_atomic_add(void *ptr, cid_t cid, sint_t add_val);

inline void remote_memcpy(void* dest, cid_t dcid, void* src, cid_t scid, size_t size);

#endif
