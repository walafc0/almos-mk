/*
 * kern/kthread_create.c - kernel-level threads creation
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

#include <kmem.h>
#include <cpu.h>
#include <thread.h>
#include <task.h>
#include <list.h>
#include <ppm.h>
#include <pmm.h>
#include <signal.h>
#include <scheduler.h>
#include <spinlock.h>
#include <cluster.h>

static void kthread_exit(void)
{
	struct thread_s *this;
  
	this = current_thread;
	
	thread_dmsg(1, "Kernel Thread %x on CPU %d has finished\n",
	       this, 
	       current_cpu->gid);
	
	sys_thread_exit(0);//sched_exit(this);
	
	PANIC ("Thread %x, CPU %d must never return", this, current_cpu->gid);
}

void thread_init(struct thread_s *thread);

struct thread_s* kthread_create(struct task_s *task, kthread_t *kfunc, void *arg, uint_t cpu_lid)
{
	kmem_req_t req;
	register struct page_s *page;
	register struct thread_s *thread;
  
	req.type  = KMEM_PAGE;
	req.size  = ARCH_THREAD_PAGE_ORDER;
	req.flags = AF_KERNEL | AF_ZERO | AF_REMOTE;
	//req.ptr   = clusters_tbl[cid].cluster;  
	req.ptr   = current_cluster;  
	page      = kmem_alloc(&req);
  
	if(page == NULL) return NULL;

	thread = ppm_page2addr(page);

	thread_init(thread);

	//thread_set_current_cpu(thread, &clusters_tbl[cid].cluster->cpu_tbl[cpu_lid]);
	thread_set_current_cpu(thread, &current_cluster->cpu_tbl[cpu_lid]);
	sched_setpolicy(thread, SCHED_RR);
	
	thread->task                 = task;
	thread->type                 = KTHREAD;
	thread->state		     = S_KERNEL;
	thread->info.sig_mask        = SIG_DEFAULT_MASK;
	thread->info.attr.entry_func = (void*)kfunc;
	thread->info.attr.exit_func  = (void*)kthread_exit;
	thread->info.attr.arg1       = arg;
	thread->info.attr.tid        = (uint_t) thread;
	thread->info.attr.pid        = task->pid;
	thread->info.page            = page;
  
	cpu_context_init(&thread->pws, thread);
	return thread;
}
