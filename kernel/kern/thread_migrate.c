/*
 * kern/thread_migrate.c - thread migration logic
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

#include <types.h>
#include <errno.h>
#include <kmem.h>
#include <kdmsg.h>
#include <kmagics.h>
#include <cpu.h>
#include <thread.h>
#include <task.h>
#include <list.h>
#include <scheduler.h>
#include <spinlock.h>
#include <cluster.h>
#include <ppm.h>
#include <pmm.h>
#include <dqdt.h>
#include <page.h>

typedef struct
{
	volatile error_t    err;
	struct thread_s     *victim;
	struct task_s       *task;
	struct cpu_s        *ocpu;
	void                *sched_listner;
	uint_t              sched_event;
	uint_t              event_tm;
	struct event_s      event;
}th_migrate_info_t;

error_t do_migrate(th_migrate_info_t *info);

EVENT_HANDLER(migrate_event_handler)
{
	th_migrate_info_t *rinfo;
	th_migrate_info_t linfo;
	error_t err;
	uint_t tm_start;
	uint_t tm_end;
	uint_t tid;
	uint_t pid;

	tm_start     = cpu_time_stamp();
	rinfo        = event_get_argument(event);
	linfo.victim = rinfo->victim;
	linfo.task   = rinfo->task;
	linfo.ocpu   = rinfo->ocpu;
	tid          = linfo.victim->info.order;
	pid          = linfo.task->pid;

	cpu_wbflush();

	err             = do_migrate(&linfo);
	tm_end          = cpu_time_stamp();
	rinfo->err      = err;
	rinfo->event_tm = tm_end - tm_start;
	cpu_wbflush();

#if CONFIG_USE_SCHED_LOCKS
	sched_wakeup(linfo.victim);
#else
	sched_event_send(rinfo->sched_listner, rinfo->sched_event);
#endif
	return 0;
}

/*
 * Hypothesis:
 * 1) This function is executed by the server.
 * 2) This function is executed by the victim thread.
 * 3) The migration decision has been already made.
 *
 * On Error:
 * 1) The victim thread remainses unchagned.
 *
 * TODO: verify the compatiblity with !CONFIG_REMOTE_THREAD_CREATE
*/
error_t thread_migrate(struct thread_s *this, sint_t target_gid)
{
	register uint_t cid, lid;
	register uint_t tm_start;
	register uint_t tm_end;
	struct dqdt_cluster_s *logical;
	//struct cluster_s *cluster;
	struct dqdt_attr_s attr;
	struct task_s *task;
	struct cpu_s *cpu;
	th_migrate_info_t info;
	uint_t tid,pid;
	uint_t state;
	error_t err;

	task    = this->task;
	cpu     = current_cpu;
	tid     = this->info.order;
	pid     = task->pid;

	tm_start = cpu_time_stamp();

	if(target_gid < 0)
	{
		dqdt_attr_init(&attr, NULL);

		logical = cpu->cluster->levels_tbl[0];

		err = dqdt_thread_migrate(logical, &attr);

		if((err) || (attr.cpu_id == cpu->lid && attr.cid == current_cid))
		{
			this->info.migration_fail_cntr ++;
			return EAGAIN;
		}
	}
	else
	{
		lid             = arch_cpu_lid(target_gid);
		cid             = arch_cpu_cid(target_gid);
		attr.cpu_id     = lid;
		attr.cid	= cid;
		attr.tm_request = 0;
	}

	cpu_disable_all_irq(&state);

	if(cpu->fpu_owner == this)
	{
		cpu_fpu_context_save(&this->uzone);
		cpu->fpu_owner = NULL;
		cpu_fpu_disable();
	}

	cpu_restore_irq(state);

	err = cpu_context_save(&this->info.pss);

	if(err != 0) /* DONE */
	{
		if((task->threads_count == 1) &&
		   (current_cluster->id != cpu->cluster->id) &&
		   (this->info.attr.flags & PT_ATTR_AUTO_MGRT))
		{
			vmm_set_auto_migrate(&task->vmm, task->vmm.data_start, MGRT_STACK);
		}

		return 0;
	}

	info.victim        = this;
	info.task          = task;

#if !(CONFIG_USE_SCHED_LOCKS)
	info.sched_listner = sched_get_listner(this, SCHED_OP_WAKEUP);
	info.sched_event   = sched_event_make (this, SCHED_OP_WAKEUP);
#endif

	event_set_argument(&info.event, &info);
	event_set_handler(&info.event, migrate_event_handler);
	event_set_priority(&info.event, E_MIGRATE);

	info.ocpu = cpu;

	thread_set_exported(this);

	event_send(&info.event, arch_cpu_gid(attr.cid, attr.cpu_id));
 
	sched_sleep(this);

	err = info.err;

	if(err)
	{
		this->info.migration_fail_cntr ++;
		thread_clear_exported(this);
		return err;
	}

	tm_end = cpu_time_stamp();

#if CONFIG_SHOW_MIGRATE_MSG
	printk(INFO,
	       "INFO: pid %d, tid %d has been migrated from "
	       "[cid %d, cpu %d] to [cid %d, cpu %d] [e:%u, d:%u, t:%u, r:%u]\n",
	       pid,
	       tid,
	       cpu->cluster->id,
	       cpu->lid,
	       attr.cid,
	       attr.cpu_id,
	       tm_end,
	       attr.tm_request,
	       tm_end - tm_start,
	       info.event_tm);
#endif

	sched_remove(this);
	return 0;
}

