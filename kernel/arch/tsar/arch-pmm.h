/*
 * arch-pmm.h - architecture related implementation of pmm.h interface
 * (see mm/pmm.h)
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

#ifndef _ARCH_PMM_H_
#define _ARCH_PMM_H_

#ifndef _PMM_H_
#error This file must not be used directly, use pmm.h instead
#endif

/** Page Related Macros */
#undef PMM_PAGE_MASK 
#undef PMM_PAGE_SHIFT
#undef PMM_PAGE_SIZE
#undef PPM_HUGE_PAGE_MASK
#undef PPM_HUGE_PAGE_SHIFT
#undef PMM_HUGE_PAGE_SIZE

/** Generic Page Attributes */
#undef PMM_READ  
#undef PMM_WRITE  
#undef PMM_EXECUTE
#undef PMM_USER
#undef PMM_PRESENT
#undef PMM_DIRTY
#undef PMM_ACCESSED
#undef PMM_CACHED
#undef PMM_GLOBAL
#undef PMM_COW
#undef PMM_HUGE
#undef PMM_MIGRATE
#undef PMM_SWAP
#undef PMM_LOCKED
#undef PMM_CLEAR

/** Page flags */
#undef PMM_TEXT
#undef PMM_DATA
#undef PMM_UNKNOWN
#undef PMM_OTHER_TASK
#undef PMM_LAZY

/** MMU Exception Report */
#undef pmm_except_isWrite
#undef pmm_except_isExecute
#undef pmm_except_isPresent
#undef pmm_except_isRights
#undef pmm_except_isInKernelMode

#define _PMM_BIT_ORDER(x)     (1 << (x))

/** Page Related Macros */
#define PMM_PAGE_SHIFT          0x0c
#define PMM_PAGE_SIZE           0x1000
#define PMM_PAGE_MASK           0x0FFF

#define PMM_HUGE_PAGE_SHIFT     0x15
#define PMM_HUGE_PAGE_SIZE      0x200000
#define PMM_HUGE_PAGE_MASK      0x1FFFFF

#define PMM_CID_SHIFT          20

/** Generic Page Attributes */
#define PMM_PRESENT             _PMM_BIT_ORDER(31)
#define PMM_READ                _PMM_BIT_ORDER(31)
#define PMM_HUGE                _PMM_BIT_ORDER(30)
#define MMU_PTD1                _PMM_BIT_ORDER(30)
#define MMU_LACCESSED           _PMM_BIT_ORDER(29)
#define MMU_RACCESSED           _PMM_BIT_ORDER(28)
#define PMM_ACCESSED            (MMU_LACCESSED | MMU_RACCESSED)
#define PMM_CACHED              _PMM_BIT_ORDER(27)
#define PMM_WRITE               _PMM_BIT_ORDER(26)
#define PMM_EXECUTE             _PMM_BIT_ORDER(25)
#define PMM_USER                _PMM_BIT_ORDER(24)
#define PMM_GLOBAL              _PMM_BIT_ORDER(23)
#define PMM_DIRTY               _PMM_BIT_ORDER(22)
#define PMM_COW                 0x01
#define PMM_MIGRATE             0x02
#define PMM_SWAP                0x04
#define PMM_LOCKED              0x08
#define PMM_CLEAR               0x10

/** Page flags */
#define PMM_TEXT                0x001
#define PMM_DATA                0x002
#define PMM_UNKNOWN             0x003
#define PMM_OTHER_TASK          0x004
#define PMM_LAZY                0x008

/** MMU Exception Report */
#define MMU_EKMODE              0x0100

//#define pmm_except_isWrite(_flags)        (((_flags) & 0x000B) && !((_flags) & 0x1000))
//#define pmm_except_isPresent(_flags)      (((_flags) & 0x0003) && ((_flags) & 0x1000))

#define pmm_except_isWrite(_flags)        (((_flags) & 0x1000) == 0)
#define pmm_except_isExecute(_flags)      ((_flags)  & 0x0010)
#define pmm_except_isPresent(_flags)      ((_flags)  & 0x0003)
#define pmm_except_isRights(_flags)       ((_flags)  & 0x0004)
#define pmm_except_isInKernelMode(_flags) ((_flags)  & 0x0100)

#define MMU_EFATAL              0x01E0
#define MMU_EWMASK              0x0FFF

/* Internal Use */
#define MMU_PTE_SHIFT           PMM_PAGE_SHIFT
#define MMU_PTE_MASK            0x001FF000
#define MMU_PDE_SHIFT           PMM_HUGE_PAGE_SHIFT
#define MMU_PDE_MASK            0xFFE00000
#define MMU_PDE_PPN_MASK        0x0007FFFF
#define MMU_PDE_ATTR_MASK       0xFFC00000
#define MMU_PDE(vma)            ((vma) >> MMU_PDE_SHIFT)
#define MMU_PTE(vma)            (((((vma) & MMU_PTE_MASK)  >> MMU_PTE_SHIFT)) << 3)

#define MMU_PPN_MASK            ((_PMM_BIT_ORDER(28)) - 1)

#define MMU_IS_SET(_val,_flags)   (_val) & (_flags)
#define MMU_SET(_val,_flags)      (_val) |= (_flags)
#define MMU_CLEAR(_val,_flags)    (_val) &= ~(_flags)

/** 
 * Structure & Operations must be implemented 
 * by hardware-specific low-level code 
 */
struct cluster_s;
struct page_s;

/* Physical Memory Manager */
struct pmm_s
{
	uint_t *pgdir;
	uint_t flags;
	ppn_t pgdir_ppn;
	struct page_s *page;
	struct cluster_s *cluster;
	//struct semaphore_s sem;
};			

#endif	/* _ARCH_PMM_H_ */
