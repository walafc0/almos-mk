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

#ifndef _KU_TRANSFERT_H_
#define _KU_TRANSFERT_H_ 

#include <cpu.h>
#include <types.h>

struct ku_obj;

typedef uint_t ku_scpy_t(struct ku_obj *kuo, void *ptr, uint_t size);
typedef uint_t ku_copy_t(struct ku_obj *kuo, void *ptr);

typedef uint_t ku_size_t(struct ku_obj *kuo);
typedef uint_t ku_get_ppn_t(struct ku_obj *kuo, ppn_t *ppns, size_t nb_ppn);

struct ku_obj{
	void* buff;
	uint_t size;

	//funcs
	union{
		ku_copy_t *copy_to_buff;
		ku_scpy_t *scpy_to_buff;
	};
	union{
		ku_copy_t *copy_from_buff;
		ku_scpy_t *scpy_from_buff;
	};
	ku_size_t *get_ppn_max;//reuse the typedef of size
	ku_get_ppn_t *get_ppn;
	ku_size_t *get_size;
};

//copy to kuo
uint_t kk_scpy_to(struct ku_obj *kuo, void *ptr, uint_t size);
uint_t kk_copy_to(struct ku_obj *kuo, void *ptr);
//copy from kuo
uint_t kk_scpy_from(struct ku_obj *kuo, void *ptr, uint_t size);
uint_t kk_copy_from(struct ku_obj *kuo, void *ptr);
//kuo size
uint_t kk_strlen(struct ku_obj *kuo);
//get the ppns of addr in ppns table (filled or null teminated), 
//return the offset of buffer relative to the first ppn
uint_t kk_get_ppn(struct ku_obj *kuo, ppn_t *ppns, size_t nb_ppn);
//return the max number of ppns composing the buffer
uint_t kk_get_ppn_max(struct ku_obj *kuo);


//copy to user (from kernl)
uint_t ku_scpy_to(struct ku_obj *kuo, void *ptr, uint_t size);
uint_t ku_copy_to(struct ku_obj *kuo, void *ptr);
//copy from user (to kernel)
uint_t ku_scpy_from(struct ku_obj *kuo, void *ptr, uint_t size);
uint_t ku_copy_from(struct ku_obj *kuo, void *ptr);
//similar to the kk_* funcs
uint_t ku_strlen(struct ku_obj *kuo);
uint_t ku_get_ppn(struct ku_obj *kuo, ppn_t *ppns, size_t nb_ppn);
uint_t ku_get_ppn_max(struct ku_obj *kuo);

uint_t default_gsize(struct ku_obj *kuo);

#define __KU_OBJ_CONSTRUCTOR(name, buf, copy_to, copy_from, _ppn, _ppn_max, sz, get_sz) \
name.buff = buf;		\
name.copy_to_buff = (ku_copy_t*) &copy_to;	\
name.copy_from_buff = (ku_copy_t*) &copy_from;\
name.get_ppn = (ku_get_ppn_t*) &_ppn;	\
name.get_ppn_max = (ku_size_t*) &_ppn_max;\
name.get_size = &get_sz;	\
name.size = sz;		

#define KU_SZ_BUFF(name, buff, size) \
__KU_OBJ_CONSTRUCTOR(name, buff, ku_scpy_to, ku_scpy_from, ku_get_ppn, ku_get_ppn_max, size, default_gsize)

#define KU_SLICE_BUFF(name, buff, size) \
__KU_OBJ_CONSTRUCTOR(name, buff, ku_scpy_to, ku_scpy_from, ku_get_ppn, ku_get_ppn_max, size, default_gsize)

#define KU_BUFF(name, buff) \
__KU_OBJ_CONSTRUCTOR(name, buff, ku_copy_to, ku_copy_from, ku_get_ppn, ku_get_ppn_max, 0, ku_strlen)

#define KU_OBJ(name, obj) \
__KU_OBJ_CONSTRUCTOR(name, (void*)obj, ku_copy_to, ku_copy_from, ku_get_ppn, ku_get_ppn_max, sizeof(*obj), default_gsize)

#define KK_SZ_BUFF(name, buff, size) \
__KU_OBJ_CONSTRUCTOR(name, buff, kk_scpy_to, kk_scpy_from, kk_get_ppn, kk_get_ppn_max, size, default_gsize)


#define KK_BUFF(name, buff) \
__KU_OBJ_CONSTRUCTOR(name, buff, kk_copy_to, kk_copy_from, kk_get_ppn, kk_get_ppn_max, 0, kk_strlen)

#define KK_OBJ(name, obj) \
__KU_OBJ_CONSTRUCTOR(name, (void*)(obj), kk_copy_to, kk_copy_from, kk_get_ppn, kk_get_ppn_max, sizeof(*(obj)), default_gsize)

#if 0
#define KK_BUFF(name) \
struct ku_obj name = {	.buff = NULL,		\
			.get_len = kk_strlen,	\
			.copy_to_buff = kk_copy, \
			.copy_from_buff = kk_copy};


#define KU_OBJ(name, obj) \
struct ku_obj name = {	.buff = NULL,		\
			.get_len = ku_strlen,	\
			.copy_to_buff = ku_copy_to,	\
			.copy_from_buff = ku_copy_from	};
#endif
#endif
