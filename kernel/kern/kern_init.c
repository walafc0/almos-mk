/*
 * kern/kern_init.c - kernel parallel initialization
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

#include <almOS-date.h>
#include <types.h>
#include <system.h>
#include <kdmsg.h>
#include <cpu.h>
#include <atomic.h>
#include <mcs_sync.h>
#include <list.h>
#include <thread.h>
#include <scheduler.h>
#include <kmem.h>
#include <config.h>
#include <task.h>
#include <cluster.h>
#include <devfs.h>
#include <sysfs.h>
#include <string.h>
#include <ppm.h>
#include <page.h>
#include <sysconf.h>
#include <device.h>
#include <boot-info.h>
#include <dqdt.h>
#include <pid.h>

#define BOOT_SIGNAL  0xA5A5B5B5
#define die(args...) do {boot_dmsg(args); while(1);} while(0)

//static 
kthread_args_t idle_args;

volatile uint_t local_boot_signal;

static void print_boot_banner(void);

void vfs_init();

void kern_init (boot_info_t *info)
{
	register struct cpu_s *cpu;
	register struct cluster_s *cluster;
	register uint_t cpu_lid;
	register uint_t cluster_id;
	struct task_s *task0;
	struct thread_s *thread;
	struct thread_s thread0;
	uint_t retval;
	uint_t i;
	//error_t err;
	//uint_t tm_start;

	cluster_id = info->local_cluster_id;
	cpu_lid    = info->local_cpu_id;
        task0      = task_lookup_zero();

        if ( task0 == NULL )
                PANIC("Failed to lookup task0 !\n");

	memset(&thread0, 0, sizeof(thread0));
	cpu_set_current_thread(&thread0);
	thread0.type  = TH_BOOT;
	thread0.state = S_KERNEL;
	thread0.task  = task0;
	
	if(info->boot_signal)
		info->boot_signal(info);

	//all wait except the last proc 
	//who incremented the boot_cntr
	if(info->main_cpu_local_id != cpu_lid)
	{
		/* Wait for boot-signal from bootstrap CPU of local cluster */
		while(local_boot_signal != BOOT_SIGNAL);
		retval = cpu_time_stamp() + 2000;
		while(cpu_time_stamp() < retval)
			;
	}else
	{
		task_bootstrap_init(info);
		task_manager_init();  
		devfs_root_init();
		sysfs_root_init();

		//initialise the cluster, cpus and the mem allocator and devices
		//also current_cluster
		//arch_memory_init(info);
		arch_init(info);

		//now the tasks can allocate memory, if they need
		task_bootstrap_finalize(info);
		clusters_sysfs_register();
		kdmsg_init();
                task_manager_init_finalize();
		vfs_init();
		sysconf_init();

		dqdt_init(info); 

#if 0
		if(cluster_id == info->boot_cluster_id)
		{
			printk(INFO, "INFO: Building Distributed Quaternary Decision Tree (DQDT)\n");

			tm_start = cpu_time_stamp();

			err = arch_dqdt_build(info);

			if(err)
				PANIC("Failed to build DQDT, err %d\n", err);

			printk(INFO, "INFO: DQDT has been built [%d]\n", cpu_time_stamp() - tm_start);
		}
#endif




		//the task is already replicated
		//err = task_bootstrap_replicate(info);

		cluster = current_cluster;
		cluster->task = task0;

		idle_args.val[0] = info->reserved_start;
		idle_args.val[1] = info->reserved_end;
		idle_args.val[2] = cpu_get_id();  
		///////////////////////////////////
		for(i = 0; i < cluster->cpu_nr; i++)
		{
			thread = kthread_create(cluster->task, &thread_idle, &idle_args, i);
			
			printk(INFO, "INFO: Creating thread idle %x on cpu %d [ %d ]\n", thread, cpu_get_id(), cpu_time_stamp());

			thread->state = S_KERNEL;
			thread->type  = TH_IDLE;
			cpu = &cluster->cpu_tbl[i];
			cpu_set_thread_idle(cpu,thread);
		}
		///////////////////////////////////

		cluster_wait_init(info);

		if(info->boot_cluster_id == cluster_id)
		{
			printk(INFO, "All clusters have been Initialized [ %d ]\n", cpu_time_stamp());
			print_boot_banner();
		}

		local_boot_signal = BOOT_SIGNAL;
	}

	cluster = current_cluster;
	cpu = &cluster->cpu_tbl[cpu_lid];
	thread = cpu_get_thread_idle(cpu);

	cpu_context_load(&thread->pws);
}


#if CONFIG_SHOW_BOOT_BANNER
static void print_boot_banner(void)
{ 
        printk(BOOT,"\n           ____        ___        ___       ___    _______    ________         ___       ___   ___     ___   \n");
        printk(BOOT,"          /    \\      |   |      |   \\     /   |  /  ___  \\  /  ____  |       |   \\     /   | |   |   /  / \n");
        printk(BOOT,"         /  __  \\      | |        |   \\___/   |  |  /   \\  | | /    |_/        |   \\___/   |   | |   /  /  \n");
        printk(BOOT,"        /  /  \\  \\     | |        |  _     _  |  | |     | | | |______   ___   |  _     _  |   | |__/  /     \n");
        printk(BOOT,"       /  /____\\  \\    | |        | | \\   / | |  | |     | | \\______  \\ |___|  | | \\   / | |   |  __  <  \n");
        printk(BOOT,"      /   ______   \\   | |     _  | |  \\_/  | |  | |     | |  _     | |        | |  \\_/  | |   | |  \\  \\  \n");
        printk(BOOT,"     /   /      \\   \\  | |____/ | | |       | |  |  \\___/  | | \\____/ |        | |       | |   | |   \\  \\\n");
        printk(BOOT,"    /_____/    \\_____\\|_________/|___|     |___|  \\_______/  |________/       |___|     |___| |___|   \\__\\\n");


        printk(BOOT,"\n\n\t\t\t\t Advanced Locality Management Operating System\n");
        printk(BOOT,"\t\t\t\t   UPMC/LIP6/SoC (%s)\n\n\n",ALMOS_DATE);
        //printk(BOOT,"\t\t\tUPMC/LIP6/SoC (%s)\n\n\n",ALMOS_DATE);
}
#else
static void print_boot_banner(void)
{
}
#endif
