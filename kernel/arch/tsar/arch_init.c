/*
 * arch_init.c - architecture intialization operations (see kern/hal-arch.h)
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

#include <config.h>
#include <types.h>
#include <list.h>
#include <bits.h>
#include <cpu.h>
#include <system.h>
#include <cluster.h>
#include <device.h>
#include <driver.h>
#include <thread.h>
#include <task.h>
#include <kmem.h>
#include <ppm.h>
#include <pmm.h>
#include <page.h>
#include <kdmsg.h>
#include <devfs.h>
#include <drvdb.h>
#include <boot-info.h>
#include <soclib_xicu.h>
#include <soclib_iopic.h>
#define _ARCH_BIB_SIGNATURE_
#include <arch-bib.h>

#define die(args...) do {boot_dmsg(args); while(1);} while(0)

#define ICU_MASK     0xFFFFFFFF  // interrupt enabled for first 32 devices 
#define KATTR     (PMM_HUGE | PMM_READ | PMM_WRITE | PMM_EXECUTE | PMM_CACHED | PMM_GLOBAL | PMM_DIRTY | PMM_ACCESSED)
#define KDEV_ATTR (PMM_READ | PMM_WRITE | PMM_GLOBAL | PMM_DIRTY | PMM_ACCESSED)

#define MSB(paddr) ((uint_t)((paddr)>>32))
#define LSB(paddr) ((uint_t)(paddr))

typedef struct arch_bib_header_s  header_info_t;
typedef struct arch_bib_cluster_s cluster_info_t;
typedef struct arch_bib_device_s  dev_info_t;

/** Set XICU device's mask */
void arch_xicu_set_mask(struct cluster_s *cluster, struct device_s *icu);

/** Find the first device having DriverId drvid */
struct device_s* arch_dev_locate(struct list_entry *devlist_root, uint_t drvid);

bool_t arch_dev_init(struct cluster_s *cluster, struct cluster_entry_s *centry,
				dev_info_t *dev_tbl, cluster_info_t* cluster_ptr);

/** Register all detected devices of given cluster */
void arch_dev_register(struct cluster_s *cluster, 
			struct cluster_entry_s *entry);

void arch_dev_iopic_register(struct cluster_s *cluster, 
			struct cluster_entry_s *entry);

/** HAL-ARCH CPUs & Clusters counters */
static global_t arch_onln_cpus_nr;
static global_t arch_cpus_per_cluster;
static global_t arch_onln_clusters_nr;
static global_t arch_boot_cid_;

/**/
struct arch_entry_s arch_entrys[CLUSTER_NR];
global_t __current_cid;

/** Return Platform CPUs Online Number */
inline uint_t arch_onln_cpu_nr(void)
{
	return arch_onln_cpus_nr.value;
}

/** Return Platform Clsuters Online Number */
inline uint_t arch_onln_cluster_nr(void)
{
	return arch_onln_clusters_nr.value;
}

inline uint_t arch_cpu_per_cluster(void)
{
	return arch_cpus_per_cluster.value;
}

inline cid_t arch_boot_cid(void)
{
	return arch_boot_cid_.value;
}

extern uint_t cpu_gid_tbl[CPU_PER_CLUSTER] CACHELINE;

inline void cpu_gid_tbl_init(struct boot_info_s *info)
{
	uint_t i;
	uint_t cpu_nr;

	cpu_nr = info->local_onln_cpu_nr;

	if(CPU_PER_CLUSTER < cpu_nr)
		while(1);//die("ERROR: This Kernel Is Compiled For only %d cpus per cluster\n", CPU_PER_CLUSTER);

	for(i=0; i < cpu_nr; i++)
		cpu_gid_tbl[i] = arch_cpu_gid(info->local_cluster_id, i); 
}

cid_t Arch_cid_To_Almos_cid[CLUSTER_NR]; //address of info->arch_cid_to_cid array

