/*
 * kern/cluster.c - Cluster-Manger related operations
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
#include <errno.h>
#include <spinlock.h>
#include <cpu.h>
#include <scheduler.h>
#include <list.h>
#include <cluster.h>
#include <atomic.h>
#include <sysfs.h>
#include <boot-info.h>
#include <bits.h>
#include <pmm.h>
#include <thread.h>
#include <kmem.h>
#include <task.h>
#include <dqdt.h>

struct cluster_entry_s clusters_tbl[CLUSTER_NR];
struct cluster_s cluster_manager;//TODO: align the field on a cache line

/* Note: this field may induce false sharing between	*
 * procs (mostly important in MESI-like protocols). But	*
 * it also avoid reading more than one cache line, for  *
 * the reader (RPCs)					*/
uint32_t cpus_in_kernel[CPU_PER_CLUSTER];

void clusters_sysfs_register(void)
{
	register uint_t i;
	register struct cluster_entry_s *entry;
  
	for(i=0; i < CLUSTER_NR; i++)
	{
		entry = &clusters_tbl[i];
		if(entry->flags != CLUSTER_DOWN)
			sysfs_entry_register(&sysfs_root_entry, &entry->node);
	}
}

void cluster_entry_init(uint_t cid, uint_t addr_start, size_t size)
{
	struct cluster_entry_s *entry = &clusters_tbl[cid];

	entry->cid = cid;
	entry->vmem_start = addr_start;
	entry->size = size;

	list_root_init(&entry->devlist);

	sprintk(entry->name,
#if CONFIG_ROOTFS_IS_VFAT
		"CID%d"
#else
		"cid%d"
#endif
		,cid);

	sysfs_entry_init(&entry->node, NULL, entry->name);
}

//Wait all cluster to initialise their flag
void cluster_wait_init(struct boot_info_s *info)
{
	uint_t i;
	uint_t cid;

	for(i=0; i < info->onln_clstr_nr; i++)
	{
		cid = clusters_tbl[i].cid;
		if(clusters_tbl[i].size == 0)//NO RAM!
			continue;

		while(!(clusters_tbl[cid].flags & CLUSTER_UP))
			cpu_rdbar();
	}
}

uint_t cluster_with_ram_nr(struct boot_info_s *info)
{
	uint_t i;
	uint_t cnt;

	cnt=0;
	for(i=0; i < info->onln_clstr_nr; i++)
	{
		if(clusters_tbl[i].size == 0)//NO RAM!
			continue;
		cnt++;
	}

	return cnt;
}

void cluster_init_flag(struct boot_info_s *info, uint_t flag)
{
	uint_t i;
	uint_t cid;
	uint_t lcid;

	lcid = info->local_cluster_id;

	for(i=0; i < info->onln_clstr_nr; i++)
	{
		cid = clusters_tbl[i].cid;

		//FIXME use clstr_wram_nr
		if(clusters_tbl[i].size == 0)//NO RAM!
			continue;

		if(cid == lcid)
			clusters_tbl[lcid].flags = flag;
		else
			remote_sw((void*)&clusters_tbl[lcid].flags, cid,
					flag);
	}

	cpu_wbflush();
}

