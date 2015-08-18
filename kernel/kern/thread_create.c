/*
 * kern/thread_create.c - create new thread descriptor
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
#include <page.h>

const char* const thread_state_name[THREAD_STATES_NR] = 
{
	"S_CREATE",
	"S_USR",
	"S_KERNEL",
	"S_READY",
	"S_WAIT",
	"S_DEAD"
};

const char* const thread_attr_name[THREAD_TYPES_NR] =
{
	"USR",
	"KTHREAD",
	"IDLE"
};

void thread_init(struct thread_s *thread)
{
	spinlock_init(&thread->lock, "Thread");
	thread->info.kstack_addr     = (uint_t*)thread;
	thread->info.kstack_size     = PMM_PAGE_SIZE << ARCH_THREAD_PAGE_ORDER;
	thread->signature            = THREAD_ID;
	thread->info.before_sleep    = NULL;
	thread->info.after_wakeup    = NULL;
}

error_t thread_create(struct task_s *task, pthread_attr_t *attr, struct thread_s **new_thread)
{
	kmem_req_t req;
	struct cluster_s *cluster;
	struct cpu_s *cpu;
	register struct thread_s *thread;
	struct page_s *page;

	//FIXME
	//cluster = cluster_cid2ptr(attr->cid);
	cluster = current_cluster;
	//cpu     = cpu_gid2ptr(attr->cpu_gid);
	cpu     = cpu_lid2ptr(attr->cpu_lid);

	// New Thread Ressources Allocation
	req.type  = KMEM_PAGE;
	req.size  = ARCH_THREAD_PAGE_ORDER;
	req.flags = AF_KERNEL | AF_ZERO | AF_REMOTE;
	req.ptr   = cluster;

#if CONFIG_THREAD_LOCAL_ALLOC
	req.ptr   = current_cluster;
#endif

	page      = kmem_alloc(&req);

	if(page == NULL) return EAGAIN;

	thread = (struct thread_s*) ppm_page2addr(page);

	thread_init(thread);

	// Initialize new thread
	thread_set_current_cpu(thread,cpu);

	sched_setpolicy(thread, attr->sched_policy);
	thread->task = task;
	thread->type = PTHREAD;

	if(attr->flags & PT_ATTR_DETACH)
		thread_clear_joinable(thread);
	else
		thread_set_joinable(thread);

	signal_init(thread);
	attr->tid = (uint_t)thread;
	attr->pid = task->pid;
	memcpy(&thread->info.attr, attr, sizeof(*attr));
	thread->info.page = page;
	thread->info.ppm_last_cid = attr->cid;

	wait_queue_init(&thread->info.wait_queue, "Join/Exit Sync");
	cpu_context_init(&thread->pws, thread); 

	// Set referenced thread pointer to new thread address
	*new_thread = thread;

        thread_dmsg(1, "%s: thread %x of task %u created on cluster %u by cpu %u\n",    \
                        __FUNCTION__, thread->info.attr.tid, thread->info.attr.pid,     \
                        thread->info.attr.cid, thread->info.attr.cpu_lid);

	return 0;
}
