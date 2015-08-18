/*
 * kern/dqdt.c - Distributed Quaternary Decision Tree
 * 
 * Copyright (c) 2008,2009,2010,2011,2012 Ghassan Almaless
 * Copyright (c) 2011,2012,2013,2014,2015 UPMC Sorbonne Universites
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
#include <system.h>
#include <thread.h>
#include <task.h>
#include <cluster.h>
#include <pmm.h>
#include <cpu.h>
#include <boot-info.h>
#include <utils.h>

#include <dqdt.h>

#define DQDT_MGR_PERIOD      CONFIG_DQDT_MGR_PERIOD
#define DQDT_SELECT_LTCN     0x001
#define DQDT_THREAD_OP       0x002
#define DQDT_FORK_OP         0x004
#define DQDT_MIGRATE_OP      0x008
#define DQDT_MEMORY_OP       0x010

#define DQDT_MAX_M           0x1000
#define DQDT_MAX_T           0x1000
#define DQDT_BIG_T           0x04F0
#define DQDT_MAX_U           0x1000
#define DQDT_MAX_D           0x10000

#if CONFIG_USE_DQDT
#define down_dmsg(x,...)    printk(INFO, __VA_ARGS__)
#define up_dmsg(x,...)      printk(INFO, __VA_ARGS__)
#define select_dmsg(x,...)  printk(INFO, __VA_ARGS__)
#define sort_dmsg(x,...)    printk(INFO, __VA_ARGS__)
#define update_dmsg(x,...)  printk(INFO, __VA_ARGS__)
#else
#define down_dmsg(x,...)    dqdt_dmsg(x,__VA_ARGS__)
#define up_dmsg(x,...)      dqdt_dmsg(x,__VA_ARGS__)
#define select_dmsg(x,...)  dqdt_dmsg(x,__VA_ARGS__)
#define sort_dmsg(x,...)    dqdt_dmsg(x,__VA_ARGS__)
#define update_dmsg(x,...)  dqdt_dmsg(x,__VA_ARGS__)
#endif

#define DQDT_SELECT_HELPER(n) bool_t (n) (struct dqdt_cluster_s *logical, \
					  struct dqdt_attr_s *attr,	\
					  uint_t child_index)

typedef DQDT_SELECT_HELPER(dqdt_select_t);

struct dqdt_cluster_s *dqdt_root;
/* TODO:
 * if CONFIG_USE_DQDT is set to yes, this file does not compile. We need to
 * add to struct dqdt_attr_s two fields : struct cluster_s and struct cpu_s.
 * Those fields are also used in kern/sys_fork.c
 *
 * use the lock-free kfifo (MWSR) instead of using spinlock + wait_queue
 */

#if CONFIG_USE_DQDT
static spinlock_t dqdt_lock;
static struct wait_queue_s dqdt_task_queue;
static atomic_t dqdt_waiting_cntr;
static uint_t dqdt_update_cntr;

void dqdt_print_summary(struct dqdt_cluster_s *cluster)
{
	uint_t i;

	printk(INFO,"cpu %d, cid %d, level %d, cores_nr %d, index %d, Indicators Summary:\n",
	       cpu_get_id(),
	       current_cluster->id,
	       cluster->level,
	       cluster->cores_nr,
	       cluster->index);

	printk(INFO,"   M %d, T %d, U %d\n", 
	       cluster->info.summary.M, 
	       atomic_get(&cluster->info.summary.T), 
	       cluster->info.summary.U);
  
	printk(INFO,"   Pages_tbl [");
  
	for(i = 0; i < PPM_MAX_ORDER; i++)
		printk(INFO, "%d, ", cluster->info.summary.pages_tbl[i]);
	
	printk(INFO, "\b\b]\n");
}

void dqdt_print(struct dqdt_cluster_s *cluster)
{
	uint_t i;
 
	printk(INFO,"Entring Level %d, Index %d\n", 
	       cluster->level, 
	       cluster->index);
    
	for(i = 0; i < 4; i++)
	{
		if(cluster->children[i] == NULL)
		{
			printk(INFO,"  Level %d: Child %d is Down\n", 
			       cluster->level, i);

			continue;
		}

		if(cluster->level == 1)
		{
			printk(INFO,"  Level %d: Child %d is Physical-Cluster of cid %d\n", 
			       1, i, 
			       cluster->children[i]->home->id);

			continue;
		}

		dqdt_print(cluster->children[i]);
	}

	printk(INFO, "Leaving Level %d, Index %d\n", 
	       cluster->level, 
	       cluster->index);
}

