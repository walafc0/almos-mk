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

#ifndef _PPN_H_
#define _PPN_H_


cid_t ppn_ppn2cid(ppn_t ppn);

bool_t ppn_is_local(ppn_t ppn);

vma_t ppn_ppn2vma(ppn_t ppn);

void ppn_copy(ppn_t dest, ppn_t src);

void ppn_refcount_down(ppn_t ppn);

void ppn_refcount_up(ppn_t ppn);

#endif
