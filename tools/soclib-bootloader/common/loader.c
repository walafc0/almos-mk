
/*  This file is part of AlmOS.
  
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

/* FG
 * Modifications :
 *    -réveil des porcesseurs par une WTI kern_init.c
 *    rq : CONFIG_BOOT_SIGNAL_SIGNATURE a priori désormais inutile
 *    -on place le bootloader avant le secteur réservé du kernel (avant le kernel) 
 *    a partir de 0x00000000 (physique) -> evite que le code soit efface alors
 *    que les autres procs l'utilise pour leur utilisation
 *    - nouveau champ dans info
 *
 *    TODO : supprimer le champ cluster_nr dans info : redondant avec x_max et y_max
 */ 

///////////////////////////////////////////
//            SYSTEM STARTUP             //
///////////////////////////////////////////


#include <segmentation.h>
#include <boot-info.h>
#include <config.h>
#include <string.h>
#include <mmu.h>
#include <dmsg.h>
#include <bits.h>

#define _ARCH_BIB_SIGNATURE_
#include <arch-bib.h>

#define DEBUG           no
#define MSB(paddr)      ((uint_t)((paddr)>>32))
#define LSB(paddr)      ((uint_t)(paddr))

static void *boot_dma_memcpy(void *dest, void *src, 
			uint_t arch_cid_dst, uint_t arch_cid_src, 
			unsigned long size);

static void *boot_cpu_memcpy(void *dest, void *src, 
			uint64_t arch_cid_dst, uint64_t arch_cid_src, 
			unsigned long size);

//TODO
#define assert(a) /**/

//Boot loader variables
extern uint_t __kernel_base;
extern uint_t __kernel_end;
extern uint_t __boot_base;
extern uint_t __boot_end;
extern uint_t __boot_loader_base;
extern uint_t __boot_loader_end;
extern uint_t __boot_info_block_base;
extern uint_t __boot_info_block_end;

//WRC?: the following global variable are at the same time virtual addresses and 
//physical adrreses when extended with arch_cid of boot cluster
unsigned int arch_bib_base	= (uint_t) &__boot_info_block_base;
unsigned int arch_bib_end	= (uint_t) &__boot_info_block_end;
unsigned int boot_base		= (uint_t) &__boot_base;
unsigned int boot_end		= (uint_t) &__boot_end;
unsigned int boot_loader_base	= (uint_t) &__boot_loader_base;
unsigned int boot_loader_end	= (uint_t) &__boot_loader_end;
unsigned int kernel_img_base	= (uint_t) &__kernel_base;
unsigned int kernel_img_end	= (uint_t) &__kernel_end;

//for boot_printd
uint64_t boot_tty;
uint64_t dma_base;
#define boot_printd(...) boot_dmsg(boot_tty, __VA_ARGS__)

/* There is no lock on boot_dmsg : if DEBUG is set to yes, tty is a mess.. */
#if DEBUG
#define other_boot_printd(...) ({if(arch_cid == 0x10) boot_dmsg(boot_tty, __VA_ARGS__);})
#else
#define other_boot_printd(...)
#endif
//used by boot.c
uint32_t boot_tbl_base;
unsigned int boot_arch_cid;

/* There's one main proc per cluster */
#define MAIN_PROC 0

typedef struct boot_signal_block_s bsb_t;
typedef void (kernel_entry_t)(boot_info_t *info);
typedef struct arch_bib_header_s  header_info_t;
typedef struct arch_bib_cluster_s cluster_info_t;
typedef struct arch_bib_device_s  dev_info_t;

#define MAX_CLUSTER 256
uint8_t	Arch_cid_To_Almos_cid[MAX_CLUSTER]; //address of info->arch_cid_to_cid array
unsigned int Arch_cid_To_Almos_cid_base;


volatile uint_t sync_global;//used by the boot proc, to synchronise with other proc that are not local
volatile uint_t sync_local;//used by the main procs, to synchronise with local proc
volatile uint_t boot_proc_kernel_ready;

//should support the execution in both physical and virtual
static void boot_signal_op(const struct boot_info_s *info)
{
	mmu_physical_atomic_inc((uint_t) &sync_global, (uint_t) info->data);

	while(!mmu_physical_cluster_rw((uint_t) &boot_proc_kernel_ready, (uint_t) info->data))
		cpu_rbflush();
}

static void local_boot_signal_op(struct boot_info_s *info)
{
	mmu_physical_atomic_inc((uint_t) &sync_local, (uint_t) info->data);
}