void dqdt_init(struct boot_info_s *info)
{
	uint_t i;

	struct cluster_s *cluster = current_cluster;
	struct dqdt_cluster_s *logical = cluster->levels_tbl[0];

	if(cluster->id == info->boot_cluster_id)
	{
		spinlock_init(&dqdt_lock, "dqdt lock");
		wait_queue_init(&dqdt_task_queue, "dqdt task");
		atomic_init(&dqdt_waiting_cntr, 0);
		dqdt_update_cntr = 0;
		cpu_wbflush();
	}

	for(i = 0; i < 4; i++)
		atomic_init(&logical->info.tbl[i].T, 0);

	atomic_init(&logical->info.summary.T, 0);
}

struct dqdt_cluster_s* dqdt_logical_lookup(uint_t level)
{
	register struct dqdt_cluster_s *current;

	current = current_cluster->levels_tbl[0];

	while((current != NULL) && (current->level != level))
		current = current->parent;

	return (current == NULL) ? dqdt_root : current;
}

void dqdt_wait_for_update(void)
{
#if CONFIG_DQDT_WAIT_FOR_UPDATE
	struct thread_s *this;

	this = current_thread;

	spinlock_lock(&dqdt_lock);
	wait_on(&dqdt_task_queue, WAIT_LAST);
	spinlock_unlock(&dqdt_lock);

	(void)atomic_add(&dqdt_waiting_cntr, 1);

	sched_sleep(current_thread);

	(void)atomic_add(&dqdt_waiting_cntr, -1);
#endif
}


void dqdt_update_done(void)
{
#if CONFIG_DQDT_WAIT_FOR_UPDATE
	register bool_t isEmpty;

	dqdt_update_cntr ++;

	if((dqdt_update_cntr % 2) == 0)
	{
		spinlock_lock(&dqdt_lock);

		isEmpty = wait_queue_isEmpty(&dqdt_task_queue);

		if(!isEmpty)
		{
			(void)wakeup_one(&dqdt_task_queue, WAIT_FIRST);
			isEmpty = wait_queue_isEmpty(&dqdt_task_queue);
		}

		spinlock_unlock(&dqdt_lock);
	}
#endif
}

void dqdt_indicators_update(dqdt_indicators_t *entry,
			 uint_t M, 
			 uint_t T,
			 uint_t U,
			 uint_t *pages_tbl)
{
	register uint_t i;

	entry->M = M;
	entry->U = U;

	//(void)atomic_set(&entry->T, T);

	for(i = 0; ((pages_tbl != NULL) && (i < PPM_MAX_ORDER)); i++)
		entry->pages_tbl[i] = pages_tbl[i];

	cpu_wbflush();
}

error_t dqdt_update(void)
{
	struct dqdt_cluster_s *logical;
	struct dqdt_cluster_s *parent;
	struct cluster_s *cluster;
	register uint_t i,j,p,t;
	register uint_t usage;
	register uint_t threads;
	register uint_t free_pages;
	uint_t pages_tbl[PPM_MAX_ORDER];

	cluster = current_cluster;
	usage   = 0;
	threads = 0;
	t       = 0;

	update_dmsg(1, "%s: cluster %d, started, onln_cpu_nr %d\n", 
		    __FUNCTION__,
		    cluster->id,
		    cluster->onln_cpu_nr);

	for(i = 0; i < cluster->onln_cpu_nr; i++)
	{
		cpu_compute_stats(&cluster->cpu_tbl[i], CONFIG_CPU_LOAD_PERIOD);
		usage   += cluster->cpu_tbl[i].busy_percent;//cluster->cpu_tbl[i].usage;
		//threads += cluster->cpu_tbl[i].scheduler.user_nr;
		t += atomic_get(&cluster->levels_tbl[0]->info.tbl[i].T);
	}

	usage /= cluster->onln_cpu_nr;
  
	update_dmsg(1, "%s: cluster %d, usage %d, T %d/%d\n",
		    __FUNCTION__,
		    cluster->id,
		    usage,
		    threads,t);
 
	logical = cluster->levels_tbl[0];

	for(i = 0; i < PPM_MAX_ORDER; i++)
		logical->info.summary.pages_tbl[i] = cluster->ppm.free_pages[i].pages_nr;

	dqdt_indicators_update(&logical->info.summary,
			       cluster->ppm.free_pages_nr,
			       threads,
			       usage,
			       NULL);

	parent = logical->parent;

	dqdt_indicators_update(&parent->info.tbl[logical->index], 
			       cluster->ppm.free_pages_nr,
			       threads,
			       usage,
			       &logical->info.summary.pages_tbl[0]);

	for(i = 1; i < DQDT_LEVELS_NR; i++)
	{
		logical = cluster->levels_tbl[i];

		if(logical == NULL)
			continue;  /* TODO: verify that we can break here instead of continue */
   
		update_dmsg(1, "%s: cluster %d, level %d, logical level %d\n",
			    __FUNCTION__,
			    cluster->id,
			    i,logical->level);
    
		free_pages = 0;
		threads    = 0;
		usage      = 0;
		memset(&pages_tbl[0], 0, sizeof(pages_tbl));

		for(j = 0; j < 4; j++)
		{
			if(logical->children[j] != NULL)
			{
				free_pages += logical->info.tbl[j].M;
				//threads    += atomic_get(&logical->info.tbl[j].T);
				usage      += logical->info.tbl[j].U;
      
				for(p = 0; p < PPM_MAX_ORDER; p++)
					pages_tbl[p] += logical->info.tbl[j].pages_tbl[p];
			}
		}

		usage = usage / logical->childs_nr;

		update_dmsg(1, "%s: cluster %d, level %d, usage %d, T %d, M %d (j=%d,p=%d)\n",
			    __FUNCTION__,
			    cluster->id,
			    i,
			    usage,
			    threads,
			    free_pages,j,p);

		dqdt_indicators_update(&logical->info.summary,
				       free_pages,
				       threads,
				       usage,
				       pages_tbl);

		parent = logical->parent;

		if(parent != NULL)
		{
			dqdt_indicators_update(&parent->info.tbl[logical->index],
					       free_pages,
					       threads,
					       usage,
					       pages_tbl);

			update_dmsg(1, "%s: cluster %d, level %d to level %d, propagated\n",
				    __FUNCTION__,
				    cluster->id,
				    i, i+1);
		}
		else
		{
			dqdt_update_done();

#if CONFIG_DQDT_DEBUG == 2
			dqdt_print_summary(logical);
#endif
		}
	}

	return 0;
}

