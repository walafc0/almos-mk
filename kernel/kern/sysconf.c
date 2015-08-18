/*
 * kern/sysconf.c - interface to access system exported configurations
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
#include <config.h>
#include <libk.h>
#include <system.h>
#include <sysfs.h>
#include <arch.h>
#include <pmm.h>
#include <thread.h>
#include <sysconf.h>

static sysfs_entry_t sysconf;
static void sysconf_sysfs_op_init(sysfs_op_t *op);

error_t sysconf_init(void)
{
	sysfs_op_t op;

	sysconf_sysfs_op_init(&op);
	sysfs_entry_init(&sysconf, &op, 
#if CONFIG_ROOTFS_IS_VFAT
			 "SYSCONF"
#else
			 "sysconf"
#endif
		);
	sysfs_entry_register(&sysfs_root_entry, &sysconf);

	return 0;
}

static error_t sysconf_sysfs_read_op(sysfs_entry_t *entry, sysfs_request_t *rq, uint_t *offset)
{
	struct thread_s *this;

	if(*offset != 0)
	{
		*offset   = 0;
		rq->count = 0;
		return 0;
	}
  
	this = current_thread;

	sprintk((char*)rq->buffer, 
		"CLUSTER_ONLN_NR %d\nCPU_ONLN_NR %d\nCPU_CACHE_LINE_SIZE %d\nPAGE_SIZE %d\nHEAP_MAX_SIZE %d\n",
		arch_onln_cluster_nr(),
		arch_onln_cpu_nr(), 
		CACHE_LINE_SIZE, 
		PMM_PAGE_SIZE,
		CONFIG_TASK_HEAP_MAX_SIZE);
  
	rq->count = strlen((const char*)rq->buffer); 
	*offset   = 0;
	return 0;
}

static void sysconf_sysfs_op_init(sysfs_op_t *op)
{
	op->open  = NULL;
	op->read  = sysconf_sysfs_read_op;
	op->write = NULL;
	op->close = NULL;
}