static uint_t get_nb_cluster(struct arch_bib_header_s *header)
{
	return header->onln_clstr_nr;
}

static void fill_arch_cid_to_almos_array(struct arch_bib_header_s *header)
{
	int i;
	boot_tbl_entry_t *boot_tbl;
	cluster_info_t   *cluster_tbl;
	dev_info_t       *dev_tbl;

	assert(MAX_CLUSTER >= get_nb_cluster(header));

	cluster_tbl = (cluster_info_t*)((uint_t)header + sizeof(header_info_t));

	for(i = 0; i < get_nb_cluster(header); i++)
		Arch_cid_To_Almos_cid[cluster_tbl[i].arch_cid] = cluster_tbl[i].cid;

	Arch_cid_To_Almos_cid_base = (uint_t) &Arch_cid_To_Almos_cid[0];
	cpu_wbflush();
}

static uint_t get_almos_cid(uint_t arch_cid)
{
	return Arch_cid_To_Almos_cid[arch_cid];
}


/* Only the main procs are allowed to use this (sequential) allocator */
uint_t reserved_mem_base;
uint_t reserved_mem_end;
uint_t rmi=0;
//this allocator allocate by soustracting memory from init
//end is smaller than end
static void init_reserved_mem(uint_t init, uint_t end)
{
	//memory for the stacks
	reserved_mem_base = init;
	reserved_mem_end = end;
	rmi=1;
}

uint_t allocate_reserved_mem(uint_t size, uint_t align)
{
	uint_t new_reserved_base;
	new_reserved_base = reserved_mem_base - size;
	new_reserved_base = ARROUND_DOWN(new_reserved_base, align);

	if(rmi && (new_reserved_base >= reserved_mem_end))
	{
		reserved_mem_base = new_reserved_base;
		return new_reserved_base;
	}
        else
        {
		boot_printd("[ERROR]\tNo more resersed memory!\n",                      \
                                __FUNCTION__);
		while(1);
	}
}

uint_t destroy_reserved_mem()
{
	rmi=0;
	return reserved_mem_base;
}

static void construct_virtual_space(boot_info_t *info, kernel_info_t *kinfo, uint64_t tty, uint_t arch_cid)
{
		memory_init(info, kinfo, tty, arch_cid);
		info->reserved_start = destroy_reserved_mem();
                /* From this point no more allocation is allowed */
}

static void wakeup_main_procs(struct arch_bib_header_s *header, uint_t boot_cid, uint_t func)
{
	int             i;
	uint64_t        xicu_base;
	cluster_info_t  *cluster_tbl;
	dev_info_t      *dev_tbl;

	cluster_tbl = (cluster_info_t*) ((uint_t)header + sizeof(header_info_t));

	for(i = 0; i < get_nb_cluster(header); i++)
	{
		dev_tbl = (dev_info_t*)(cluster_tbl[i].offset + (uint_t)header);

                /* Don't wakeup ourself */
		if(cluster_tbl[i].cid == boot_cid)
			continue;
                /* Dont wake up empties clusters */
		if(cluster_tbl[i].cpu_nr == 0)
			continue;
                /* Don't wake a cluster without device */
		if(dev_tbl[0].size == 0)        // FG
		{
                        boot_printd("[ERROR]\t%s: This kernel version does not support cpu-only clusters\n",    \
                                        __FUNCTION__);
			while(1);
		}
		xicu_base = dev_tbl[XICU_INDEX].base;   /* Physical address on 64 bits */

                boot_printd("[INFO]\t\tWaking up logical cluster %d\n",                 \
                                cluster_tbl[i].cid);

		mmu_physical_sw((xicu_base+(MAIN_PROC<<2)), func);
	}
}

static void wakeup_local_procs(struct arch_bib_header_s *header, uint_t cid, uint_t lpid, uint_t func)
{
	int             i;
	uint64_t        xicu_base;
	cluster_info_t  *cluster_tbl;
	dev_info_t      *dev_tbl;

	cluster_tbl     = (cluster_info_t*) ((uint_t)header + sizeof(header_info_t));
	dev_tbl         = (dev_info_t*)(cluster_tbl[cid].offset + (uint_t)header);

	assert(cluster_tbl[cid].cpu_nr != 0)

	if(cluster_tbl[cid].cpu_nr == 0)
	{
                boot_printd("[ERROR]\t%s: There is no cpu in cluster %d\n",             \
                                __FUNCTION__, cid);
		while(1);
	}

	if(dev_tbl[0].size == 0)        // FG
	{
                boot_printd("[ERROR]\t%s: This kernel version does not support cpu-only clusters\n",    \
                                __FUNCTION__);
		while(1);
	}

	for(i = 0; i < cluster_tbl[cid].cpu_nr; i++)
	{
		if(i == lpid)
                        continue;
		xicu_base = dev_tbl[XICU_INDEX].base;   /* Physical address on 64 bits */
		mmu_physical_sw((xicu_base+(i<<2)), func);
	}
}