void dqdt_update_threads_number(struct dqdt_cluster_s *logical, uint_t core_index, sint_t count)
{
	register uint_t index;

	index = core_index;
	
	while(logical != NULL)
	{
		(void)atomic_add(&logical->info.tbl[index].T, count);
		(void)atomic_add(&logical->info.summary.T, count);

		index   = logical->index;
		logical = logical->parent;
	}

#if CONFIG_DQDT_WAIT_FOR_UPDATE
	register bool_t isEmpty;
	sint_t old;

	if((count < 0) && ((old = atomic_get(&dqdt_waiting_cntr)) != 0))
	{
		spinlock_lock(&dqdt_lock);

		isEmpty = wait_queue_isEmpty(&dqdt_task_queue);

		if(!isEmpty)
			(void)wakeup_one(&dqdt_task_queue, WAIT_FIRST);

		spinlock_unlock(&dqdt_lock);
	}
#endif
}

void dqdt_primary_table_sort1(sint_t *values_tbl, sint_t *aux_tbl, uint_t count)
{
	uint_t tmp, i, j;

	for(i = 0; i < count; i++)
	{
		for(j = i + 1; j < count; j++)
		{
			if(values_tbl[j] < values_tbl[i])
			{
				tmp           = values_tbl[i];
				values_tbl[i] = values_tbl[j];
				values_tbl[j] = tmp;

				tmp           = aux_tbl[i];
				aux_tbl[i]    = aux_tbl[j];
				aux_tbl[j]    = tmp;
			}
		}
	}
}

void dqdt_primary_table_sort2(sint_t *key_tbl, sint_t *aux1_tbl, sint_t *aux2_tbl, uint_t count)
{
	uint_t tmp, i, j;

	for(i = 0; i < count; i++)
	{
		for(j = i + 1; j < count; j++)
		{
			if(key_tbl[j] < key_tbl[i])
			{
				tmp         = key_tbl[i];
				key_tbl[i]  = key_tbl[j];
				key_tbl[j]  = tmp;

				tmp         = aux1_tbl[i];
				aux1_tbl[i] = aux1_tbl[j];
				aux1_tbl[j] = tmp;

				tmp         = aux2_tbl[i];
				aux2_tbl[i] = aux2_tbl[j];
				aux2_tbl[j] = tmp;
			}
		}
	}
}