static void fill_arch_cid_to_almos_array(struct arch_bib_header_s *header)
{
	int i;
	cluster_info_t *cluster_tbl;

	cluster_tbl = (cluster_info_t*)((uint_t)header + sizeof(header_info_t));
	assert(CLUSTER_NR >= header->onln_clstr_nr);

	for(i = 0; i < header->onln_clstr_nr; i++)
	{
		Arch_cid_To_Almos_cid[cluster_tbl[i].arch_cid] = cluster_tbl[i].cid;

	}
}

/** 
 * HAL-ARCH implementation: 
 * Build kernel virtual mapping of all hardware memory banks.
 * Initialize the boot table entries.
 */
void arch_state_init(struct boot_info_s *info)
{
	struct task_s *task;
	struct thread_s *this;
	header_info_t *header;
	cluster_info_t *clusters;
	cluster_info_t *cluster_ptr;
	dev_info_t *dev_tbl;
	uint_t vaddr_start;
	uint_t vaddr_limit;
	uint_t size;
	uint_t cid;
	uint_t i;
	
	header = (header_info_t*) info->arch_info;
	

	clusters        = (cluster_info_t*) ((uint_t)header + sizeof(header_info_t));
	cluster_ptr     = &clusters[info->local_cluster_id];
	dev_tbl		= (dev_info_t*)(cluster_ptr->offset + (uint_t)header);
	cid             = cluster_ptr->cid;

	/* Global variables init */
	arch_onln_clusters_nr.value = header->onln_clstr_nr;
	arch_onln_cpus_nr.value     = header->onln_cpu_nr;
	arch_cpus_per_cluster.value = header->cpu_nr;
	arch_boot_cid_.value	    = info->boot_cluster_id;
	__current_cid.value = cid;
	fill_arch_cid_to_almos_array(header);

	//used by remote_sb->arch_cid_...
	arch_entrys[cid].arch_cid   = cluster_ptr->arch_cid;
	cpu_gid_tbl_init(info);
	kboot_tty_init(info);

	size = dev_tbl[0].size;

	if(size == 0)
                die("[ERROR]\t%s: Unexpected memory size for cluster %u\n",             \
                                __FUNCTION__, cid);

	this                         = current_thread;
	task                         = this->task;

	/* TODO: deal with offline clusters as well */
	for(i = 0; i < info->onln_clstr_nr; i++)
	{
		cluster_ptr = &clusters[i];
		cid = cluster_ptr->cid;
   
		if(cid >= CLUSTER_NR)
		{
			die("\n[ERROR]\t%s: This kernel version support up to %d clusters, found %d\n",         \
                                        __FUNCTION__, CLUSTER_NR, cid);
		}

		dev_tbl = (dev_info_t*)(cluster_ptr->offset + (uint_t)header);
		if(dev_tbl[0].id == SOCLIB_RAM_ID)
		{
			vaddr_start	= dev_tbl[0].base;
			size		= dev_tbl[0].size;
		}else
		{
			vaddr_start = 0;
			size = 0;
		}

		if((cluster_ptr->cpu_nr != 0) && (size == 0))
			die("\n[ERROR]\t%s: This Kernel Version Do Not Support CPU-Only Clusters, cid %d\n",    \
                                        __FUNCTION__, cid);

		vaddr_limit          = vaddr_start + size;

		if(cid == info->local_cluster_id)
		{
			task->vmm.limit_addr         = vaddr_limit; 
		}

		arch_entrys[cid].arch_cid = cluster_ptr->arch_cid;

                boot_dmsg("[INFO]\tHardware initialization of cluster %u\t\t\t\t[ %d ]\n",          \
                                cid, cpu_time_stamp());

		cluster_entry_init(cid, vaddr_start, size);
	}
}

/**
 * HAL-ARCH implementation: Initialize current
 * cluster and dynamicly detect its devices, 
 * associate them to approperiate drivers, 
 * and register them into the kernel.
 */