error_t do_migrate(th_migrate_info_t *info)
{
	kmem_req_t req;
	struct page_s *page;
	struct cpu_s *cpu;
	struct thread_s *new;
	struct task_s *task;
	struct thread_s *victim;
	error_t err;

	task      = info->task;
	victim    = info->victim;
	cpu       = current_cpu;

	dqdt_update_threads_number(cpu->cluster->id, cpu->lid, 1);

	req.type  = KMEM_PAGE;
	req.size  = 0;
	req.flags = AF_KERNEL;
	page      = kmem_alloc(&req);

	if(page == NULL)
	{
		err = ENOMEM;
		goto fail_nomem;
	}

	new = ppm_page2addr(page);
  
	/* TODO: Review locking */
	spinlock_lock(&task->th_lock);
	spinlock_lock(&victim->lock);

	/* TODO: inform low layers about this event */
	page_copy(page, victim->info.page);

	thread_set_origin_cpu(new,info->ocpu);
	thread_set_current_cpu(new,cpu);
	sched_setpolicy(new, new->info.attr.sched_policy);

	err = sched_register(new);
	/* FIXME: Redo registeration or/and reask 
	 * for a target core either directly (dqdt)
	 * or by returning EAGIN so caller can redo
	 * the whole action.*/
	assert(err == 0);

	list_add_last(&task->th_root, &new->rope);
	list_unlink(&victim->rope);
	task->th_tbl[new->info.order] = new;

//TODO:set the uzone DATA PADDR EXT!
//It should be done by the HAL code 
	new->info.attr.cid     = cpu->cluster->id;
	new->info.attr.cpu_lid = cpu->lid;
	new->info.attr.cpu_gid = cpu->gid;
	new->info.attr.tid     = (uint_t)  new;
	new->info.kstack_addr  = (uint_t*) new;
	new->info.page         = page;

	wait_queue_init2(&new->info.wait_queue, &victim->info.wait_queue);

	spinlock_unlock(&victim->lock);
	spinlock_unlock(&new->lock);
	spinlock_unlock(&task->th_lock);

	cpu_context_set_tid(&new->info.pss, (reg_t)new);
	cpu_context_dup_finlize(&new->pws, &new->info.pss);

	thread_set_imported(new);
	thread_clear_exported(new);
	/* TODO: put a threshold on the migration number to prevent against permanent migration */
	new->info.migration_cntr ++;
	sched_add(new);
	return 0;

fail_nomem:
	dqdt_update_threads_number(cpu->cluster->id, cpu->lid, -1);
	return err;
}