static uint_t get_cluster_cpu_nr(struct arch_bib_header_s * header, uint_t cid)
{
	cluster_info_t *clusters;
	clusters = (cluster_info_t*) ((uint_t)header + sizeof(header_info_t));
	return clusters[cid].cpu_nr;
}

static uint_t get_cluster_span(struct arch_bib_header_s * header)
{
		uint_t cluster_span;
		uint_t per_cluster_virtual_space;

#if WITH_VIRTUAL_ADDR
                /* There is one page table per cluster and each table is independant */
		int per_cluster_virtual_space = 
			ARROUND_DOWN(KERNEL_VIRTUAL_SPACE_SIZE, HUGE_PAGE_SIZE);

		cluster_span = 
			(per_cluster_virtual_space < header->rsrvd_limit) ? 
			per_cluster_virtual_space : header->rsrvd_limit;

#else
		cluster_span = header->rsrvd_limit;
#endif
	return cluster_span;
}

uint_t copy_header_to_reserved(struct arch_bib_header_s* header, uint_t arch_cid)
{
	uint_t bib_size   = arch_bib_end - arch_bib_base;
	uint_t new_header = allocate_reserved_mem(bib_size, 0x1000);

	boot_cpu_memcpy((void*) new_header, (void*) header, arch_cid, arch_cid, bib_size);

	return new_header;
}

/* dev_tbl[0] must be a description of the RAM device ! */
static void fill_boot_tbl(struct arch_bib_header_s *header)
{
        uint_t i;
	uint_t cid;
	cluster_info_t   *cluster_tbl;
	boot_tbl_entry_t *boot_tbl;
	dev_info_t       *dev_tbl;

	cluster_tbl = (cluster_info_t*)((uint_t)header + sizeof(header_info_t));
	boot_tbl    = (boot_tbl_entry_t*) allocate_reserved_mem(sizeof(boot_tbl_entry_t) * \
                        get_nb_cluster(header), 4);

        /* All clusters means all clusters with at least one cpu */
        boot_printd("[INFO]\tInitialize boot table for all clusters (%d)\n",            \
                        get_nb_cluster(header)-1);      /* -1: don't init the IOPIC cluster */

	for(i = 0; i < get_nb_cluster(header); i++)
	{
		cid     = cluster_tbl[i].cid;
		dev_tbl = (dev_info_t*)(cluster_tbl[i].offset + (uint_t)header);

                /* Don't initialize empty clusters */
		if(cluster_tbl[i].cpu_nr == 0)
			 continue;

                /* Check if the bootstraping cluster has a least one device (François Guerret) */
		if(dev_tbl[0].size == 0)
		{
                        boot_printd("[ERROR]\t%s: This kernel version do not support cpu-only clusters\n",     \
                                        __FUNCTION__);
			while(1);
		}

		boot_printd("[INFO]\tCluster %d has %d cpu and %d devices. ",            \
                                cid, cluster_tbl[i].cpu_nr, cluster_tbl[i].dev_nr);

		boot_tbl[cid].reserved_start = dev_tbl[0].base; 
		boot_tbl[cid].reserved_end   = dev_tbl[0].base + dev_tbl[0].size;

		boot_printd("Range for cluster %d : <0x%x,0x%x>\n",                     \
                                cid, boot_tbl[cid].reserved_start,                      \
                                boot_tbl[cid].reserved_end);
	}

	boot_tbl_base = (uint32_t) boot_tbl;
	cpu_wbflush();
}


void wait_on(volatile uint_t *wait)
{
	int i;
	do
	{      
		for(i=0; i < 0X1000; i++)
			cpu_rbflush();	/* A better wait ? */
	}while( *(wait) == 0 );
}

void kernel_failed()
{
	while(1);
}

/* For boot proc */
boot_info_t *boot_info_base;

