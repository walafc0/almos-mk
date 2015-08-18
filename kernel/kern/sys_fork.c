/*
 * kern/sys_fork.c - fork the current process
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

#include <errno.h>
#include <config.h>
#include <cpu.h>
#include <cluster.h>
#include <event.h>
#include <list.h>
#include <thread.h>
#include <scheduler.h>
#include <kmem.h>
#include <dqdt.h>
#include <task.h>
#include <pid.h>

typedef struct 
{
	volatile bool_t   isDone;
	volatile error_t  err;
	struct thread_s  *this_thread;
	struct task_s    *this_task;
        struct cluster_s *current_clstr;
	struct cpu_s     *cpu;
	struct event_s    event;
	struct thread_s  *child_thread;
	struct task_s    *child_task;
	uint_t           flags;
	uint_t           tm_event;
	bool_t           isPinned;
        cid_t            cid_exec;

}fork_info_t;

static error_t do_fork(fork_info_t *info);

EVENT_HANDLER(fork_event_handler)
{
	fork_info_t *rinfo;
	fork_info_t linfo;
	error_t err;
	register uint_t tm_start, tm_end;

	tm_start = cpu_time_stamp();

	fork_dmsg(1, "%s: cpu %d, started [%d]\n",
		  __FUNCTION__,
		  cpu_get_id(),
		  tm_start);

	rinfo = event_get_argument(event);

	linfo.this_thread  = rinfo->this_thread;
	linfo.this_task    = rinfo->this_task;
	linfo.cpu          = rinfo->cpu;
	linfo.child_thread = NULL;
	linfo.child_task   = NULL;
	linfo.isPinned     = rinfo->isPinned;
	linfo.flags        = rinfo->flags;

	err = do_fork(&linfo);

	tm_end = cpu_time_stamp();

	rinfo->child_thread = linfo.child_thread;
	rinfo->child_task   = linfo.child_task;
	rinfo->err          = err;
	rinfo->tm_event     = tm_end - tm_start;

	cpu_wbflush();

	rinfo->isDone = true;

	cpu_wbflush();
	return 0;
}

int sys_fork(uint_t flags, uint_t cpu_gid)
{
	fork_info_t info;
	struct dqdt_attr_s attr;
	struct thread_s *this_thread;
	struct task_s *this_task;
	struct thread_s *child_thread;
	struct task_s *child_task;
	uint_t irq_state;
	uint_t cpu_lid;
	uint_t cid;
	error_t err;
	uint_t tm_start;
	uint_t tm_end;
	uint_t tm_bRemote;
	uint_t tm_aRemote;

	tm_start = cpu_time_stamp();

	fork_dmsg(1, "%s: cpu %d, started [%d]\n",
		  __FUNCTION__, 
		  cpu_get_id(),
		  tm_start);

	this_thread = current_thread;
	this_task   = this_thread->task;
	info.current_clstr = current_cluster;

	err = atomic_add(&this_task->childs_nr, 1);
  
	if(err >= CONFIG_TASK_CHILDS_MAX_NR)
	{
		err = EAGAIN;
		goto fail_childs_nr;
	}

	fork_dmsg(1, "%s: task of pid %d can fork a child [%d]\n",
		  __FUNCTION__, 
		  this_task->pid,
		  cpu_time_stamp());

	info.isDone      = false;
	info.this_thread = this_thread;
	info.this_task   = this_task;
	info.flags       = flags;

	cpu_disable_all_irq(&irq_state);
	cpu_restore_irq(irq_state);
  
	if(current_cpu->fpu_owner == this_thread)
	{
		fork_dmsg(1, "%s: going to save FPU\n", __FUNCTION__);
		cpu_fpu_context_save(&this_thread->uzone);
	}

	if(flags & PT_FORK_USE_TARGET_CPU)
	{
		cpu_gid       = cpu_gid % arch_onln_cpu_nr();
		cpu_lid       = arch_cpu_lid(cpu_gid);
		cid           = arch_cpu_cid(cpu_gid);
		attr.cid      = cid;
		attr.cpu_id   = arch_cpu_lid(cpu_gid);
		info.isPinned = true;
	}
	else
	{
		info.isPinned = false;
		dqdt_attr_init(&attr, NULL);
		err = dqdt_task_placement(dqdt_root, &attr);
	}

        info.cpu = cpu_lid2ptr(attr.cpu_id);
        info.cid_exec = attr.cid_exec;

        /* Keeps the first two processes on current cluster. This is used by cluster zero to keep
         * the "sh" process on this cluster. Init is forced on current_cluster in the
         * task_load_init() function.
         */
        if ( this_task->pid < PID_MIN_GLOBAL+2 )
                info.cid_exec = current_cid;

	fork_dmsg(1, "%s: new task will be placed on cluster %d, cpu %d. Task will be moved on cluster %u on exec()\n", \
                        __FUNCTION__, attr.cid, attr.cpu_id, info.cid_exec);

	tm_bRemote = cpu_time_stamp();
	err = do_fork(&info);
	tm_aRemote = cpu_time_stamp();

	if(err)
		goto fail_do_fork;

	child_thread = info.child_thread;
	child_task   = info.child_task;

	spinlock_lock(&this_task->lock);

	list_add(&this_task->children, &child_task->list);
	spinlock_unlock(&this_task->lock);

	fork_dmsg(1, "%s: childs (task & thread) have been registered in their parents lists [%d]\n", 
		  __FUNCTION__, 
		  cpu_time_stamp());
  
	fork_dmsg(1, "%s: going to add child to target scheduler\n", __FUNCTION__);
	sched_add_created(child_thread);
	tm_end = cpu_time_stamp();
    
	fork_dmsg(1, "%s: cpu %d, pid %d, done [s:%u, bR:%u, aR:%u, e:%u, d:%u, t:%u, r:%u]\n",
	       __FUNCTION__,
	       cpu_get_id(),
	       this_task->pid,
	       tm_start,
	       tm_bRemote,
	       tm_aRemote,
	       tm_end,
	       attr.tm_request,
	       tm_end - tm_start,
	       info.tm_event);

	return child_task->pid;