void arch_init(struct boot_info_s *info)
{
	struct cluster_s *cluster;
	header_info_t *header;
	cluster_info_t *clusters;
	cluster_info_t *cluster_ptr;
	dev_info_t *dev_tbl;
	uint_t iopic_cid;
	bool_t iopic;
	error_t err;
	uint_t rcid;
	uint_t cid;
	uint_t i;
  
	/* Local variables init */
	header      = (header_info_t*) info->arch_info;

	if(strncmp(header->signature, arch_bib_signature, 16))
		while(1);			/* No other way to die !! */

	if(strncmp(header->arch, "SOCLIB-TSAR", 16))
		while(1);//NO TTY yet
  
	iopic = false;
	iopic_cid = 0;
	clusters    = (cluster_info_t*) ((uint_t)header + sizeof(header_info_t));
	cluster_ptr = &clusters[info->local_cluster_id];
	dev_tbl     = (dev_info_t*)(cluster_ptr->offset + (uint_t)header);
	cid         = cluster_ptr->cid;

	arch_state_init(info);

	/* init cluster: also init current_{cid, cluster} macros */
	err = cluster_init(info, 
			   dev_tbl[0].base, 
			   dev_tbl[0].base + dev_tbl[0].size,
			   clusters_tbl[cid].vmem_start);
	
	if(err)
                die("ERROR: Failed To Initialize Cluster %d, Err %d\n", cid, err);

	cluster           = current_cluster;
	//TODO: move it to cluster.c
	//!!! THis value include the io_cluster !
	cluster->clstr_nr = info->onln_clstr_nr; // TODO: headr->x_max * header->y_max;
  
	for(i = 0; i < info->onln_clstr_nr; i++)
	{
		cluster_ptr = &clusters[i];

		dev_tbl     = (dev_info_t*)(cluster_ptr->offset + (uint_t)header);

		rcid = cluster_ptr->cid;

		boot_dmsg("\n[INFO]\tDevices initialization for cluster %d\t\t\t\t[ %d ]\n",    \
                                rcid, cpu_time_stamp());

		if(arch_dev_init(cluster, &clusters_tbl[rcid], dev_tbl, cluster_ptr))
		{
			iopic = true;
			iopic_cid = rcid;
		}
	}

	if(iopic) //&& (cluster->io_clstr == cluster->id)) // Now it's dynamique
		arch_dev_iopic_register(cluster, &clusters_tbl[iopic_cid]);
}

bool_t arch_dev_init(struct cluster_s *cluster, struct cluster_entry_s *centry,
				dev_info_t *dev_tbl, cluster_info_t* cluster_ptr)
{
	struct drvdb_entry_s *entry;
	struct list_entry *devlist;	
	register uint_t dev_nr;
	register uint_t cid;
	register uint_t i;
	struct task_s *task0;
	struct device_s *dev;
	uint64_t dev_base;
	bool_t iopic_clstr;
	driver_t *driver;
	error_t err;
	uint_t size;
	uint_t rcid;
  
	iopic_clstr = false;
	task0	= current_task;
	dev_nr	= cluster_ptr->dev_nr;
	devlist = &centry->devlist;
	rcid	= cluster_ptr->cid;
	cid	= cluster->id;

	for(i=0; i < dev_nr; i++)
	{
	   	if(dev_tbl[i].id == SOCLIB_RAM_ID)
		      continue;

		entry = drvdb_locate_byId(dev_tbl[i].id);

		if((entry == NULL) || (drvdb_entry_get_driver(entry) == NULL))
		{
			boot_dmsg("[WARNING]\tUnknown Device [cid %d, dev %d, base 0x%x, devid %d], Ignored\n", \
				  rcid, i, dev_tbl[i].base, dev_tbl[i].id);
			continue;
		}

		dev_base = dev_tbl[i].base;
		size = dev_tbl[i].size;

	   	if(dev_tbl[i].id == SOCLIB_IOPIC_ID)
		   	iopic_clstr = true;

		if((dev = device_alloc()) == NULL)
			die("[ERROR]\tFailed To Allocate Device [Cluster %d, Dev %d]\n",                        \
                                        cid, i);


		driver		= drvdb_entry_get_driver(entry);
		dev->base	= (void*)(uint_t)(dev_base);//do we need it ?
		dev->cid	= rcid;
		dev->base_paddr	= dev_base;
		dev->irq	= dev_tbl[i].irq;
		dev->iopic	= NULL;

		if((err=driver->init(dev)))
			die("[ERROR]\tFailed To Initialize Device %s [Cluster %d, Dev %d, Err %d]\n",           \
			    drvdb_entry_get_name(entry), rcid, i, err);
      
		devfs_register(dev);
		list_add_last(devlist, &dev->list);

		boot_dmsg("[INFO]\tFound Device: %s\t\t\t\t\t\t[ %d ]\n[INFO]\t\tBase <0x%x> cid %d Size <0x%x> Irq <%d>\t\t[ %d ]\n", \
			  drvdb_entry_get_name(entry), cpu_time_stamp(), (uint_t)dev_base,           \
			  rcid, dev_tbl[i].size, dev_tbl[i].irq, cpu_time_stamp());
	}

		
	arch_dev_register(cluster, centry);

	return iopic_clstr;

}