void dqdt_primary_table_sort3(sint_t *key_tbl, sint_t *aux1_tbl, sint_t *aux2_tbl, sint_t *aux3_tbl, uint_t count)
{
	uint_t tmp, i, j;

	for(i = 0; i < count; i++)
	{
		for(j = i + 1; j < count; j++)
		{
			if(key_tbl[j] < key_tbl[i])
			{
				tmp         = key_tbl[i];
				key_tbl[i]  = key_tbl[j];
				key_tbl[j]  = tmp;

				tmp         = aux1_tbl[i];
				aux1_tbl[i] = aux1_tbl[j];
				aux1_tbl[j] = tmp;

				tmp         = aux2_tbl[i];
				aux2_tbl[i] = aux2_tbl[j];
				aux2_tbl[j] = tmp;
				
				tmp         = aux3_tbl[i];
				aux3_tbl[i] = aux3_tbl[j];
				aux3_tbl[j] = tmp;
			}
		}
	}
}

void dqdt_secondary_table_sort(sint_t *key_tbl, sint_t *aux_tbl, sint_t *select_tbl, uint_t count)
{
	register uint_t i,j,tmp;

	for(j = 0; j <= count; j++)
	{
		for(i = 0; i < (count-1); i++)
		{
			if((key_tbl[i] == key_tbl[i+1]) && (aux_tbl[i] > aux_tbl[i+1]))
			{
				tmp             = aux_tbl[i];
				aux_tbl[i]      = aux_tbl[i+1];
				aux_tbl[i+1]    = tmp;

				tmp             = select_tbl[i];
				select_tbl[i]   = select_tbl[i+1];
				select_tbl[i+1] = tmp;
			}
		}
	}
}

void dqdt_tertiary_table_sort(sint_t *key1_tbl, sint_t *key2_tbl, sint_t *aux_tbl, sint_t *select_tbl, uint_t count)
{
	register uint_t i,j,tmp;

	for(j = 0; j <= count; j++)
	{
		for(i = 0; i < (count-1); i++)
		{
			if((key1_tbl[i] == key1_tbl[i+1]) && 
			   (key2_tbl[i] == key2_tbl[i+1]) && 
			   (aux_tbl[i] > aux_tbl[i+1]))
			{
				tmp             = aux_tbl[i];
				aux_tbl[i]      = aux_tbl[i+1];
				aux_tbl[i+1]    = tmp;

				tmp             = select_tbl[i];
				select_tbl[i]   = select_tbl[i+1];
				select_tbl[i+1] = tmp;
			}
		}
	}
}

DQDT_SELECT_HELPER(dqdt_select_by_T)
{
	register uint_t N, T;

	T = atomic_get(&logical->info.summary.T);
	N = logical->cores_nr;

	select_dmsg(1, "%s: cpu %d, current level %d, indx %d, T:%d/%d\n",
		    __FUNCTION__,
		    cpu_get_id(),
		    logical->level,
		    child_index,
		    T, N);

	return (T < N) ? true : false;
}

DQDT_SELECT_HELPER(dqdt_select_by_T_U)
{
	register uint_t N, T, U;

	T = atomic_get(&logical->info.summary.T);
	U = logical->info.summary.U;
	N = logical->cores_nr;

	select_dmsg(1, "%s: cpu %d, current level %d, indx %d, T: %d/%d, U:%d/%d\n",
		    __FUNCTION__,
		    cpu_get_id(),
		    logical->level,
		    child_index,
		    T, N,
		    U, attr->u_threshold);

	return ((T < N) || (U < attr->u_threshold)) ? true : false;
}

DQDT_SELECT_HELPER(dqdt_select_by_T_M)
{
	register uint_t M, T, N;

	T = atomic_get(&logical->info.summary.T);
	M = logical->info.summary.M;
	N = logical->cores_nr;

	select_dmsg(1, "%s: cpu %d, current level %d, indx %d, T: %d/%d, M:%d/%d\n",
		    __FUNCTION__,
		    cpu_get_id(),
		    logical->level,
		    child_index,
		    T, N,
		    attr->m_threshold,M);

	return ((T < N) || (M > attr->m_threshold)) ? true : false;
}

DQDT_SELECT_HELPER(dqdt_sort_by_U_T_D)
{
	register uint_t i;
	sint_t usage_tbl[4];
	sint_t distance_tbl[4];
	sint_t threads_tbl[4];
	
	for(i = 0; i < 4; i++)
	{
		attr->select_tbl[i] = i;

		if((logical->children[i] == NULL) || (i == child_index))
		{
			usage_tbl[i]    = DQDT_MAX_U;
			distance_tbl[i] = DQDT_MAX_D;
			threads_tbl[i]  = DQDT_MAX_T;
			continue;
		}

		usage_tbl[i]    = logical->info.tbl[i].U;
		distance_tbl[i] = arch_dqdt_distance(attr->origin, logical->children[i], attr);
		threads_tbl[i]  = atomic_get(&logical->info.tbl[i].T);
	}

	dqdt_primary_table_sort3(usage_tbl, threads_tbl, distance_tbl, attr->select_tbl, 4);

	dqdt_secondary_table_sort(usage_tbl, threads_tbl, attr->select_tbl, 4);

	dqdt_tertiary_table_sort(usage_tbl, threads_tbl, distance_tbl, attr->select_tbl, 4);

	return true;
}


