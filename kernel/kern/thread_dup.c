/*
 * kern/thread_dup.c - duplicate a thread
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

error_t thread_dup(struct task_s *task, 
		   struct thread_s *dst, 
		   struct cpu_s *dst_cpu, 
		   struct cluster_s *dst_clstr,
		   struct thread_s *src)
{
	register uint_t sched_policy;
	register uint_t cpu_lid;
	register uint_t cid;
	struct page_s *page;

	sched_policy = sched_getpolicy(src);
	cpu_lid      = dst_cpu->lid;
	cid          = dst_clstr->id;
	page         = dst->info.page;

	// Duplicate
	page_copy(page, src->info.page);

	// Initialize dst thread
	spinlock_init(&dst->lock, "Thread");
	dst->flags                           = 0;
	thread_clear_joinable(dst);
	dst->locks_count                     = 0;
	dst->ticks_nr                        = 0;
	thread_set_current_cpu(dst, dst_cpu);
	sched_setpolicy(dst, sched_policy);
	dst->task                            = task;
	dst->type                            = PTHREAD;
	dst->info.sched_nr                   = 0;
	dst->info.ppm_last_cid               = cid;
	dst->info.migration_cntr             = 0;
	dst->info.migration_fail_cntr        = 0;
	dst->info.tm_exec                    = 0;
	dst->info.tm_tmp                     = 0;
	dst->info.tm_usr                     = 0;
	dst->info.tm_sys                     = 0;
	dst->info.tm_sleep                   = 0;
	dst->info.tm_wait                    = 0;
	signal_init(dst);
	dst->info.join                       = NULL;
	wait_queue_init(&dst->info.wait_queue, "Join/Exit Sync");
	dst->info.attr.sched_policy          = sched_policy;
	dst->info.attr.cid                   = cid;
	dst->info.attr.cpu_lid               = cpu_lid;
	dst->info.attr.cpu_gid               = dst_cpu->gid;
	dst->info.attr.tid                   = (uint_t) dst;
	dst->info.attr.pid                   = task->pid;
	dst->info.kstack_addr                = (uint_t*)dst;
	dst->info.page                       = page;
	dst->signature                       = THREAD_ID;
	
	return 0;
}
