/*
   This file is part of AlmOS.
  
   AlmOS is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   AlmOS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with AlmOS; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
   UPMC / LIP6 / SOC (c) 2009
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#ifndef _MMU_H_
#define _MMU_H_
#include <boot-info.h>

#define ARROUND_UP(val, size) (((val) & ((size) -1)) ? ((val) & ~((size)-1)) + (size) : (val))
#define ARROUND_DOWN(val, size)  ((val) & ~((size) - 1))

void __attribute__ ((section(".boot"))) boot_cluster_reset(void);
void __attribute__ ((section(".boot"))) other_proc_reset(void);

uint_t allocate_reserved_mem(uint_t size, uint_t align);

void memory_init(boot_info_t *binfo, kernel_info_t *kinfo, uint64_t tty, uint_t arch_cid);
void mmu_activate(uint32_t pgdir, unsigned long kentry, unsigned long arg);
uint_t cpu_get_id(void);
uint_t cpu_get_epc(void);
uint_t cpu_get_arch_id(void);
void cpu_invalid_dcache_line(void *ptr);
void cpu_wbflush(void);
void cpu_rbflush(void);
void cpu_power_idle(void);
void cpu_shutdown(void);
void cpu_goto_entry(uint32_t entry, uint_t arg);
void cpu_set_ebase(uint32_t vaddr_ebase);
void cpu_set_physical_ispace(uint_t arch_cid);
inline void set_mmu_mode(unsigned int val);
inline void mmu_physical_sw( unsigned long long paddr, unsigned int value); 
inline void mmu_physical_sb( unsigned long long paddr, char value); 
inline unsigned mmu_physical_cluster_rw(unsigned paddr,  unsigned ext ); 
inline unsigned mmu_physical_rw(uint64_t paddr); 
inline char mmu_physical_rb(uint64_t paddr); 
inline unsigned mmu_physical_atomic_inc(unsigned paddr,  unsigned ext); 
#endif	/* _MMU_H_ */
