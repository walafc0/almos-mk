/*
 * soclib_iopic.c - soclib iopic driver
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
#include <system.h>
#include <cpu.h>
#include <device.h>
#include <driver.h>
#include <drvdb.h>
#include <kmem.h>
#include <soclib_iopic.h>
#include <errno.h>
#include <string.h>
#include <cpu-trace.h>
#include <thread.h>
#include <task.h>
#include <vm_region.h>
#include <pmm.h>
#include <vfs.h>

#define MSB(paddr) ((uint_t)((paddr)>>32))
#define LSB(paddr) ((uint_t)(paddr))

static uint_t iopic_count = 0;

error_t soclib_iopic_init(struct device_s *iopic)
{
	iopic->type = DEV_INTERNAL;

	iopic->action.dev         = iopic; 
	iopic->action.data        = NULL;

	iopic->op.iopic.bind_wti_and_irq = &iopic_bind_wti_and_irq;
	iopic->op.iopic.get_status = &iopic_get_status;
	iopic->op.drvid = SOCLIB_IOPIC_ID;
   
	sprintk(iopic->name, "iopic%d", iopic_count++);//actually only 1 iopic can be handled until now
	metafs_init(&iopic->node, iopic->name);
  
	return 0;
}

void iopic_bind_wti_and_irq(struct device_s *iopic, struct device_s *dev)
{
	uint_t cid;
	uint_t *reg;
	uint_t channel;
	uint64_t mailbox;

	cid = iopic->cid;
	channel = dev->irq;
	mailbox = dev->mailbox;
	reg = (uint_t *)(iopic->base + (channel*IOPIC_SPAN*4));

	//boot_dmsg("%s : Bind IOPIC channel %d (reg = %d:0x%x) with mailbox 0x%x.%x\n", 
	//		__FUNCTION__, channel, cid, reg, MSB(mailbox), LSB(mailbox));

	remote_sw(&reg[IOPIC_ADDRESS], cid, LSB(mailbox));
	remote_sw(&reg[IOPIC_EXTEND], cid, MSB(mailbox));
}

void iopic_get_status(struct device_s *iopic, uint_t wti_channel, uint_t *status)
{
	uint_t cid;
	uint_t *reg;

	cid	= iopic->cid;
	reg     = (uint_t *)(iopic->base + wti_channel*IOPIC_SPAN);

	*status = remote_lw(&reg[IOPIC_STATUS], cid);
}

driver_t soclib_iopic_driver = { .init = &soclib_iopic_init };