DQDT_SELECT_HELPER(dqdt_sort_by_T_D)
{
	register uint_t i;
	sint_t threads_tbl[4];
	sint_t distance_tbl[4];

	i = atomic_get(&logical->info.summary.T);

	if((attr->flags & DQDT_SELECT_LTCN) && (i >= logical->cores_nr))
		return false;

	for(i = 0; i < 4; i++)
	{
		attr->select_tbl[i] = i;

		if((logical->children[i] == NULL) || (i == child_index))
		{
			threads_tbl[i]  = DQDT_MAX_T;
			distance_tbl[i] = DQDT_MAX_D;
			continue;
		}

		threads_tbl[i]  = atomic_get(&logical->info.tbl[i].T);
		distance_tbl[i] = arch_dqdt_distance(attr->origin, logical->children[i], attr);
	}

	dqdt_primary_table_sort2(&threads_tbl[0], &distance_tbl[0], attr->select_tbl, 4);

	dqdt_secondary_table_sort(&threads_tbl[0], &distance_tbl[0], attr->select_tbl, 4);

	return true;
}


DQDT_SELECT_HELPER(dqdt_sort_by_rT_D)
{
	register uint_t i;
	register sint_t val, cores, bigval;
	sint_t threads_tbl[4];
	sint_t distance_tbl[4];

	val = atomic_get(&logical->info.summary.T);

	if((attr->flags & DQDT_SELECT_LTCN) && (val >= logical->cores_nr))
		return false;

	cores  = logical->cores_nr / logical->childs_nr;
	bigval = DQDT_BIG_T;
 
	for(i = 0; i < 4; i++)
	{
		attr->select_tbl[i] = i;

		if((logical->children[i] == NULL) || (i == child_index))
		{
			threads_tbl[i]  = DQDT_MAX_T;
			distance_tbl[i] = DQDT_MAX_D;
			continue;
		}

		val = atomic_get(&logical->info.tbl[i].T);
		val = cores - val;
		threads_tbl[i]  = (val > 0) ? val : bigval + (0 - val);
		distance_tbl[i] = arch_dqdt_distance(attr->origin, logical->children[i], attr);
	}

	dqdt_primary_table_sort2(&threads_tbl[0], &distance_tbl[0], attr->select_tbl, 4);

	dqdt_secondary_table_sort(&threads_tbl[0], &distance_tbl[0], attr->select_tbl, 4);

	return true;
}

bool_t dqdt_down_traversal(struct dqdt_cluster_s *logical, 
			   struct dqdt_attr_s *attr,
			   dqdt_select_t *sort,
			   dqdt_select_t *select,
			   dqdt_select_t *request,
			   uint_t index)
{
	register uint_t i,j;
	register bool_t found,done;
	sint_t select_tbl[4];

	down_dmsg(1, "%s: cpu %d, current level %d, index %d\n",
		  __FUNCTION__,
		  cpu_get_id(),
		  logical->level,
		  index);

	if(logical->level == 0)
		return request(logical,attr,-1);
	
	attr->select_tbl = &select_tbl[0];

	found = sort(logical, attr, index);

	if(!found)
		return false;
	
	down_dmsg(1,"%s: S-Tbl: [%d,%d,%d,%d]\n",
		  __FUNCTION__,
		  select_tbl[0],
		  select_tbl[1],
		  select_tbl[2],
		  select_tbl[3]);

	for(i = 0; i < 4; i++)
	{
		j = select_tbl[i];

		if((logical->children[j] == NULL) || (j == index))
			continue;

		found = select(logical->children[j], attr, j);

		if(!found)
			continue;

		done = dqdt_down_traversal(logical->children[j], attr, sort, select, request, 5);

		if(done) return true;

		down_dmsg(1, "%s: child %d is busy\n", __FUNCTION__, j);
	}

	down_dmsg(1, "%s: current level %d do not mach request\n", __FUNCTION__, logical->level);

	return false;
}

