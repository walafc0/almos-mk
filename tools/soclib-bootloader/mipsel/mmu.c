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
*/

#include <mmu.h>
#include <mips32_registers.h>

//#define CP2_DATA_PADDR_EXT   $24
//#define CP2_INST_PADDR_EXT   $25

uint_t cpu_get_id(void)
{
  register unsigned int proc_id;

  asm volatile ("mfc0    %0,  $0" : "=r" (proc_id));
  
  return proc_id;
}

uint_t cpu_get_epc(void)
{
  register unsigned int proc_epc;

  asm volatile ("mfc0    %0,  $14" : "=r" (proc_epc));
  
  return proc_epc;
}

uint_t cpu_get_arch_id(void)
{
  register unsigned int proc_id;

  asm volatile ("mfc0    %0,  $15, 1" : "=r" (proc_id));
  
  return (proc_id & 0xFFF);
}

void cpu_invalid_dcache_line(void *ptr)
{
  __asm__ volatile
    ("cache    %0,     (%1)              \n"
     : : "i" (0x11) , "r" (ptr)
    );
}

void cpu_wbflush(void)
{
  __asm__ volatile
    ("sync                               \n"::);
}

void cpu_rbflush(void)
{
  asm volatile("" ::: "memory");
}

void cpu_power_idle(void)
{
  __asm__ volatile
    ("wait                               \n"::);
}

void cpu_shutdown(void)
{
  __asm__ volatile
    ("1:                                 \n"
     "wait                               \n"
     "nop                                \n"
     "j      1b                          \n"
     "nop                                \n"
     ::);
}

//FG : to set to the right value EBase
//Exception entry point address
void cpu_set_ebase(uint32_t ebase)
{
   asm volatile
      ("mtc0 %0, $15, 1 \n"
      ::"r" (ebase));
}

void cpu_goto_entry(uint32_t entry, uint_t arg)
{
  asm volatile
    (".set noreorder          \n"
     "or     $31,  $0,  %0    \n"
     "or     $4,   $0,  %1    \n"
     "addiu  $29,  $29, -4    \n"
     "jr     $31              \n"
     "sw     $4,  0($29)      \n"
     ".set reorder            \n"
     :: "r" (entry), "r" (arg));
}

void cpu_set_physical_ispace(uint_t arch_cid)
{
   asm volatile
      ("mtc2 %0, $25\n"
      ::"r" (arch_cid));
}

void mmu_activate(uint32_t pgdir, unsigned long kentry, unsigned long arg)
{
   //FG
  //pgdir = pgdir >> 13;

  asm volatile 
    (".set noreorder          \n"
     "or     $26,  $0,  %0    \n"
     "or     $31,  $0,  %1    \n"
     "addiu  $29,  $29, -4    \n"
     "or     $4,   $0,  %2    \n"
     "sw     $4,   0($29)     \n"
     "mtc2   $26,  $0         \n"     /* PTPR <= pgdir */
     "nop                     \n"
     "ori    $27,  $0,  0xF   \n"
     "mtc2   $27,  $1         \n"     /* MMU_MODE <= 0xf */
     "jr     $31              \n"
     "mtc2   $0,     $24      \n"     /* PADDR_EXT <= 0  */   
     ".set reorder            \n"
     :: "r"(pgdir), "r"(kentry), "r" (arg));
}

inline void set_mmu_mode(unsigned int val) 
{
    asm volatile ( "mtc2     %0,     $1             \n"
                   :
                   :"r" (val) );
}

#if DEBUG_MMU
#include <dmsg.h>
extern uint64_t boot_tty;
#endif
//Both mmu and irq are supposed to be disabled
inline void mmu_physical_sw(unsigned long long paddr,  unsigned int value ) 
{
    unsigned int lsb = (unsigned int)paddr;
    unsigned int msb = (unsigned int)(paddr >> 32);

#if DEBUG_MMU
    boot_dmsg(boot_tty, "%s: %p\n", __FUNCTION__, paddr);
#endif
    asm volatile(
            "mfc2   $8,     $24                \n"     /* $8 <= PADDR_EXT */   
            "mtc2   %2,     $24                \n"     /* PADDR_EXT <= msb */   
            "sw     %0,     0(%1)              \n"     /* *paddr <= value  */
            "mtc2   $8,     $24                \n"    /* restore PADDR */
            :: "r" (value), "r" (lsb), "r" (msb));
}