struct device_s* arch_dev_locate(struct list_entry *devlist_root, uint_t drvid)
{
	struct list_entry *iter;
	struct device_s *dev;
  
	list_foreach(devlist_root, iter)
	{
		dev = list_element(iter, struct device_s, list);
		if(dev->op.drvid == drvid)
			return dev;
	}
  
	return NULL;
}

void arch_dev_register(struct cluster_s *cluster, 
			struct cluster_entry_s *entry)
{
	struct device_s *xicu;
	struct list_entry *iter;
	struct device_s *dev;
	struct cpu_s *cpu_ptr;
	uint_t cpu;
	error_t err;

	xicu = arch_dev_locate(&entry->devlist, SOCLIB_XICU_ID);
	dev  = arch_dev_locate(&entry->devlist, SOCLIB_DMA_ID);

	arch_entrys[entry->cid].xicu = xicu;
	arch_entrys[entry->cid].dma  = dev;
  
	if((cluster->id == entry->cid) && (xicu == NULL))
		die("[ERROR]\tNo XICU is found for cluster %d\n", entry->cid);
		
	//only local cluster get to set the irq
	if(cluster->id != entry->cid)
		return;

	list_foreach((&entry->devlist), iter)
	{
		dev = list_element(iter, struct device_s, list);	

		if((dev != xicu) && (dev->irq != -1))
		{
			err = xicu->op.icu.bind(xicu, dev);
      
			if(err) 
				die("[ERROR]\tFailed to bind device %s, irq %d, on xicu %s @%x [ err %d ]\n",
				    dev->name, dev->irq, xicu->name, xicu, err);
		}

	}
		
	if(cluster->id != entry->cid)
		return;

	arch_xicu_set_mask(cluster, xicu);

	for(cpu = 0; cpu < cluster->cpu_nr; cpu++)
	{
		cpu_ptr = &cluster->cpu_tbl[cpu];
		arch_cpu_set_irq_entry(cpu_ptr, 0, &xicu->action);
	}
}

void arch_xicu_set_wti(struct device_s *xicu, 
			uint_t cpu_inter, uint_t wti_index, 
			struct irq_action_s *action, 
			struct device_s *dev)
{
	uint64_t mailbox;
	uint_t irq_out;
	error_t err;

	mailbox = xicu->base_paddr + (wti_index << 2);
	irq_out = cpu_inter*OUTPUT_IRQ_PER_PROC;


	xicu->op.icu.set_mask(xicu,
				(1 << wti_index),
				XICU_MSK_WTI_ENABLE,
				irq_out);

