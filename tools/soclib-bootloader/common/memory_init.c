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

#include <boot-info.h>
#include <mmu.h>
#include <dmsg.h>
#include <string.h>



#define _PMM_BIT_ORDER(x)       (1 << (x))

#define PMM_PRESENT             _PMM_BIT_ORDER(31)
#define PMM_READ                _PMM_BIT_ORDER(31)
#define PMM_HUGE                _PMM_BIT_ORDER(30)
#define MMU_PTD1                _PMM_BIT_ORDER(30)
#define MMU_LACCESSED           _PMM_BIT_ORDER(29)
#define MMU_RACCESSD            _PMM_BIT_ORDER(28)
#define PMM_ACCESSED            (MMU_LACCESSED | MMU_RACCESSD)
#define PMM_CACHED              _PMM_BIT_ORDER(27)
#define PMM_WRITE               _PMM_BIT_ORDER(26)
#define PMM_EXECUTE             _PMM_BIT_ORDER(25)
#define PMM_USER                _PMM_BIT_ORDER(24)
#define PMM_GLOBAL              _PMM_BIT_ORDER(23)
#define PMM_DIRTY               _PMM_BIT_ORDER(22)

#define MMU_PDIR_SHIFT          21
#define MMU_PDIR_MASK           0xFFE00000

#define PMM_PAGE_SHIFT          12
#define PMM_PAGE_SIZE           0x1000
#define PMM_PAGE_MASK           0x0FFF

#define PMM_HUGE_PAGE_SHIFT     21
#define PMM_HUGE_PAGE_SIZE      0x2000
#define PMM_HUGE_PAGE_OFFSET_MASK 0x1FFFFF

#define KATTR_HUGE  (PMM_HUGE | PMM_READ | PMM_WRITE | PMM_EXECUTE | PMM_CACHED | PMM_GLOBAL | PMM_DIRTY | PMM_ACCESSED)
#define KATTR  (PMM_READ | PMM_WRITE | PMM_EXECUTE | PMM_CACHED | PMM_GLOBAL | PMM_DIRTY | PMM_ACCESSED)
#define KDEV_ATTR (PMM_HUGE | PMM_READ | PMM_WRITE | PMM_GLOBAL | PMM_DIRTY | PMM_ACCESSED)

#define MMU_PDE(vma)            (((vma) & MMU_PDIR_MASK) >> MMU_PDIR_SHIFT)

#define MMU_PTE_SHIFT           PMM_PAGE_SHIFT
#define MMU_PTE_MASK            0x001FF000
#define MMU_PTE(vma)            (((((vma) & MMU_PTE_MASK)  >> MMU_PTE_SHIFT)) << 3)
#define MMU_PPN_MASK            ((_PMM_BIT_ORDER(28)) - 1)


static void* alloc_zero(uint_t size, uint_t align)
{
	void* buff;
	buff = (void*)allocate_reserved_mem(size, align);
	memset(buff, 0, size);//Reminder : mmu is still unactivated and PADDR_EXT contains the boot_clluster_leti_id => writing as in right space
	return buff;
}

#if 0
static void pmm_map_huge(uint64_t tty, 
		    uint_t *boot_pgdir,
		    uint_t vaddr,
		    uint32_t phy_start, 
		    uint32_t phy_end, 
		    uint_t arch_cid,
		    uint_t attr)
{
		  uint_t start;
		  uint_t end;

#if CONFIG_MEM_DEBUG
		  boot_dmsg(tty, "\t<0x%x:%x : 0x%x:%x> -> <0x%x : 0x%x>\n",
					 arch_cid, phy_start, arch_cid, phy_end, 
					 vaddr, vaddr + (uint_t)(phy_end - phy_start));
#endif

		  phy_end = ARROUND_UP(phy_end , HUGE_PAGE_SIZE);
		  phy_start = ARROUND_DOWN(phy_start , HUGE_PAGE_SIZE);
		  phy_end = (phy_end == phy_start) ? 
					(phy_end + HUGE_PAGE_SIZE) : phy_end;

		  start   = (uint_t) ( ( (((uint64_t) arch_cid)<<32) | ((uint64_t) phy_start) ) >> PMM_HUGE_PAGE_SHIFT);
		  end   = (uint_t) ( ( (((uint64_t) arch_cid)<<32) | ((uint64_t) phy_end) ) >> PMM_HUGE_PAGE_SHIFT);
		  while(start < end)
		  {  
			 boot_pgdir[MMU_PDE(vaddr)] = (attr ^ PMM_HUGE) | start; 
#if CONFIG_MEM_DEBUG
			 boot_dmsg(tty, "\t\tStart 0x%x, Vaddr 0x%x, pde_idx 0x%x, pde 0x%x\n", 
			 start, vaddr, MMU_PDE(vaddr), boot_pgdir[MMU_PDE(vaddr)]);
#endif

			 vaddr += PMM_HUGE_PAGE_SIZE;
			 start ++;
		  }
}
#endif