/* TODO: deal with case of cluster of CPU_ONLY or MEM_ONLY ressources */
error_t cluster_init(struct boot_info_s *info,
		     uint64_t start_paddr, 
		     uint64_t limit_paddr,
		     uint_t start_vaddr)
{
	error_t err;
	uint_t cid;
	uint_t cpu;
	uint_t heap_start;
	uint_t begin_vaddr;
	struct cpu_s *cpu_info;
	struct cluster_s *cluster;
	//struct cluster_entry_s *entry;
	struct thread_s *this;
	extern uint_t __heap_start;

	cid         = info->local_cluster_id;
	cluster     = &cluster_manager;

	/* As soon as possible we must intialize current cpu & ids */
	this              = current_thread;
	cpu_info          = &cluster->cpu_tbl[info->local_cpu_id];
	cpu_info->cluster = cluster;

	thread_set_current_cpu(this, cpu_info);
  
	cluster->id		= cid;
	cluster->cpu_nr		= info->local_cpu_nr;
	cluster->onln_cpu_nr	= info->local_onln_cpu_nr;
	cluster->local_bscpu	= cpu_info;
	cluster->boot_clstr	= info->boot_cluster_id;
	cluster->io_clstr	= info->io_cluster_id;
	cluster->entry		= &clusters_tbl[cid];
	cluster->clstr_wram_nr	= cluster_with_ram_nr(info);
	/* ------------------------------------------------- */

	spinlock_init(&cluster->lock, "Cluster");
	atomic_init(&cluster->buffers_nr, 0);
	atomic_init(&cluster->vfs_nodes_nr, 0);
	memset(&cluster->levels_tbl[0], 0, sizeof(cluster->levels_tbl));
	memset(&cluster->keys_tbl[0], 0, sizeof(cluster->keys_tbl));
	cluster->next_key = KMEM_TYPES_NR;
 

	begin_vaddr = start_vaddr;
	start_vaddr = (uint_t)&__heap_start;
	start_vaddr  = ARROUND_UP(start_vaddr, PMM_PAGE_SIZE);
	heap_start   = start_vaddr;
	start_vaddr += (1 << (CONFIG_KHEAP_ORDER + PMM_PAGE_SHIFT));
	heap_manager_init(&cluster->khm, heap_start, heap_start, start_vaddr);

	ppm_init(&cluster->ppm, 
		 start_paddr, 
		 limit_paddr, 
		 begin_vaddr,//virtual begin of kernel 
		 start_vaddr, //virtual begin of virtual free mem (=begin_vaddr + heap_end)
		 info);
  
	kcm_init(&cluster->kcm, 
		 "KCM", 
		 sizeof(struct kcm_s), 
		 0, 1, 1, NULL, NULL, 
		 &kcm_page_alloc, &kcm_page_free);

	for(cpu = 0; cpu < cluster->cpu_nr; cpu++)
	{
		if((err = cpu_init(&cluster->cpu_tbl[cpu], 
			 cluster, 
			 cpu, 
			 arch_cpu_gid(cid, cpu))))
			return err;

		cpus_in_kernel[cpu] = 1;
	}
	
	rpc_listner_init(&cluster->re_listner);

	cluster->cpu_tbl[info->local_cpu_id].state = CPU_ACTIVE;

	if(cid == info->io_cluster_id)
		cluster_init_flag(info, CLUSTER_UP | CLUSTER_IO);
	else
		cluster_init_flag(info, CLUSTER_UP);

	cluster->task = NULL;
	cluster->manager = NULL;

	return 0;
}

/* TODO: invalidate cache line */
bool_t cluster_in_kernel(cid_t cid)
{
	gid_t cpu;

	for(cpu = 0; cpu < current_cluster->onln_cpu_nr; cpu++)
	{
		if(__cpu_in_kernel(cid, cpu))
			return true;
	}

	return false;
}

EVENT_HANDLER(manager_alarm_event_handler)
{
	struct thread_s *manager;
 
	manager = event_get_senderId(event);
 
	thread_preempt_disable(current_thread);

	//printk(INFO, "%s: cpu %d [%u]\n", __FUNCTION__, cpu_get_id(), cpu_time_stamp());

	sched_wakeup(manager);
  
	thread_preempt_enable(current_thread);

	return 0;
}