	boot_dmsg("[INFO]\t\t WTI %d <-> CPU %d (mailbox 0x%x%x)\t\t\t[ %d ]\n",        \
                        wti_index, cpu_inter, MSB(mailbox), LSB(mailbox),               \
                        cpu_time_stamp());

	err = xicu->op.icu.bind_wti(xicu, action, wti_index);

	  
	if(err != 0) 
		die("[ERROR]\tFailed to bind %s via wti_mailbox 0x%x:%x , on xicu %s @%x [ err %d ]\n", \
			dev ? dev->name : "IPI", MSB(mailbox),LSB(mailbox), xicu->name, xicu, err); 
	  
	if(dev)
                dev->mailbox = mailbox;
	//The binding is (to be) done dynamically in the perpherique
	//iopic->op.iopic.bind_wti_and_irq(iopic, mailbox, dev->irq);
			
}

//TODO: distibute iopic devices on multiple cluster ?
//For now all the devices of the IOPIC_Cluster (cluster 
//without cpus) and there IRQs are handled by the IO_Cluster.
void arch_dev_iopic_register(struct cluster_s *cluster, 
			struct cluster_entry_s *entry)
{
	struct list_entry *iter;
	struct device_s *iopic;
	struct device_s *xicu;
	struct device_s *dev;
	uint_t device_index;
	uint_t wti_index;
	uint_t cpu_inter;
	uint_t cpu_nr;
	//error_t err;

	cpu_inter = 0;
	device_index = 0;
	cpu_nr = cluster->cpu_nr;
	wti_index = cpu_nr;//Leave one WTI per proc for IPI

	xicu = arch_entrys[cluster->id].xicu;
	iopic = arch_dev_locate(&entry->devlist, SOCLIB_IOPIC_ID);

	if(xicu == NULL)
                die("[ERROR]\tNo XICU Is Found for IOPIC cluster %d\n",                         \
                                entry->cid);

        boot_dmsg("\n[INFO]\tIOPIC cluster configuration: cid %d <0x%x,0x%x>\t[ %d ]\n",      \
                        entry->cid, iopic->base, (iopic->base+iopic->size), cpu_time_stamp());

	list_foreach(&entry->devlist, iter)
	{
		dev = list_element(iter, struct device_s, list);

		if(wti_index > (IOPIC_WTI_PER_CLSTR - cpu_nr))
			die("[ERROR]\tNo enough WTI in cluster %d\n", cluster->id);
		
			
		if((dev != iopic) && (dev->irq != -1))
		{
			boot_dmsg("[INFO]\tLinking device %s to XICU on cluster %d through IOPIC cluster\t[ %d ]\n",  \
                                        dev->name, xicu->cid, cpu_time_stamp());

			arch_xicu_set_wti(xicu, device_index%cpu_nr, wti_index, &dev->action, dev);

			dev->iopic = iopic;

			wti_index++;
			device_index++;
		}

		devfs_register(dev);
	}
}