static void boot_loader(int);

void boot_entry(int arch_cpu_id)
{
	boot_loader(arch_cpu_id);
}

volatile int print_lock = 0;
unsigned __physical_cas(unsigned lsb, unsigned msb, unsigned old, unsigned new);
static void boot_loader(int cpu_arch_id)
{
	boot_info_t *info;
	struct arch_bib_header_s *header;
	kernel_info_t *kinfo;
	uint_t load_kernel_base;
	uint_t cid;
	uint_t cpu_lid;
	uint_t cpu_nr;
	uint_t arch_cid;
	uint_t boo_arch_cid;
	uint_t cluster_span;
	uint_t kernel_base;
	uint_t local_cpu_nr;//FG
	uint_t onln_cpu_nr;
	uint_t cluster_nr;

  
	header = (struct arch_bib_header_s*) arch_bib_base;
	kinfo  = (kernel_info_t*) (kernel_img_end - sizeof (kernel_info_t));

	if(strcmp(header->signature, arch_bib_signature))
		while(1);			/* no other way to die !! */	

#if DEBUG
	cpu_set_ebase((uint_t) kernel_failed);
#endif

        /* Some common variables */
        local_cpu_nr = header->cpu_nr;
        cpu_arch_id  = cpu_get_arch_id();
        onln_cpu_nr  = header->onln_cpu_nr;
        arch_cid     = cpu_arch_id / local_cpu_nr;
        cpu_lid	     = cpu_arch_id % local_cpu_nr;
        cluster_nr   = get_nb_cluster(header);
        load_kernel_base = header->rsrvd_start;

	/* boot_tty must be set before printing */
	boot_tty = header->bootstrap_tty;
	dma_base = header->bootstrap_dma;

	if((kinfo->text_start != header->rsrvd_start) || (load_kernel_base != 0))
	{
		boot_printd("This kernel (%p) and boot memc base (%p) \
		addresses must correspond to zero\n", 
		kinfo->text_start, header->rsrvd_start);
		
	}

	if(cpu_arch_id == header->arch_bootstrap_cpu)
	{    
		if(kinfo->signature != KERNEL_SIGNATURE)
		{
			boot_printd("Wrong kernel signature!\n");
			while(1);
		}

                boot_printd("\nAlmos-MK Bootloader\t\t\t\t\t\t\t[ STARTED ]\n\n");
                boot_printd("[INFO]\tBooting from cpu %d on cluster %d\n",              \
                                cpu_arch_id, header->arch_bootstrap_cpu);

		/* Gloabl variable: used by other proc */
		boot_arch_cid = arch_cid;
		cpu_wbflush();

		/* cluster_span */
		cluster_span = get_cluster_span(header);

		/*************** Init mem reserved allocator ********************/
		init_reserved_mem(header->rsrvd_limit - (CONFIG_BOOT_STACK_SIZE *       \
                                        local_cpu_nr), boot_loader_end);        /* FIXME: Better end limit ? */

		/*****	Setting boot_tbl for other proc (also do some printing) **/
		fill_boot_tbl(header);
		
		/******** Setting Arch_cid_To_Almos_cid for other proc *******/
		fill_arch_cid_to_almos_array(header);
		cid = get_almos_cid(arch_cid);
		boot_proc_kernel_ready = 0;
                cpu_wbflush();


		/******************* setting boot_info ************************/
                boot_printd("[INFO]\tLoading Boot Information Block (.bib file)\n");

		info = (boot_info_t*) allocate_reserved_mem(sizeof(boot_info_t), 4);
		boot_info_base = info;                          /* For local procs */

                info->text_start        = load_kernel_base;     // ?
                info->text_end          = kinfo->text_end;
                info->data_start        = kinfo->data_start;
                info->data_end          = kinfo->data_end;
                info->boot_loader_start = boot_loader_base;
                info->boot_loader_end   = boot_loader_end;
                info->brom_start        = CONFIG_BROM_START;    // FG ?
                info->brom_end          = CONFIG_BROM_END;      // FG ?

                info->reserved_end      = header->rsrvd_limit;
                info->onln_cpu_nr       = header->onln_cpu_nr;
                info->onln_clstr_nr     = header->onln_clstr_nr;

                info->local_cpu_nr      = local_cpu_nr;
                info->local_onln_cpu_nr = local_cpu_nr;
                info->local_cpu_id      = cpu_lid;
                info->local_cluster_id  = cid;

                info->boot_cluster_id   = cid;
                info->boot_cpu_id       = header->bootstrap_cpu;
                info->main_cpu_local_id = MAIN_PROC;
                info->io_cluster_id     = header->iopic_cluster;

                /* The boot proc does not need to execute any func when he      *
                 * reach the kernel everybody will be in                        */
                info->boot_signal       = NULL;
                info->data              = NULL;

		/* FIXME FG: Updating tty_vbase in boot_info: this value is     *
                 * then used by kdmsg_init                                      */
                info->tty_base = LSB(boot_tty); 
                info->tty_cid  = get_almos_cid(MSB(boot_tty));

                info->cluster_span      = cluster_span;
                info->boot_pgdir        = 0; /* Set by construct_virtual_space func */
                info->arch_info         = copy_header_to_reserved(header, arch_cid);

                boot_printd("[INFO]\tHeader was copied in the boot_info_s structure\n");

		/* Preparing virtual space. The table will be used by local     *
		 * procs. This will set info->boot_pgdir                        */
                boot_printd("[INFO]\tPreparing kernel's memory layout\n");
		construct_virtual_space(info, kinfo, boot_tty, arch_cid);

		/****** Wake proc and wait until they get to the kernel ********/		
                boot_printd("[INFO]\tWaking up other clusters main cpu (%d)\n",         \
                                header->with_cpu_clstr_nr-1);
		sync_global = 0;
                cpu_wbflush();
		wakeup_main_procs(header, cid, (uint_t) other_proc_reset);

                /* FIXME: Change to passive waiting ? */
		while(sync_global != (header->with_cpu_clstr_nr - 1))
			cpu_rbflush();

		/* Now wake up our local cpus (get them out of the preloader,   *
		 * before he get erased below). They will come to the boot      *
		 * code and wait for the kernel to be loaded.		        */
                boot_printd("[INFO]\tWaking up the local cluster cpu (%d)\n", local_cpu_nr-1);
		wakeup_local_procs(header, cid, cpu_lid, (uint_t) boot_cluster_reset);

		while(sync_local != (local_cpu_nr - 1))/* -1: don't wait for this proc */
			cpu_rbflush();

		/* Now we can move our local kernel. This step will erase the   *
                 * preloader and potentially the kernel but not the boot code.  */
                boot_printd("[INFO]\tLoading kernel image (size = %x)\n",               \
                                kernel_img_end - kernel_img_base);
		boot_dma_memcpy((void*) load_kernel_base, (void*) kernel_img_base, 
					arch_cid, arch_cid, kernel_img_end - kernel_img_base);

		/* Now the kernel is set. Be sure all local procs are. */
		sync_local = 0;
		cpu_wbflush();
		boot_proc_kernel_ready = 1;
		cpu_wbflush();
		while(sync_local != (local_cpu_nr-1))//wait that they reach the kernel	
			cpu_rbflush();
		
#if DEBUG
		boot_printd("Boot Info:\n\tArch Info %x\n\tText <0x%x:%x : 0x%x:%x>\n\tData <0x%x:%x : 0x%x:%x>\n\tReserved <0x%x : 0x%x>\n\tBSC %d, BSP %d, CPU NR %d, MMU OFF\n",
			  info->arch_info,
			  arch_cid, info->text_start,
			  arch_cid, info->text_end,
			  arch_cid, info->data_start,
			  arch_cid, info->data_end,
			  info->reserved_start,
			  info->reserved_end,
			  info->boot_cluster_id, info->boot_cpu_id, info->onln_cpu_nr
			  );
#endif

		/* Set the kernel ebase: is this addr safe in all case ? */
		cpu_set_ebase(load_kernel_base);        // FG MLK

#if WITH_VIRTUAL_ADDR
                /* info is a virtual address -> ok thanks to pseudo identity    *
                 * mapping in memory init : virt (info) -> phy (x|y|info)       */
		mmu_activate(info->boot_pgdir, kinfo->kern_init, (uint_t)info);
#else
                boot_printd("[INFO]\tKernel entry point : 0x%x\n", kinfo->kern_init);
                boot_printd("\nAlmos-MK Bootloader\t\t\t\t\t\t\t[  ENDED  ]\n\n");
                /* Jump to kernel code */
		cpu_goto_entry(kinfo->kern_init, (uint_t)info);
#endif

	}else 
	{
#if DEBUG
		cpu_set_ebase((uint_t) kernel_failed);
#endif
		/* Wait for the kernel code to be set */
		mmu_physical_atomic_inc((uint_t) &sync_local, (uint_t) arch_cid);
		wait_on(&boot_proc_kernel_ready);//sleep
		/* Wakeup signaling to the main proc will be done by the kernel *
                 * using info->boot_signal!                                     */

		/* once we are waked up this mean that the kernel has been      *
                 * copied: jump to it. But first we need to set the boot_info   */
		boot_info_t binfo;              /* allocated on stack */
		boot_info_t *info = &binfo;

		/* Copy the boot info */
		boot_cpu_memcpy(info, boot_info_base, arch_cid, 
					arch_cid, sizeof(*info));
		/* Fill fields */
		info->local_cpu_id = cpu_lid;
		info->boot_signal  = (boot_signal_t *) &local_boot_signal_op;
		info->data          = (void*) boot_arch_cid;
		/* Other fields are similar to the boot proc */

                /* Set the kernel ebase */
		cpu_set_ebase(load_kernel_base);        // FG & MLK
#if WITH_VIRTUAL_ADDR
                /* info is a virtual address -> ok thanks to pseudo identity    *
                 * mapping in memory init : virt (info) -> phy (x|y|info)       */
		mmu_activate(info->boot_pgdir, kinfo->kern_init, (uint_t)info);
#else
		cpu_goto_entry(kinfo->kern_init, (uint_t)info);
#endif

	}
	boot_printd("[ERROR]\tThis code should not be reached %s:%d\n", __FILE__,__LINE__);
		while(1);
}