error_t dqdt_up_traversal(struct dqdt_cluster_s *logical,
			  struct dqdt_attr_s *attr,
			  dqdt_select_t clstr_select,
			  dqdt_select_t child_sort,
			  dqdt_select_t child_select,
			  dqdt_select_t request,
			  uint_t limit,
			  uint_t index)
{
	bool_t found;
	bool_t done;

	if(logical == NULL)
		return EAGAIN;
  
	if(logical->level > limit)
		return ERANGE;
 
	up_dmsg(1, "%s: cpu %d, current level %d\n",
		__FUNCTION__, 
		cpu_get_id(),
		logical->level);

	found = clstr_select(logical,attr,-1);

	if(found)
	{
		done = dqdt_down_traversal(logical,
					   attr,
					   child_sort,
					   child_select,
					   request,
					   index);

		if(done) return 0;
	}

	if(logical->childs_nr == 1)
		return EAGAIN;

	return dqdt_up_traversal(logical->parent,
				 attr,
				 clstr_select,
				 child_sort,
				 child_select,
				 request,
				 limit,
				 logical->index);
}

DQDT_SELECT_HELPER(dqdt_core_min_threads_select_old)
{
	register struct cluster_s *cluster;
	register struct cpu_s *cpu;
	register uint_t i, min, indx, val;
	uint_t new_core_T;
	uint_t new_clstr_T;
	uint_t old_clstr_T;
	uint_t cntr = 4;
	bool_t isDone;

	isDone = false;

	while(!isDone && (cntr > 0))
	{
		cntr --;
		min  = (uint_t) -1;
		indx = 0;

		for(i = 0; i < 4; i++)
		{
			val = atomic_get(&logical->info.tbl[i].T);

			if(val < min)
			{
				min  = val;
				indx = i;
			}
		}

		cluster       = logical->home;
		cpu           = &cluster->cpu_tbl[indx];
		attr->cluster = cluster;
		attr->cpu     = cpu;

		old_clstr_T = atomic_get(&logical->info.summary.T);

		if((attr->flags & DQDT_SELECT_LTCN) && (old_clstr_T >= logical->cores_nr))
			return false;

		dqdt_update_threads_number(logical, indx, 1);

		new_clstr_T  = atomic_get(&logical->info.summary.T);
		new_core_T   = atomic_get(&logical->info.tbl[indx].T);
		min         += 1;
		old_clstr_T += 1;

		if((new_core_T != min) || (new_clstr_T != old_clstr_T))
			dqdt_update_threads_number(logical, indx, -1);
		else
			isDone = true;
	}

	return isDone;
}

DQDT_SELECT_HELPER(dqdt_core_min_threads_select)
{
	register struct cluster_s *cluster;
	register struct cpu_s *cpu;
	register uint_t i, min, indx, val;

	min  = (uint_t) -1;
	indx = 0;

	for(i = 0; i < 4; i++)
	{
		val = atomic_get(&logical->info.tbl[i].T);

		if(val < min)
		{
			min  = val;
			indx = i;
		}
	}

	cluster       = logical->home;
	cpu           = &cluster->cpu_tbl[indx];
	attr->cluster = cluster;
	attr->cpu     = cpu;

	val = atomic_get(&logical->info.summary.T);

	return ((attr->flags & DQDT_SELECT_LTCN) && (val >= logical->cores_nr)) ? true : false;
}

DQDT_SELECT_HELPER(dqdt_cpu_min_usage_select)
{
	register struct cluster_s *cluster;
	register struct cpu_s *cpu;
	register uint_t i;
	register uint_t min;
	register uint_t usage;

	cluster = logical->home;
  
	assert(cluster != NULL);

	for(min = 0, usage = 1000, i = 0; i < cluster->onln_cpu_nr; i++)
	{
		if(cluster->cpu_tbl[i].usage < usage)
		{
			usage = cluster->cpu_tbl[i].usage;
			min = i;
		}
	}

	if(usage >= attr->u_threshold)
		return false;

	cpu = &cluster->cpu_tbl[min];
	cluster->cpu_tbl[min].usage = 100;
	cpu_wbflush();
	pmm_cache_flush_vaddr((vma_t)&cluster->cpu_tbl[min].usage, PMM_DATA);

	attr->cluster = cluster;
	attr->cpu     = cpu;
	return true;
}

