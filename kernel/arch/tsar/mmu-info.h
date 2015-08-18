/*
 * mmu-info.h - TSAR MMU registers & exceptions related informations
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

#ifndef _MMU_INFO_H_
#define _MMU_INFO_H_

#ifndef _USE_MMU_INFO_H_
#error This file must not be used directly
#endif

/* MMU Registers Interface */
#define MMU_PTPR 	        0 	/* Page Table Pointer Register 	              R/W  */
#define MMU_MODE 	        1 	/* Mode Register 	                      R/W  */
#define MMU_ICACHE_FLUSH 	2 	/* Instruction Cache flush 	              W    */
#define MMU_DCACHE_FLUSH 	3 	/* Data Cache flush 	                      W    */
#define MMU_ITLB_INVAL 	        4 	/* Instruction TLB line invalidation 	      W    */
#define MMU_DTLB_INVAL        	5 	/* Data TLB line Invalidation 	              W    */
#define MMU_ICACHE_INVAL 	6 	/* Instruction Cache line invalidation 	      W    */
#define MMU_DCACHE_INVAL 	7 	/* Data Cache line invalidation 	      W    */
#define MMU_ICACHE_PREFETCH 	8 	/* Instruction Cache line prefetch 	      W    */
#define MMU_DCACHE_PREFETCH 	9 	/* Data Cache line prefetch 	              W    */
#define MMU_SYNC 	        10 	/* Complete pending writes 	              W    */
#define MMU_IETR 	        11 	/* Instruction Exception Type Register 	      R    */
#define MMU_DETR 	        12 	/* Data Exception Type Register 	      R    */
#define MMU_IBVAR 	        13 	/* Instruction Bad Virtual Address Register   R    */
#define MMU_DBVAR 	        14 	/* Data Bad Virtual Address Register 	      R    */
#define MMU_PARAMS 	        15 	/* Caches & TLBs hardware parameters 	      R    */
#define MMU_RELEASE 	        16 	/* Generic MMU release number 	              R    */
#define MMU_WORD_LO 	        17 	/* Lowest part of a double word 	      R/W  */
#define MMU_WORD_HI 	        18 	/* Highest part of a double word 	      R/W  */
#define MMU_ICACHE_PA_INV 	19 	/* Instruction cache inval physical adressing W    */
#define MMU_DCACHE_PA_INV 	20 	/* Data cache inval physical addressing       W    */
#define MMU_DOUBLE_LL 	        21 	/* Double word linked load 	              W    */
#define MMU_DOUBLE_SC 	        22 	/* Double word store conditional 	      W    */ 

#include <types.h>

/* MMU Exception Descriptor */
struct mmu_except_info_s
{
	uint_t err;
	char *name;
	char *info;
	char *severty;
};

typedef struct mmu_except_info_s mmu_except_info_t;

inline mmu_except_info_t* mmu_except_get_entry(uint_t mmu_err_val);

#endif	/* _MMU_INFO_H_ */