fail_do_fork:
fail_childs_nr:
	atomic_add(&this_task->childs_nr, -1);
	this_thread->info.errno = err;
	return -1;
}

error_t do_fork(fork_info_t *info)
{
	kmem_req_t req;
	struct dqdt_attr_s attr;
	struct thread_s *child_thread;
	struct task_s *child_task;
	struct page_s *page;
	uint_t cid;
	error_t err;
	sint_t order;
  
	fork_dmsg(1, "%s: cpu %d, started [%d]\n", 
		  __FUNCTION__, 
		  cpu_get_id(),
		  cpu_time_stamp());
  
	child_thread = NULL;
	child_task   = NULL;
	page         = NULL;
	cid	      = info->cpu->cluster->id;
	attr.cid      = cid;
	attr.cpu_id   = 0;
        attr.cid_exec = info->cid_exec;

	//dqdt_update_threads_number(attr.cluster->levels_tbl[0], attr.cpu->lid, 1);
	dqdt_update_threads_number(cid, attr.cpu_id, 1);

        //attr.cluster = info->current_clstr;
        attr.cid = cid;
	err = task_create(&child_task, &attr, CPU_USR_MODE);
  
        //attr.cluster = info->cpu->cluster;
        attr.cid = cid;

	if(err) goto fail_task;

	fork_dmsg(1, "%s: cpu %d, ppid %d, task @0x%x, pid %d, task @0x%x [%d]\n",
		  __FUNCTION__, 
		  cpu_get_id(), 
		  info->this_task->pid, 
		  info->this_task,
		  child_task->pid,
		  child_task,
		  cpu_time_stamp());
  
	req.type  = KMEM_PAGE;
	req.size  = ARCH_THREAD_PAGE_ORDER;
	req.flags = AF_KERNEL | AF_REMOTE;
	req.ptr   = info->cpu->cluster;
	req.ptr   = info->current_clstr;

	page = kmem_alloc(&req);

	if(page == NULL) 
		goto fail_mem;

	fork_dmsg(1, "%s: child pid will be %d on cluster %d, cpu %d [%d]\n", 
		  __FUNCTION__, 
		  child_task->pid, 
		  child_task->cpu->cluster->id, 
		  child_task->cpu->gid,
		  cpu_time_stamp());

	err = task_dup(child_task, info->this_task);
  
	if(err) goto fail_task_dup;

	signal_manager_destroy(child_task);
	signal_manager_init(child_task);
  
	fork_dmsg(1, "%s: parent task has been duplicated [%d]\n", 
		  __FUNCTION__, 
		  cpu_time_stamp());

	child_task->current_clstr = info->current_clstr;

	err = vmm_dup(&child_task->vmm, &info->this_task->vmm);

	if(err) goto fail_vmm_dup;
  
	fork_dmsg(1, "%s: parent vmm has been duplicated [%d]\n", 
		  __FUNCTION__, 
		  cpu_time_stamp());

	child_thread = (struct thread_s*) ppm_page2addr(page);

	/* Set the child page before calling thread_dup */
	child_thread->info.page = page;

	err = thread_dup(child_task,
			 child_thread,
			 info->cpu,
			 info->cpu->cluster,
			 info->this_thread);

	if(err) goto fail_thread_dup;

	/* Adjust child_thread attributes */
	if(info->flags & PT_FORK_USE_AFFINITY)
	{
		child_thread->info.attr.flags |= (info->flags & ~(PT_ATTR_LEGACY_MASK));

		if(!(info->flags & PT_ATTR_MEM_PRIO))
			child_thread->info.attr.flags &= ~(PT_ATTR_MEM_PRIO);

		if(!(info->flags & PT_ATTR_AUTO_MGRT))
			child_thread->info.attr.flags &= ~(PT_ATTR_AUTO_MGRT);

		if(!(info->flags & PT_ATTR_AUTO_NXTT))
			child_thread->info.attr.flags &= ~(PT_ATTR_AUTO_NXTT);
	}

	fork_dmsg(1, "%s: parent current thread has been duplicated, tid %x [%d]\n",
		  __FUNCTION__, 
		  child_thread, 
		  cpu_time_stamp());
	
	if(info->isPinned)
		thread_migration_disabled(child_thread);
	else
		thread_migration_enabled(child_thread);
	
	list_add_last(&child_task->th_root, &child_thread->rope);
	child_task->threads_count = 1;
	child_task->threads_nr ++;
	child_task->state = TASK_READY;

	order = bitmap_ffs2(child_task->bitmap, 0, sizeof(child_task->bitmap));

	if(order == -1) goto fail_order;

	bitmap_clear(child_task->bitmap, order);
	child_thread->info.attr.key = order;
	child_thread->info.order = order;
	child_task->next_order = order + 1;
	child_task->max_order = order;
	child_task->uid = info->this_task->uid;
	child_task->parent = info->this_task->pid;

	err = sched_register(child_thread);
  
	assert(err == 0);
    
	cpu_context_set_tid(&child_thread->info.pss, (reg_t)child_thread);
	cpu_context_set_pmm(&child_thread->info.pss, &child_task->vmm.pmm);
	cpu_context_dup_finlize(&child_thread->pws, &child_thread->info.pss);
  
	child_thread->info.retval = 0;
	child_thread->info.errno = 0;

	info->child_thread = child_thread;
	info->child_task = child_task;
	return 0;

fail_order:
fail_thread_dup:
fail_vmm_dup:
fail_task_dup:
	printk(WARNING, "WARNING: %s: destroy child thread\n", __FUNCTION__);
	req.ptr = page;
	kmem_free(&req);

fail_mem:
fail_task:
	//FIXME
	//dqdt_update_threads_number(attr.cluster->levels_tbl[0], attr.cpu->lid, -1);
	dqdt_update_threads_number(attr.cid, attr.cpu_id, -1);

	printk(WARNING, "WARNING: %s: destroy child task\n", __FUNCTION__);

	if(child_task != NULL)
		task_destroy(child_task);

	printk(WARNING, "WARNING: %s: fork err %d [%d]\n", 
	       __FUNCTION__, 
	       err, 
	       cpu_time_stamp());

	return err;
}