static uint_t get_ppn(uint_t addr, uint_t arch_cid)
{
	return (uint_t)( (((uint64_t) arch_cid <<32) | (uint64_t)addr) >> PMM_PAGE_SHIFT );
}

static void pmm_map(uint64_t tty, 
		    uint_t *boot_pgdir,
		    uint_t vaddr,
		    uint32_t phy_start, 
		    uint32_t phy_end, 
		    uint_t arch_cid,
		    uint_t attr)
{
		uint_t current_ppn;
		uint_t end_ppn;
		uint_t pde_val;
		uint_t pte_ppn;
		uint_t *pde;
		uint_t *pte;

		current_ppn = get_ppn(phy_start, arch_cid);
		end_ppn = get_ppn(phy_end, arch_cid);
		end_ppn = (phy_end & PMM_PAGE_MASK)? end_ppn+1 : end_ppn;

		while(current_ppn < end_ppn)
		{
			pde    = &boot_pgdir[MMU_PDE(vaddr)];
			pde_val = (uint_t) *pde;

			if(pde_val == 0)
			{
				pte = (uint_t*) alloc_zero(PMM_PAGE_SIZE, PMM_PAGE_SIZE);
				pte_ppn = get_ppn((uint_t)pte, arch_cid);
				*pde = PMM_PRESENT | MMU_PTD1 | pte_ppn;
			}
			else
			{
				pte_ppn = pde_val & MMU_PPN_MASK;
				//TODO: assert that MSB(pte_ppn << PMM_PAGE_SIZE) is pointing to local ?
				pte = (uint_t*) (pte_ppn << PMM_PAGE_SHIFT);
			}
			
			pte = (uint_t*)((char*) pte + MMU_PTE(vaddr));
			pte[1] = get_ppn(vaddr, arch_cid);
			pte[0] = attr;

			current_ppn++;
			vaddr += PMM_PAGE_SIZE;
#if CONFIG_MEM_DEBUG
			boot_dmsg(tty, "\t<0x%x:%x : 0x%x:%x> -> <0x%x : 0x%x>\n",
					 arch_cid, phy_start, arch_cid, phy_end, 
					 vaddr, vaddr + (uint_t)(phy_end - phy_start));
#endif
		}
		cpu_wbflush();
}

void memory_init(boot_info_t *binfo, kernel_info_t *kinfo, uint64_t tty, uint_t arch_cid)
{
		uint32_t *boot_pgdir;
		uint_t  virtual_end;
		uint_t  virtual_size;
		uint_t  virtual_start;
		uint_t  reserved_size;
		uint_t  physical_mem_size;

		boot_pgdir = (uint32_t*) alloc_zero(PMM_HUGE_PAGE_SIZE, PMM_HUGE_PAGE_SIZE);

		binfo->boot_pgdir = (uint_t) boot_pgdir;

#if CONFIG_MEM_DEBUG  
		boot_dmsg(tty, "\n\tBoot PageDir @0x%x\n", binfo->boot_pgdir);
#endif

//FIXME: confusing virtual and pseudo-physical addresses
//FIXME: no tested
#if WITH_VIRTUAL_ADDR
		  virtual_start = kinfo->text_start;
		  virtual_end  = kinfo->text_start + binfo->cluster_span;
		  virtual_size = binfo->cluster_span;

		  reserved_size = binfo->reserved_end - binfo->reserved_start;
		  physical_mem_size = binfo->reserved_end - binfo->text_start;
		  if(physical_mem_size > virtual_size)
		  {
					 //map reserved. (and check we have enough virtual to map the kernel ?)
					 //TODO check that the allocator is destroyed!
					 virtual_start = virtual_end - reserved_size;
					 pmm_map(tty,
								(uint_t*)boot_pgdir, 
								arch_cid,
								vitual_start, //virtual or pseudo-physical address
								binfo->reserved_start, 
								virtual_end, 
								KATTR);
					 virtual_start = kinfo->text_start;
					 virtual_end = ARROUND_DOWN(virtual_start, HUGE_PAGE_SIZE);
		  }
#else
		  //TODO: check that the virtual map does not exceeds CONFIG_KERNEL_OFFSET
		  //may be it's to the kernel(kern_init) to do it ?
		  virtual_start = kinfo->text_start;
		  virtual_end	= kinfo->virtual_end;
#endif

		  pmm_map(tty,
			 (uint_t*)boot_pgdir, 
			 kinfo->text_start, //virtual address
			 binfo->text_start, //physical start (LSB)
			 virtual_end, //physical end (LSB)
			 arch_cid, //MSB of the physical @es
			 KATTR);

#if WITH_VIRTUAL_ADDR
		  /* The above mapping should also map the boot code.			*
		   * other wise the following check will fail.				*/
		  if(binfo->boot_loader_end > virtual_end)
					 boot_dmsg(tty, "\n\tCID %x: Not enough virtual space(%p) \
					 to map the boot code (%p)!\n", arch_cid, virtual_end, 
					 binfo->boot_loader_end);
#endif

}