static void other_cluster_boot_loader(int boot_arch_cid, int arch_cid, int cpu_lid);
void other_cluster_boot_entry(int boot_arch_cid, int arch_cid, int cpu_lid)
{
	other_cluster_boot_loader(boot_arch_cid, arch_cid, cpu_lid);
}

#define ONE_OTHER_CLUSTER 1
static void other_cluster_boot_loader(int boot_arch_cid, int arch_cid, int cpu_lid)
{
	struct arch_bib_header_s *header;
	uint_t load_kernel_base;
	kernel_info_t *kinfo;
	uint_t cluster_span;
	uint_t local_cpu_nr;
	uint_t cpu_arch_id;
	boot_info_t *info;
	uint_t cid;

	cpu_arch_id = cpu_get_arch_id();

	/* common variables: reading/writing to global variables is forbidden here      *
	* since this code will be executed by main procs which have their DPAADR_EXT    *
	* pointing to their local cluster (done to access the stack) while there is     *
	* no boot code/data in the cluster. However, the INST_PADDR_EXT point to        *
	* boot cluster, so we can access the boot code.				        */

	/* Some assumption: all cluster have the same mem(start and end),               *
	 * the same number of proc and ?. Otherwise one need to lookup for              *
	 * theses information in the header cluster_tbl/dev_tbl	and ...?                */

	/* Here the data_paddr_ext register point to the local cluster! */
	if(cpu_lid == MAIN_PROC)
	{
		/* Copy boot code+BIB (at pseudo identity) */
		uint_t bb  = mmu_physical_cluster_rw((uint_t) &boot_base, boot_arch_cid);
		uint_t ble = mmu_physical_cluster_rw((uint_t) &boot_loader_end, boot_arch_cid);

		boot_cpu_memcpy((void*) bb, (void*) bb, arch_cid, 
					boot_arch_cid, ble - bb);
	
		/*+++++++++++++ Now, it's safe to use gloable varibles ++++++++++++++++*/
		other_boot_printd("[%x] Loading Boot code done : \
				cp from 0x%x:0x%x to 0x%x:0x%x, size = %x\n", 
				cpu_arch_id, boot_arch_cid, bb, arch_cid, bb, 
				ble - bb);
		/******* Switch to using the text of the boot code just copied ***/
		cpu_set_physical_ispace(arch_cid);
#if DEBUG
		cpu_set_ebase((uint_t) kernel_failed);
#endif

		/************************** Some variables ***********************/
		header          = (struct arch_bib_header_s*) arch_bib_base;
		cid             = get_almos_cid(arch_cid);
		cluster_span    = get_cluster_span(header);
		local_cpu_nr    = get_cluster_cpu_nr(header, cid);
		
		other_boot_printd("[%x]Booting with arch_cid %p, cpu_lid %p, cid %d, cpu_nr %d\n",
				cpu_arch_id, arch_cid, cpu_lid, cid, local_cpu_nr);

		/*************** Init mem reserved allocator ********************/
		init_reserved_mem(header->rsrvd_limit - (CONFIG_BOOT_STACK_SIZE *       \
                                        local_cpu_nr), boot_loader_end);

		/************* Copy the boot info from boot proc *********/
		info = (boot_info_t*)allocate_reserved_mem(sizeof(boot_info_t), 4);
		other_boot_printd("[%x]Loading Boot Info : cp from 0x%x:0x%x to 0x%x:0x%x, size = %x\n", 
				cpu_arch_id, boot_arch_cid, boot_info_base, arch_cid, info, 
				sizeof(*info));
		//info must be in reserved: the boot code will be erased by the kernel
		boot_cpu_memcpy((void*) info, (void*) boot_info_base,                   \
					arch_cid, boot_arch_cid, sizeof(*info));

                /* For this cluster local procs   */
		boot_info_base = info;
                /* Same load address for all proc */
		load_kernel_base = info->text_start;
		cpu_wbflush();


		/****************** Copy the kernel (to addr 0) ********************/
		other_boot_printd("[%x]Loading Kernel Image : cp from 0x%x:0x%x to 0x%x:0x%x, size = %x\n", \
                                cpu_arch_id, boot_arch_cid, kernel_img_base, arch_cid,  \
                                load_kernel_base, kernel_img_end - kernel_img_base);

                /* FIXME: use local dma ? */
		boot_cpu_memcpy((void*)load_kernel_base, (void*)kernel_img_base,        \
				arch_cid, boot_arch_cid,                                \
                                kernel_img_end - kernel_img_base);

		kinfo = (kernel_info_t*) info->data_end;

		/************************* setting boot info *************************/
                 /* FIXME? all cluster have the same mem ? */
                info->reserved_end      = header->rsrvd_limit;
                info->local_cpu_id      = cpu_lid;
                info->local_cluster_id  = cid;

                info->arch_info         = copy_header_to_reserved(header, arch_cid);

                info->boot_signal       = (boot_signal_t *)&boot_signal_op;
                info->data              = (void*) boot_arch_cid;

		other_boot_printd("\t\t[ OK ]\n boot_tty base %x, cid %d\n", 
			info->tty_base, info->tty_cid);

                info->boot_pgdir        = 0; /* Set by construct_virtual_space func */

		/* Preparing virtual space. The table will be used by local     *
		 * procs. This will set info->boot_pgdir and info->reserved_end */
		construct_virtual_space(info, kinfo, boot_tty, arch_cid);
	
		/* Wake up other proc of his cluster */
		sync_local = 0;
		cpu_wbflush();
                /* They will come to the boot code and wait */
		wakeup_local_procs(header, cid, cpu_lid, (uint_t)other_proc_reset);

		while(sync_local != (local_cpu_nr - 1)) /* -1: don't wait for this proc */
			cpu_rbflush();

		/* Set the kernel ebase: is this addr safe in all case ? */
		cpu_set_ebase(load_kernel_base);        // FG MLK

		other_boot_printd("Boot Info:\n\tArch Info %x\n\tText <0x%x:%x : 0x%x:%x>\n\tData <0x%x:%x : 0x%x:%x>\n\tReserved <0x%x : 0x%x>\n\tBSC %d, BSP %d, CPU NR %d, MMU OFF\n",
			  info->arch_info,
			  arch_cid, info->text_start,
			  arch_cid, info->text_end,
			  arch_cid, info->data_start,
			  arch_cid, info->data_end,
			  info->reserved_start,
			  info->reserved_end,
			  info->boot_cluster_id, info->boot_cpu_id, info->onln_cpu_nr
			  );

		if(kinfo->signature != KERNEL_SIGNATURE)
		{
			other_boot_printd("[ERROR]\t%s: Wrong kernel signature!\n",     \
                                        __FUNCTION__);
			while(1);
		}

#if WITH_VIRTUAL_ADDR
                /* info is a virtual address -> ok thanks to pseudo identity    *
                 * mapping in memory init : virt (info) -> phy (x|y|info)       */
		mmu_activate(info->boot_pgdir, kinfo->kern_init, (uint_t)info);
#else
		other_boot_printd("[INFO]\tKernel Entry Point @0x%x\n",                 \
                                kinfo->kern_init);
		cpu_goto_entry(kinfo->kern_init, (uint_t)info); 
#endif
	}
        else
	{	
		boot_info_t binfo;      /* Allocate on stack ! */
		boot_info_t *info = &binfo;

		/* use the new boot code copied by the main proc by switching   *
                 * to this cluster ispace                                       */
		cpu_set_physical_ispace(arch_cid);
#if DEBUG
		cpu_set_ebase((uint_t) kernel_failed);
#endif

		
		header  = (struct arch_bib_header_s*) arch_bib_base;
		cid     = get_almos_cid(arch_cid);

		if(cpu_lid == 1)
                        other_boot_printd("[%x]booting arch_cid %p, cpu_lid %p, cid %d\n",      \
                                        cpu_arch_id, arch_cid, cpu_lid, cid);
		if(cpu_lid == 1)
                        other_boot_printd("[%x]Loading Boot Info : cp from 0x%x:0x%x to 0x%x:0x%x, size = %x\n",    \
                                        cpu_arch_id, arch_cid, boot_info_base,          \
                                        arch_cid, info, sizeof(*info));

		/* Copy boot_info from this cluster main proc */
		boot_cpu_memcpy((void*) info, (void*) boot_info_base,                   \
					arch_cid, arch_cid, sizeof(*info));
		

		info->local_cpu_id      = cpu_lid;
		info->data              = (void*) arch_cid;
		info->boot_signal       = (boot_signal_t *)&local_boot_signal_op;
		load_kernel_base        = info->text_start;

		kinfo                   = (kernel_info_t*) info->data_end;

		cpu_set_ebase(load_kernel_base);        // FG

#if WITH_VIRTUAL_ADDR
                /* info is a virtual address -> ok thanks to pseudo identity    *
                 * mapping in memory init : virt (info) -> phy (x|y|info)       */
		mmu_activate(info->boot_pgdir, kinfo->kern_init, (uint_t)info);
#else
		cpu_goto_entry(kinfo->kern_init, (uint_t)info); 
#endif
	}
	other_boot_printd("This code should not be reached %s:%d\n", __FILE__,__LINE__);
		while(1);
}


