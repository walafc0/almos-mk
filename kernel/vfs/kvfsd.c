/*
 * vfs/kvfs.c - VFS deamon
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
#include <kdmsg.h>
#include <thread.h>
#include <device.h>
#include <vfs.h>
#include <scheduler.h>
#include <thread.h>
#include <task.h>
#include <cpu-trace.h>
#include <cluster.h>
#include <libk.h>
#include <kmem.h>
#include <rt_timer.h>
#include <vfs-params.h>
#include <sysconf.h>
#include <page.h>
#include <event.h>
#include <dqdt.h>

#define USR_START (CONFIG_USR_START)

#define SYNC_MS_PERIOD 4

extern void* kMiniShelld(void*);

static EVENT_HANDLER(kvfsd_alarm_event_handler)
{
	struct thread_s *kvfsd;

	kvfsd = event_get_senderId(event);
	sched_wakeup(kvfsd);
	return 0;
}

void* kvfsd(void *arg)
{
	uint_t tm_now, cntr;
	struct task_s *task;
	struct thread_s *this;
	struct cpu_s *cpu;
	struct alarm_info_s info;
	struct event_s event;
	uint_t fs_type;
	error_t err;
	
	cpu_enable_all_irq(NULL);

	printk(INFO, "INFO: Starting KVFSD on CPU %d [ %d ]\n", cpu_get_id(), cpu_time_stamp());

	task    = current_task;
	fs_type = VFS_TYPES_NR;

#if CONFIG_ROOTFS_IS_EXT2
	fs_type = VFS_EXT2_TYPE;
#endif
 
#if CONFIG_ROOTFS_IS_VFAT

#if CONFIG_ROOTFS_IS_EXT2
#error More than one root fs has been selected
#endif

	fs_type = VFS_VFAT_TYPE;
#endif  /* CONFIG_ROOTFS_IS_VFAT_TYPE */
  
	err = vfs_mount_fs_root(__sys_blk,
			       fs_type,
			       task);

	printk(INFO, "INFO: Virtual File System (VFS) Is Ready\n");

#if 1
	if(err == 0)
	{
		if((err = task_load_init(task)))
		{
			printk(WARNING, "WARNING: failed to load user process, err %d [%u]\n", 
			       err,
			       cpu_time_stamp());
		}
	}
#else
	err = 1;
#endif

#if CONFIG_DEV_VERSION
	if(err != 0)
	{
		struct thread_s *thread;

		printk(INFO, "INFO: Creating kernel level terminal\n"); 

		thread = kthread_create(task, 
					&kMiniShelld, 
					NULL, 
					current_cpu->lid);
		thread->task = task;
		list_add_last(&task->th_root, &thread->rope);
		err = sched_register(thread);
		assert(err == 0);
		sched_add_created(thread);
	}
#endif

	this = current_thread;
	cpu  = current_cpu;

	event_set_senderId(&event, this);
	event_set_priority(&event, E_FUNC);
	event_set_handler(&event, &kvfsd_alarm_event_handler);
  
	info.event = &event;
	cntr       = 0;

	while(1)
	{
		alarm_wait(&info, SYNC_MS_PERIOD);
		sched_sleep(this);
		tm_now = cpu_time_stamp();

		if(current_cid == arch_boot_cid())
			printk(INFO, "INFO: System Current TimeStamp %u\n", tm_now);
		sync_all_pages();

		if((cntr % 4) == 0)
			dqdt_print_summary(dqdt_root);

		cntr ++;
	}
	return NULL;
}
