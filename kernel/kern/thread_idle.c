/*
 * kern/thread_idle.c - Default thread executed in the absence of any ready-thread
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
#include <cpu.h>
#include <cluster.h>
#include <atomic.h>
#include <mcs_sync.h>
#include <rt_timer.h>
#include <cpu-trace.h>
#include <kmem.h>
#include <task.h>
#include <vfs.h>
#include <thread.h>
#include <event.h>
#include <ppm.h>
#include <kcm.h>
#include <page.h>
#include <dqdt.h>

//FIXME: it's supposed to be a private variable
struct device_s * __sys_blk;
error_t rpc_test();

void* thread_idle(void *arg)
{
	extern uint_t __ktext_start;
	register uint_t cpu_nr;
	register struct thread_s *this;
	register struct cpu_s *cpu;
	struct thread_s *thread;
	register struct page_s *reserved_pg;
	register uint_t reserved;
	kthread_args_t *args;
	bool_t isBSCPU;
	uint_t tm_now;
	uint_t gid;
	uint_t count;
	error_t err;

	this    = current_thread;
	cpu     = current_cpu;
	gid      = cpu->gid;
	cpu_nr  = arch_onln_cpu_nr();
	args    = (kthread_args_t*) arg;
	isBSCPU = (cpu == cpu->cluster->local_bscpu);


	cpu_trace_write(current_cpu, thread_idle);

	if(isBSCPU)//?
		pmm_tlb_flush_vaddr((vma_t)&__ktext_start, PMM_UNKNOWN);

	cpu_set_state(cpu, CPU_ACTIVE);
	rt_timer_read(&tm_now);
	this->info.tm_born = tm_now;      
	this->info.tm_tmp  = tm_now;
	//// Reset stats /// 
	cpu_time_reset(cpu);
	////////////////////

	printk(INFO, "INFO: Starting Thread Idle On Core %d\tOK\n", cpu->gid);

	if(gid == args->val[2])
	{
		for(reserved = args->val[0]; reserved < args->val[1]; reserved += PMM_PAGE_SIZE)
		{
			reserved_pg = ppm_ppn2page(&cpu->cluster->ppm, ppm_vma2ppn(&cpu->cluster->ppm, (void*)reserved));
			page_state_set(reserved_pg, PGINIT);       
			ppm_free_pages(reserved_pg);
		}
	}

	/** Creating thread event **/
	thread = kthread_create(this->task, 
				&thread_event_manager, 
				NULL, 
				cpu->lid);

	if(thread == NULL)
		PANIC("Failed to create default events handler Thread for CPU %d\n", gid);

	thread->task   = this->task;
	cpu->event_mgr = thread;
	wait_queue_init(&thread->info.wait_queue, "Events");

	err = sched_register(thread);
	assert(err == 0);

	sched_add_created(thread);
	/* Event thread can not be preempted (as for RPC thread) */
	thread_preempt_disable(thread);


	/** Creating thread rpc **/
	if(rpc_thread_create(cpu, this->task))
		PANIC("Failed to create default events handler Thread for CPU %d\n", gid);
	else
		printk(INFO, "INFO: Thread RPC %p Created On Core %d\t [%u]\n", 
						thread, gid, cpu_time_stamp());

	/** Creating Cluster manager and KVFSD threads **/
	if(isBSCPU)
	{
		dqdt_update();
#if 0
		thread = kthread_create(this->task, 
					&cluster_manager_thread,
					cpu->cluster, 
					cpu->cluster->id, 
					cpu->lid);

		if(thread == NULL)
		{
			PANIC("Failed to create cluster manager thread, cid %d, cpu %d\n", 
			      cpu->cluster->id, 
			      cpu->gid);
		}

		thread->task          = this->task;
		cpu->cluster->manager = thread;
		wait_queue_init(&thread->info.wait_queue, "Cluster-Mgr");

		err = sched_register(thread);
		assert(err == 0);

		sched_add_created(thread);

#endif

		//FIXME!
		if(current_cid == arch_boot_cid())//FIXME use __sys_blk->cid_handler!
		{
			thread = kthread_create(this->task, 
						&kvfsd, 
						NULL, 
						cpu->lid);
       
			if(thread == NULL)
			{
				PANIC("Failed to create KVFSD on cluster %d, cpu %d\n", 
				      cpu->cluster->id, 
				      cpu->gid);
			}

			thread->task  = this->task;
			wait_queue_init(&thread->info.wait_queue, "KVFSD");
			err           = sched_register(thread);
			assert(err == 0);
			sched_add_created(thread);
			printk(INFO,"INFO: kvfsd has been created on cpu %d\n", gid);
		}
	}

	cpu_set_state(cpu,CPU_IDLE);

#if CONFIG_USE_RPC_TEST
	rpc_test();
#endif

	while (true)
	{
		cpu_disable_all_irq(NULL);

		__cpu_check_sched(this);
 
		sched_idle(this);

		count = sched_runnable_count(&cpu->scheduler);

		cpu_enable_all_irq(NULL);

		if(count != 0)
			sched_yield(this);
     
		//arch_set_power_state(cpu, ARCH_PWR_IDLE);
	}

	return NULL;
}