//This function makes a physical write without dealing with DTLB and IRQ
//Function to be used in physical addressing context (no MMU activated)
//WARNING : cannot be change into _physical_write : restore PADDR_EXT
inline void mmu_physical_sb( unsigned long long paddr, char value ) 
{
    unsigned int lsb = (unsigned int)paddr;
    unsigned int msb = (unsigned int)(paddr >> 32);

    asm volatile(
            "mfc2   $8,     $24                \n"     /* $8 <= PADDR_EXT   */
            "mtc2   %2,     $24                \n"     /* PADDR_EXT <= msb */   
            "sb     %0,     0(%1)              \n"     /* *paddr <= value  */
            "mtc2   $8,     $24                \n"     /* restore PADDR_EXT */   
            "nop                               \n"
            :: "r" (value), "r" (lsb), "r" (msb));
}

//Both mmu and irq are supposed to be disabled
inline unsigned mmu_physical_cluster_rw(unsigned paddr,  unsigned ext ) 
{
    register unsigned int value;
    unsigned int lsb = paddr;
    unsigned int msb = ext;

    asm volatile(
            "mfc2   $8,     $24                \n"     /* $8 <= PADDR_EXT */   
            "mtc2   %2,     $24                \n"     /* PADDR_EXT <= msb */   
            "lw     %0,     0(%1)              \n"     /* *paddr <= value  */
            "mtc2   $8,     $24                 \n"    /* restore PADDR */
            :"=r" (value): "r" (lsb), "r" (msb));

    return value;
}

//Both mmu and irq are supposed to be disabled
inline unsigned mmu_physical_rw(uint64_t paddr) 
{
    unsigned int lsb = (unsigned int) paddr;
    unsigned int msb = (unsigned int) (paddr >> 32);
    return mmu_physical_cluster_rw(lsb, msb);
}

//Both mmu and irq are supposed to be disabled
inline char mmu_physical_cluster_rb(unsigned paddr,  unsigned ext ) 
{
    register char value;
    unsigned int lsb = paddr;
    unsigned int msb = ext;

    asm volatile(
            "mfc2   $8,     $24                \n"     /* $8 <= PADDR_EXT */   
            "mtc2   %2,     $24                \n"     /* PADDR_EXT <= msb */   
            "lb     %0,     0(%1)              \n"     /* *paddr <= value  */
            "mtc2   $8,     $24                 \n"    /* restore PADDR */
            :"=r" (value): "r" (lsb), "r" (msb));

    return value;
}

inline char mmu_physical_rb(uint64_t paddr) 
{
    unsigned int lsb = (unsigned int)paddr;
    unsigned int msb = (unsigned int)(paddr >> 32);
    return mmu_physical_cluster_rb(lsb, msb);
}

unsigned __physical_cas(unsigned lsb, unsigned msb, unsigned old, unsigned new)
{
    int isAtomic;
    asm volatile(
	    ".set noreorder                     \n"
            "mfc2   $9,     $24                \n"     /* $9 <= PADDR_EXT */   
            "mtc2   %4,     $24                \n"     /* PADDR_EXT <= msb */   

            /* CAS */
            "sync                               \n"
            "or      $8,      $0,       %3      \n"
            "ll      $3,      (%1)              \n"
            "bne     $3,      %2,       1f      \n"
            "li      $7,      0                 \n"
            "sc      $8,      (%1)              \n"
            "or      $7,      $8,       $0      \n"
            "sync                               \n"
            ".set reorder                       \n"
            "1:                                 \n"
            "or      %0,      $7,       $0      \n"

            "mtc2   $9,     $24                 \n"    /* restore PADDR */
		      : "=&r" (isAtomic): "r" (lsb), "r" (old) , "r" (new), "r" (msb) : "$3", "$7", "$8", "$9");
            //:: "r" (val), "r" (lsb), "r" (msb));

		  return isAtomic;
}

//Both mmu and irq are supposed to be disabled
inline unsigned mmu_physical_atomic_inc(unsigned paddr,  unsigned ext) 
{
    unsigned int value;
    unsigned int lsb = paddr;
    unsigned int msb = ext;
    int isAtomic = 0;

    do{
		  value = mmu_physical_cluster_rw(lsb, msb);
    }while(!__physical_cas(lsb, msb, value, value+1));

    return value;
}

#if 0
inline unsigned mmu_physical_atomic_inc(uint64_t paddr) 
{
    unsigned int lsb = (unsigned int)paddr;
    unsigned int msb = (unsigned int)(paddr >> 32);
    mmu_physical_atomic_inc(paddr, msb);
}
#endif
