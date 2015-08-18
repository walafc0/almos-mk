/*
 * kern/cluster.h - Cluster-Manager Interface
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

#ifndef _CLUSTER_H_
#define _CLUSTER_H_

#include <config.h>
#include <system.h>
#include <types.h>
#include <spinlock.h>
#include <cpu.h>
#include <list.h>
#include <kmem.h>
#include <atomic.h>
#include <mcs_sync.h>
#include <metafs.h>
#include <ppm.h>
#include <kcm.h>
#include <heap_manager.h>

#define  CLUSTER_DOWN       0x00      
#define  CLUSTER_UP         0x01
#define  CLUSTER_STANDBY    0x02
#define  CLUSTER_CPU_ONLY   0x04
#define  CLUSTER_MEM_ONLY   0x08
#define  CLUSTER_IO         0x10

#define  CLUSTER_TOTAL_KEYS_NR (KMEM_TYPES_NR + CONFIG_CLUSTER_KEYS_NR)

struct task_s;
struct thread_s;
struct heap_manager_s;
struct dqdt_cluster_s;
struct cluster_entry_s;

struct cluster_s
{
	spinlock_t lock;
	atomic_t buffers_nr;
	atomic_t vfs_nodes_nr;

	/* Logical cluster */
	struct dqdt_cluster_s *levels_tbl[CONFIG_DQDT_LEVELS_NR];

	/* Physical Pages Manager */
	struct ppm_s ppm;

	/* Kernel Cache Manager */
	struct kcm_s kcm;

	/* Kernel Heap Manager */
	struct heap_manager_s khm;

	/* Cluster keys */
	void *keys_tbl[CLUSTER_TOTAL_KEYS_NR];
	uint_t next_key;

	/* Init Info */
	uint_t id;
	uint_t cpu_nr;
	uint_t onln_cpu_nr;
	uint_t clstr_nr;
	uint_t clstr_wram_nr;

	uint_t io_clstr;
	uint_t boot_clstr;

	uint16_t x_coord;
	uint16_t y_coord;
	uint16_t z_coord;
	uint16_t chip_id;

	/* Chains Info */
	struct list_entry list;
	struct list_entry rope;

	/* This cluster entry */
	struct cluster_entry_s* entry;

	/* CPUs Info */
	struct cpu_s cpu_tbl[CPU_PER_CLUSTER];

	/* Kernel Task */
	struct task_s *task;

	/* Manger Thread */
	struct thread_s *manager;

	/* RPC cluster */
	struct rpc_listner_s re_listner;
  
	/* Cluter's BootStrap CPU */
	struct cpu_s *local_bscpu;
};

//the flag field is remotely initialised
struct cluster_entry_s
{
	uint_t cid;
	uint_t vmem_start;
	uint_t size;
	volatile uint_t flags;

	/* System Devices */
	struct list_entry devlist;

	/* Sysfs information */
	char name[SYSFS_NAME_LEN];
	sysfs_entry_t node;
};

extern struct cluster_s cluster_manager;
extern struct cluster_entry_s clusters_tbl[CLUSTER_NR];
extern uint32_t cpus_in_kernel[CPU_PER_CLUSTER];

#define cluster_get_cpus_nr(cluster)

//struct cluster_s* cluster_cid2ptr(uint_t cid);

//void cluster_init_flag(uint_t flag);
void clusters_sysfs_register(void);

error_t cluster_init(struct boot_info_s *info, 
		     uint64_t start_paddr, 
		     uint64_t limit_paddr,
		     uint_t begin_vaddr);

void cluster_entry_init(uint_t cid, uint_t addr_start, size_t size);

void cluster_wait_init(struct boot_info_s *info);

/* check if a cpu of the cluster is in the kernel */
bool_t cluster_in_kernel(cid_t cid);

//FIXME: TO remove?
struct cluster_key_s;
typedef struct cluster_key_s ckey_t;

#define cluster_get_keyValue(key_addr)

error_t cluster_key_create(ckey_t *key);
void* cluster_getspecific(ckey_t *key);
void  cluster_setspecific(ckey_t *key, void *val);
error_t cluster_key_delete(ckey_t *key);

void* cluster_manager_thread(void *arg);

/////////////////////////////////////////////
//              Private Section            //
/////////////////////////////////////////////
struct cluster_key_s
{
	uint_t val CACHELINE;
};

#undef cluster_get_keyValue
#define cluster_get_keyValue(_key) ((_key)->val)
/* Nota: set_id_by_name/get_id_by_name */

#undef cluster_get_cpus_nr
#define cluster_get_cpus_nr(_cluster) ((_cluster)->cpus_nr)
/////////////////////////////////////////////




#endif	/* _CLUSTER_H_ */