void* cluster_manager_thread(void *arg)
{
	register struct dqdt_cluster_s *root;
	register struct cluster_s *root_home;
	register uint_t tm_start;
	register uint_t tm_end;
	register uint_t cpu_id;
	struct cluster_s *cluster;
	struct thread_s *this;
	struct event_s event;
	struct alarm_info_s info;
	register uint_t cntr;
	register bool_t isRootMgr;
	register uint_t period;

	cpu_enable_all_irq(NULL);

	cluster   = arg;
	this      = current_thread;
	cpu_id    = cpu_get_id();
	root      = dqdt_root;
	root_home = dqdt_root->home;
	isRootMgr = (cluster == root_home) ? true : false;
	cntr      = 0;
	period    = (isRootMgr) ? 
		CONFIG_DQDT_ROOTMGR_PERIOD * MSEC_PER_TICK : 
		CONFIG_DQDT_MGR_PERIOD * MSEC_PER_TICK;

	event_set_senderId(&event, this);
	event_set_priority(&event, E_CHR);
	event_set_handler(&event, &manager_alarm_event_handler);
  
	info.event = &event;
	thread_preempt_disable(current_thread);

	while(1)
	{
		tm_start = cpu_time_stamp();
		dqdt_update();
		tm_end   = cpu_time_stamp();

		if(isRootMgr)
		{
			if((cntr % 10) == 0)
			{
				printk(INFO, "INFO: cpu %d, DQDT update ended [ %u - %u ]\n", 
				       cpu_id, 
				       tm_end, 
				       tm_end - tm_start);

				dqdt_print_summary(root);
			}
		}

		alarm_wait(&info, period);
		sched_sleep(this);
		cntr ++;
	}

	return NULL;
}


EVENT_HANDLER(cluster_key_create_event_handler)
{
	struct cluster_s *cluster;
	struct thread_s *sender;
	ckey_t *ckey;
	uint_t key;

	sender  = event_get_senderId(event);
	ckey    = event_get_argument(event);
	cluster = current_cluster;
	key     = cluster->next_key;

	while((key < CLUSTER_TOTAL_KEYS_NR) && (cluster->keys_tbl[key] != NULL))
		key ++;

	if(key < CLUSTER_TOTAL_KEYS_NR)
	{
		ckey->val = key;
		cluster->keys_tbl[key] = (void *) 0x1; /* Reserved */
		cluster->next_key = key;
		event_set_error(event, 0);
	}
	else
		event_set_error(event, ENOSPC);

	sched_wakeup(sender);
	return 0;
}

EVENT_HANDLER(cluster_key_delete_event_handler)
{
	struct cluster_s *cluster;
	struct thread_s *sender;
	ckey_t *ckey;
	uint_t key;

	sender  = event_get_senderId(event);
	ckey    = event_get_argument(event);
	cluster = current_cluster;
	key     = ckey->val;

	if(key < cluster->next_key)
		cluster->next_key = key;

	cluster->keys_tbl[key] = NULL;
	event_set_error(event, 0);

	sched_wakeup(sender);
	return 0;
}

#if 0
#define _CKEY_CREATE  0x0
#define _CKEY_DELETE  0x1

error_t cluster_do_key_op(ckey_t *key, uint_t op)
{
	struct event_s event;
	struct thread_s *this;
	struct cluster_s *cluster;
	struct cpu_s *cpu;

	this = current_thread;

	event_set_priority(&event, E_FUNC);
	event_set_senderId(&event, this);
	event_set_argument(&event, key);

	if(op == _CKEY_CREATE)
		event_set_handler(&event, cluster_key_create_event_handler);
	else
		event_set_handler(&event, cluster_key_delete_event_handler);

	cluster = current_cluster;
	cpu     = cluster->bscluster->bscpu;
	event_send(&event, &cpu->re_listner);

	sched_sleep(this);

	return event_get_error(&event);
}

error_t cluster_key_create(ckey_t *key)
{
	return cluster_do_key_op(key, _CKEY_CREATE);
}

error_t cluster_key_delete(ckey_t *key)
{
	return cluster_do_key_op(key, _CKEY_DELETE);
}

void* cluster_getspecific(ckey_t *key)
{
	struct cluster_s *cluster;

	cluster = current_cluster;
	return cluster->keys_tbl[key->val];
}

void  cluster_setspecific(ckey_t *key, void *val)
{
	struct cluster_s *cluster;

	cluster = current_cluster;
	cluster->keys_tbl[key->val] = val;
}
#endif