error_t dqdt_do_placement(struct dqdt_cluster_s *logical, 
			  struct dqdt_attr_s *attr, 
			  uint_t index,
			  uint_t depth,
			  uint_t d_attr)
{
	error_t err;

	attr->flags |= DQDT_SELECT_LTCN;
	attr->origin = current_cluster->levels_tbl[0];
	attr->d_attr = d_attr;

	cpu_wbflush();

	err = dqdt_up_traversal(logical,
				attr,
				(attr->flags & DQDT_FORK_OP) ? dqdt_select_by_T_M : dqdt_select_by_T,
				(attr->flags & (DQDT_FORK_OP || DQDT_MIGRATE_OP)) ? dqdt_sort_by_T_D : dqdt_sort_by_rT_D,
				(attr->flags & DQDT_FORK_OP) ? dqdt_select_by_T_M : dqdt_select_by_T,
				dqdt_core_min_threads_select,
				depth,
				index);
	if(err == 0) return 0;

	select_dmsg(1, "%s: Try strategy 2, logical level %d\n", __FUNCTION__, logical->level);

	dqdt_wait_for_update();

	attr->flags &= ~DQDT_SELECT_LTCN;
	attr->u_threshold = 100;

	err = dqdt_up_traversal(logical,
				attr,
				dqdt_select_by_T_U,
				dqdt_sort_by_U_T_D,
				dqdt_select_by_T_U,
				dqdt_cpu_min_usage_select,
				depth,
				index);

	if((err == 0) || (attr->flags & DQDT_MIGRATE_OP))
		return err;

	attr->u_threshold = 200;

	err = dqdt_up_traversal(logical,
				attr,
				dqdt_select_by_T_U,
				dqdt_sort_by_U_T_D,
				dqdt_select_by_T_U,
				dqdt_cpu_min_usage_select,
				depth,
				index);

	return err;
}


error_t dqdt_thread_placement(struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr)
{
	register error_t err;
	register uint_t tm_start, tm_end;

	select_dmsg(1, "%s: cpu %d, Started, logical level %d\n",
		    __FUNCTION__,
		    cpu_get_id(),
		    logical->level);

	attr->flags = DQDT_THREAD_OP;

	tm_start = cpu_time_stamp();

	err = dqdt_do_placement(logical, attr, logical->index, DQDT_MAX_DEPTH, DQDT_DIST_DEFAULT);

	tm_end = cpu_time_stamp();

	attr->tm_request = tm_end - tm_start;

	return err;
}

error_t dqdt_thread_migrate(struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr)
{
	struct cluster_s *cluster;
	register uint_t tm_start, tm_end;
	error_t err;

	cluster = current_cluster;
	attr->flags = DQDT_MIGRATE_OP;

	tm_start = cpu_time_stamp();

	err = dqdt_do_placement(logical,
				attr,
				(cluster->levels_tbl[0] == logical) ? logical->index : 5,
				DQDT_MAX_DEPTH,
				DQDT_DIST_DEFAULT);

	tm_end = cpu_time_stamp();

	select_dmsg(1, "%s: cpu %d, pid %d, tid %d, ended, found %s\n",
		    __FUNCTION__,
		    cpu_get_id(),
		    current_thread->task->pid,
		    current_thread->info.order,
		    (err == 0) ? "(Yes)" : "(No)");

	attr->tm_request = tm_end - tm_start;

	return err;
}


error_t dqdt_task_placement(struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr)
{
	error_t err;
	register uint_t tm_start, tm_end;

	attr->m_threshold = current_cluster->ppm.uprio_pages_min;
	attr->flags       = DQDT_FORK_OP;

	tm_start = cpu_time_stamp();

	err = dqdt_do_placement(logical, attr, 5, DQDT_MAX_DEPTH, DQDT_DIST_RANDOM);

	if(err)
		err = dqdt_do_placement(dqdt_root, attr, 5, DQDT_MAX_DEPTH, DQDT_DIST_RANDOM);

	tm_end = cpu_time_stamp();

	attr->tm_request = tm_end - tm_start;

#if CONFIG_EXEC_LOCAL
        attr->cid_exec  = current_cid;
#else
        attr->cid_exec  = (current_cid+1)%KERNEL_TOTAL_NR; /* Round-Robin allocation */
#endif
	return err;
}

bool_t dqdt_check_mem_indicators(dqdt_indicators_t *entry, struct dqdt_attr_s *attr)
{
	register struct ppm_dqdt_req_s *req;
	register bool_t found;
	register uint_t i;

	req = attr->data;

	for(found = false, i = req->order; ((i < CONFIG_PPM_MAX_ORDER) && (found == false)); i++)
		found = (entry->pages_tbl[i] != 0) ? true : false;

	return (found && (entry->M > req->threshold)) ? true : false;
}

DQDT_SELECT_HELPER(dqdt_select_by_M)
{
	return dqdt_check_mem_indicators(&logical->info.summary, attr);
}

