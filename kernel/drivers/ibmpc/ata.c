/*
 * ata.c - ATA hard disk driver
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

#include <device.h>
#include <driver.h>
#include <cpu.h>
#include <thread.h>
#include <scheduler.h>
#include <kmem.h>
#include <sysio.h>
#include <string.h>
#include <event.h>
#include <cpu-trace.h>
#include <cpu-io.h>
#include <ata.h>

#define ATA_BASE(x)  ((x) & 0xFFFF)
#define ATA_DRIVE(x) ((x) >> 16)

static void ata_start_request(struct device_s *dev, dev_request_t *rq, uint32_t type)
{
	uint16_t base  = ATA_BASE((uint32_t)dev->base);
	uint16_t drive = ATA_DRIVE((uint32_t)dev->base);
	uint32_t lba   = (uint32_t)rq->src;

	cpu_io_out8(base + ATA_ERROR_REG, 0x00);
	cpu_io_out8(base + ATA_COUNT_REG, rq->count);
	cpu_io_out8(base + ATA_LBA_LOW_REG, (uint8_t)lba); 
	cpu_io_out8(base + ATA_LBA_MID_REG, (uint8_t)(lba >> 8));
	cpu_io_out8(base + ATA_LBA_HIGH_REG,(uint8_t)(lba >> 16));
	cpu_io_out8(base + ATA_DRIVE_REG, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
	cpu_io_out8(base + ATA_CMD_REG, type);
}

error_t ata_busy_wait(uint_t base)
{
	uint_t val;
	uint_t val2;

	while((val=cpu_io_in8(base + ATA_STATUS_REG)) & ATA_BSY)
		val ++;
  
	val2 = val;

	return val2 - val;
}

static void ata_finalize_request(struct device_s *dev, dev_request_t *rq, uint32_t type)
{
	register uint16_t val;
	register uint16_t base;
	register uint_t index;
	register uint8_t *buffer;  
	error_t err;

	base = ATA_BASE((uint32_t)dev->base);
	buffer = (uint8_t*) rq->dst;

	if((err=ata_busy_wait(base)))
	{
		printk(ERROR, "ERROR: ata_finalize_request: time out [dev %x, cmd %x, start lba %d, count %d]\n", 
		       dev->base, 
		       type, 
		       (uint32_t)rq->src, 
		       rq->count);

		return;
	}

	if(type == ATA_READ_CMD)
	{
		for (index = 0; index < 256 * rq->count; index++) 
		{
			val = cpu_io_in16(base + ATA_DATA_REG);
			buffer[index * 2    ] = (uint8_t) val;
			buffer[index * 2 + 1] = (uint8_t)(val >> 8);
		}
		return;
	}
  
	for (index = 0; index < 256 * rq->count; index++) 
	{
		val = (buffer[index * 2 + 1] << 8) | buffer[index * 2];
		cpu_io_out16(base + ATA_DATA_REG, val);
	}
}

void __attribute__ ((noinline)) ata_irq_handler(struct irq_action_s *action)
{
	register struct device_s *hdd;
	register struct ata_context_s *ctx;
	uint16_t base;
	uint8_t status;

	cpu_trace_write(current_thread()->local_cpu, ata_irq_handler);

	hdd = action->dev;
	base = ATA_BASE((uint32_t)hdd->base);
	ctx = (struct ata_context_s*)hdd->data;

	status = cpu_io_in8(base + ATA_STATUS_REG); /* IRQ ACK */

	isr_dmsg(INFO, "INFO: Recived irq on DevBlk %x, Drive %d, status %x\n", 
		 base, 
		 ATA_DRIVE((uint32_t)hdd->base), 
		 status);
}


int32_t __attribute__ ((noinline)) ata_request(struct device_s *blk_dev, dev_request_t *rq, uint32_t type)
{
	struct ata_context_s *ctx;

	cpu_trace_write(current_thread()->local_cpu, ata_request);
  
	ctx = (struct ata_context_s*)blk_dev->data;

	if((rq->count + ((uint_t)rq->src)) > ctx->params.blk_count)
		return -1;

	ata_start_request(blk_dev, rq, type);
	ata_finalize_request(blk_dev, rq, type);

	return 0;
}


static sint_t ata_read(struct device_s *blk_dev, dev_request_t *rq)
{
	return ata_request(blk_dev, rq, ATA_READ_CMD);
}


static sint_t ata_write(struct device_s *blk_dev, dev_request_t *rq)
{
	return ata_request(blk_dev, rq, ATA_WRITE_CMD);
}


static sint_t ata_get_params(struct device_s *blk_dev, dev_params_t *params) 
{
	struct ata_context_s *ctx = (struct ata_context_s*) blk_dev->data;
	params->blk.count = ctx->params.blk_count;
	params->blk.size = ctx->params.blk_size;
	return 0;
}

void ibmpc_ata_init(struct device_s *hdd, void *base, uint_t irq)
{
	struct ata_context_s *ctx;
	error_t err;
	uint_t index;
	uint16_t *buffer;
	uint16_t dev_base;
	uint16_t drive;

	spinlock_init(&hdd->lock);
	hdd->base = base;
	hdd->irq = irq;
	hdd->type = DEV_BLK;

	hdd->action.dev = hdd;
	hdd->action.irq_handler = &ata_irq_handler;
	hdd->action.data = NULL;

	strcpy(hdd->name, "BLK-DEV");
	metafs_init(&hdd->node, hdd->name);

	hdd->op.dev.open = NULL;
	hdd->op.dev.read = &ata_read;
	hdd->op.dev.write = &ata_write;
	hdd->op.dev.close = NULL;
	hdd->op.dev.lseek = NULL;
	hdd->op.dev.get_params = &ata_get_params;
	hdd->op.dev.set_params = NULL;

	ctx = kbootMem_calloc(sizeof(*ctx));
	list_root_init(&ctx->request_queue);
	wait_queue_init(&ctx->pending, hdd->name);
  
	dev_base = ATA_BASE((uint32_t)base);
	drive = ATA_DRIVE((uint32_t)base);

	cpu_io_out8(dev_base + ATA_DRIVE_REG, 0xA0 | (drive << 4));
	cpu_io_out8(dev_base + ATA_CMD_REG, ATA_IDENTIFY_CMD);
  
	if((err=ata_busy_wait(dev_base)))
	{
		printk(ERROR, "ERROR: ibmpc_ata_init: dev %x, ATA_IDENTIFY_CMD time out\n", dev_base);
		return;
	}

	buffer = (uint16_t *)&ctx->params.info;

	for(index = 0; index < 256; index++)
		buffer[index] = cpu_io_in16(dev_base + ATA_DATA_REG);
  
	assert((ctx->params.info.capabilities & ATA_LBA_CAP) && "LBA mode is mandatory");
  
	ctx->params.blk_size = 512;
	ctx->params.blk_count = ctx->params.info.sectors;
	hdd->data = (void *)ctx;
	
	printk(INFO, "INFO: ATA(%x) Drive(%d): (size,count) %d:%d\n", 
	       dev_base, 
	       drive, 
	       ctx->params.blk_size, 
	       ctx->params.blk_count);
}