static void *boot_cpu_memcpy(void *dest, void *src, 
			uint64_t arch_cid_dst, uint64_t arch_cid_src, 
			unsigned long size)
{
	register uint_t i = 0;
	register uint_t isize;
	uint64_t pdest = (uint64_t)(uint_t) dest;
	uint64_t psrc  = (uint64_t)(uint_t) src;

	isize = size >> 2;
	pdest = ((arch_cid_dst << 32) | pdest);
	psrc  = ((arch_cid_src << 32) | psrc);

	for(i=0; i< isize; i++)
  		mmu_physical_sw((pdest + (i<<2)), 
			mmu_physical_rw(psrc + (i<<2)) );

  	for(i= isize << 2; i< size; i++)
  		mmu_physical_sb((pdest + i), 
			mmu_physical_rb(psrc + i));

	return dest;
}

/* This function will use the dma pointed by PADDR_EXTEND register */
static void *boot_dma_memcpy(void *dest, void *src, 
			uint_t arch_cid_dst, uint_t arch_cid_src, 
			unsigned long size)
{

#if CONFIG_BOOT_USE_DMA
	register uint_t i       = 0;
	volatile uint_t cntr    = 0;
	
  	mmu_physical_sw((dma_base +  (DMA_RESET_REG*4)), 0);
  	mmu_physical_sw((dma_base +  (DMA_SRC_REG*4)), (uint_t) src);
  	mmu_physical_sw((dma_base +  (DMA_DST_REG*4)), (uint_t) dest);
  	mmu_physical_sw((dma_base +  (DMA_SRC_EXT_REG*4)), arch_cid_src);
  	mmu_physical_sw((dma_base +  (DMA_DST_EXT_REG*4)), arch_cid_dst);
  	mmu_physical_sw((dma_base +  ( (DMA_IRQ_DISABLED*4)*4)), 1);
  	mmu_physical_sw((dma_base +  (DMA_LEN_REG*4)), size);

	do
	{      
		for(i=0; i < size/CONFIG_CACHE_LINE_SIZE; i++)
			cntr++;	/* Wait */
	}while(mmu_physical_rw((dma_base + (DMA_LEN_REG*4))) != 0);
 
#else
	boot_cpu_memcpy(dest, src, arch_cid_dst, arch_cid_src, size);

#endif	/* CONFIG_BOOT_USE_DMA */

	return dest;
}