void arch_xicu_set_mask(struct cluster_s *cluster, struct device_s *icu)//FG : we changed the interruption policy if 4 clusters
{
	uint_t i;
	uint_t one_every_cpu_nr;
	uint_t cpu_nr;

	cpu_nr = cluster->cpu_nr;//once again we suppose the same number of processor in all clusters
	one_every_cpu_nr = 0;//example : for cpu_nr==4 : one_every_cpu_nr = 0x11111111
	
	for (i = 0; i < 32; i++)
		 if(i % cpu_nr == 0)
			 one_every_cpu_nr |= (1 << i);
	
	//set to zero the mask of unsused irq_out 
	for(i = 0; i < (OUTPUT_IRQ_PER_PROC*cpu_nr); i++)
	{
		if((i % OUTPUT_IRQ_PER_PROC)==0)//if i is first irq of one proc
			continue;
		icu->op.icu.set_mask(icu, 0, XICU_HWI_TYPE, i);
		icu->op.icu.set_mask(icu, 0, XICU_PTI_TYPE, i);
		icu->op.icu.set_mask(icu, 0, XICU_WTI_TYPE, i);
	}

	for(i = 0; i < cpu_nr; i++)
	{
		icu->op.icu.set_mask(icu, (uint_t)(one_every_cpu_nr << i), XICU_HWI_TYPE, i*OUTPUT_IRQ_PER_PROC);
		icu->op.icu.set_mask(icu, 1 << i , XICU_PTI_TYPE, i*OUTPUT_IRQ_PER_PROC);
		icu->op.icu.set_mask(icu, 1 << i , XICU_WTI_TYPE, i*OUTPUT_IRQ_PER_PROC);
	}

#if ARCH_HAS_BARRIERS //TODO
	if(cpu_nr > 1)
	{
		icu->op.icu.set_mask(icu, XICU_CNTR_MASK, XICU_CNTR_TYPE, 1*OUTPUT_IRQ_PER_PROC);
		icu->op.icu.set_mask(icu, 0, XICU_CNTR_TYPE, 0);
	}
	else
	{
		icu->op.icu.set_mask(icu, XICU_CNTR_MASK, XICU_CNTR_TYPE, 0);
		icu->op.icu.set_mask(icu, 0, XICU_CNTR_TYPE, 1*OUTPUT_IRQ_PER_PROC);
	}
  
	icu->op.icu.set_mask(icu, 0, XICU_CNTR_TYPE, 2*OUTPUT_IRQ_PER_PROC);
	icu->op.icu.set_mask(icu, 0, XICU_CNTR_TYPE, 3*OUTPUT_IRQ_PER_PROC);
#endif	/* ARCH_HAS_BARRIERS */
}
#if 0
void arch_xicu_set_mask(struct cluster_s *cluster, struct device_s *icu)
{
	icu->op.icu.set_mask(icu, ICU_MASK, XICU_HWI_TYPE, 0);
	icu->op.icu.set_mask(icu, 0, XICU_HWI_TYPE, 1);
	icu->op.icu.set_mask(icu, 0, XICU_HWI_TYPE, 2);
	icu->op.icu.set_mask(icu, 0, XICU_HWI_TYPE, 3);

#if ARCH_HAS_BARRIERS
	if(cluster->cpu_nr > 1)
	{
		icu->op.icu.set_mask(icu, XICU_CNTR_MASK, XICU_CNTR_TYPE, 1);
		icu->op.icu.set_mask(icu, 0, XICU_CNTR_TYPE, 0);
	}
	else
	{
		icu->op.icu.set_mask(icu, XICU_CNTR_MASK, XICU_CNTR_TYPE, 0);
		icu->op.icu.set_mask(icu, 0, XICU_CNTR_TYPE, 1);
	}
  
	icu->op.icu.set_mask(icu, 0, XICU_CNTR_TYPE, 2);
	icu->op.icu.set_mask(icu, 0, XICU_CNTR_TYPE, 3);
#endif	/* ARCH_HAS_BARRIERS */

	icu->op.icu.set_mask(icu, 1, XICU_PTI_TYPE, 0);
	icu->op.icu.set_mask(icu, 2, XICU_PTI_TYPE, 1);
	icu->op.icu.set_mask(icu, 4, XICU_PTI_TYPE, 2);
	icu->op.icu.set_mask(icu, 8, XICU_PTI_TYPE, 3);
  
	icu->op.icu.set_mask(icu, 1, XICU_WTI_TYPE, 0);
	icu->op.icu.set_mask(icu, 2, XICU_WTI_TYPE, 1);
	icu->op.icu.set_mask(icu, 4, XICU_WTI_TYPE, 2);
	icu->op.icu.set_mask(icu, 8, XICU_WTI_TYPE, 3);
}
#endif