DQDT_SELECT_HELPER(dqdt_sort_by_M_D)
{
	register uint_t i;
	register uint_t val;
	bool_t found;
	sint_t found_tbl[4];
	sint_t distance_tbl[4];

	found = false;

	for(i = 0; i < 4; i++)
	{
		attr->select_tbl[i] = i;

		if((logical->children[i] == NULL) || (i == child_index))
		{
			found_tbl[i]    = DQDT_MAX_M;
			distance_tbl[i] = DQDT_MAX_D;
			continue;
		}

		val = dqdt_check_mem_indicators(&logical->info.tbl[i], attr);
		
		if(val == true)
		{
			found_tbl[i] = 0; /* Any min value */
			found = true;
		}
		else
			found_tbl[i] = DQDT_MAX_M;

		distance_tbl[i] = arch_dqdt_distance(attr->origin, logical->children[i], attr);
	}

	if(found == false)
		return false;

	dqdt_primary_table_sort1(found_tbl, attr->select_tbl, 4);
	
	dqdt_secondary_table_sort(found_tbl, distance_tbl, attr->select_tbl, 4);
	
	return true;
}

DQDT_SELECT_HELPER(dqdt_mem_do_request)
{
	register struct ppm_dqdt_req_s *req;
	register struct ppm_s *ppm;
	register bool_t found;
	register uint_t i;

	req = attr->data;

	ppm = &logical->home->ppm;

	for(found = false, i = req->order; ((i < CONFIG_PPM_MAX_ORDER) && (found == false)); i++)
	{
		found = (ppm->free_pages[i].pages_nr != 0) ? true : false;
	}

	found = (found && (ppm->free_pages_nr > req->threshold)) ? true : false;

	if(found)
	{
		attr->cluster = logical->home;
		attr->cpu = NULL;
		return true;
	}

	current_thread->info.m_err_nr ++;
	return false;
}


error_t dqdt_mem_request(struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr)
{
	error_t err;

	attr->d_attr = DQDT_DIST_DEFAULT;
	attr->origin = logical;
	attr->flags  = DQDT_MEMORY_OP;

	err = dqdt_up_traversal(logical,
				attr,
				dqdt_select_by_M,
				dqdt_sort_by_M_D,
				dqdt_select_by_M,
				dqdt_mem_do_request,
				DQDT_MAX_DEPTH,
				logical->index);
	return err;
}

#else /* CONFIG_USE_DQDT == no */

/** Print DQDT starting from the given logical cluster */
void dqdt_print(struct dqdt_cluster_s *cluster)
{
	printk(INFO,"%s: not yet implemented\n", __FUNCTION__);

}

/** Print summary information of the given logical cluster */
void dqdt_print_summary(struct dqdt_cluster_s *cluster)
{
	printk(INFO,"%s: not yet implemented\n", __FUNCTION__);
}

/** Update the core-usage & memory estimations, should be called periodically */
error_t dqdt_update(void)
{
	/**/
	return 0;
}

/** Update the threads number, should be called on each task's placement or destroy made outside the DQDT*/
void dqdt_update_threads_number(uint_t cid, uint_t core_index, sint_t count)
{
	/**/
}

uint_t current_local_cpu_id;
void dqdt_init(struct boot_info_s *info)
{
	current_local_cpu_id = 0;
}

/** Ask the DQDT for a thread placement decision */
error_t dqdt_thread_placement(struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr)
{
	static uint_t next_cpu = 0;
	uint_t cpu_nr    = current_cluster->cpu_nr;
	attr->cid	 = current_cid;
	attr->cpu_id     = (current_local_cpu_id + next_cpu++) % cpu_nr;
	return 0;
}

/** Ask the DQDT for a target decision for a thread migration */
error_t dqdt_thread_migrate  (struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr)
{
	static uint_t next_cpu = 0;
	uint_t cpu_nr    = current_cluster->cpu_nr;
	attr->cid	 = current_cid;
	attr->cpu_id     = (current_local_cpu_id + next_cpu++) % cpu_nr;
	return 0;

}

/** Ask the DQDT for a task placement decision */
error_t dqdt_task_placement(struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr)
{
	static uint_t next_cpu = 0;
	uint_t cpu_nr    = current_cluster->cpu_nr;
	attr->cid	 = current_cid;
	attr->cpu_id     = (current_local_cpu_id + next_cpu++) % cpu_nr;
#if CONFIG_EXEC_LOCAL
        attr->cid_exec   = current_cid;
#else
        attr->cid_exec   = (current_cid+1)%KERNEL_TOTAL_NR; /* Round-Robin allocation */
#endif
	return 0;
}

/** Ask the DQDT for a cluster decision for a memory allocation */
error_t dqdt_mem_request(struct dqdt_cluster_s *logical, struct dqdt_attr_s *attr)
{
		attr->cid	 = current_cid;;
		attr->cpu_id     = 0;
		return 0;
}

/** Get the logical cluster of the given level */
//struct dqdt_cluster_s* dqdt_logical_lookup(uint_t level);

#endif
