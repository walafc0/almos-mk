/*
 * arch_init.c - architecture intialization operations (see kern/hal-arch.h)
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
#include <types.h>
#include <hardware.h>
#include <cpu.h>
#include <device.h>
#include <driver.h>
#include <list.h>
#include <pic.h>
#include <tty.h>
#include <scheduler.h>
#include <kdmsg.h>
#include <devfs.h>
#include <cluster.h>
#include <rt_timer.h>
#include <pci-bus.h>
#include <kmem.h>
#include <ata.h>

/* TODO: dynamicly calculate this value on each icu bind operation */
#define ICU_MASK     0x3  // interrupt enabled for frist 2 devices 

struct device_s __sys_dma;
struct device_s __fb_screen;
struct device_s __sys_blk;
struct device_s rt_timer;

static void ioCluster_init(struct cluster_s *cluster, uint_t id)
{
	struct device_s *pic;
	int i;

	/* First of all: Initialize TTYs */
	ibmpc_tty_init(&ttys_tbl[0], __tty_addr, 1, 0);   // IRQ 0 is taken by Timer 0

	boot_dmsg("\nSetup Terminal \t\t\t\tOK\n");
   
	cluster_init(cluster, id, __CPU_NR);

	boot_dmsg("Setup PIC      ");
	pic = kbootMem_calloc(sizeof(*pic));
	ibmpc_pic_init(pic, __pic_addr, 0);
	ibmpc_pic_bind(pic, &ttys_tbl[0]);
	devfs_register(&ttys_tbl[0]);

	for(i=0; i < __CPU_NR; i++)
		arch_cpu_set_irq_entry(&cluster->cpu_tbl[i], 0, &pic->action);

	boot_dmsg("\t\t\t\tOK\nSetup Timer    ");
	rt_timer_init(TIC, 1);
	ibmpc_pic_bind(pic, &rt_timer);

	boot_dmsg("\t\t\t\tOK\nSetup H.D.D    ");
	ibmpc_ata_init(&__sys_blk, (void*)ATA0_DRIVE0, 14);
	ibmpc_pic_bind(pic, &__sys_blk);

	boot_dmsg("\t\t\t\tOK\nActivating IRQs");
	pic->op.icu.set_mask(pic, ICU_MASK, 0, 0);
	boot_dmsg("\t\t\t\tOK\n");
}


error_t arch_init(void)
{
	kdmsg_init();
	ioCluster_init(&clusters_tbl[IO_CLUSTER_ID], IO_CLUSTER_ID);

	return 0;
}
