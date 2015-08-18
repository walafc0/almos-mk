/*
 * kern/remote_access.c - 
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

#include <cpu-remote.h>
#include <cpu.h>
#include <string.h>
#include <types.h>

inline void remote_sw(void* addr, cid_t cid, uint_t val)
{
	cpu_remote_sw(addr, cid, val);
}

inline void remote_sb(void* addr, cid_t cid, char val)
{
	cpu_remote_sb(addr, cid, val);
}

inline uint_t remote_lw(void* addr, cid_t cid)
{
	return cpu_remote_lw(addr, cid);
}

inline bool_t remote_atomic_cas(void* addr, cid_t cid, uint_t old_val, uint_t val)
{
	return cpu_remote_atomic_cas(addr, cid, old_val, val);
}

inline sint_t remote_atomic_add(void *ptr, cid_t cid, sint_t add_val)
{
	sint_t old_val;
	sint_t new_val;

	do
	{
		old_val = remote_lw(ptr, cid);
		new_val = old_val + add_val;
	}while(!remote_atomic_cas(ptr, cid, old_val, new_val));

	return old_val;
}

inline void remote_memcpy(void* dest, cid_t dcid, void* src, cid_t scid, size_t size)
{
	if((dcid!= cpu_get_cid()) || (scid!= cpu_get_cid()))
		cpu_remote_memcpy(dest, dcid, src, scid, size);
	else
		memcpy(dest, src, size);
}
