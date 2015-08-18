/*
 * ata.h - ATA interface registers
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

#ifndef _ATA_H_
#define _ATA_H_

#include <types.h>
#include <list.h>
#include <driver.h>

/* ATA Task File offsets */
#define ATA_DATA_REG	        0
#define ATA_ERROR_REG	        1
#define ATA_PRECOMPENSATION_REG	1
#define ATA_COUNT_REG           2
#define ATA_LBA_LOW_REG	        3
#define ATA_LBA_MID_REG	        4
#define ATA_LBA_HIGH_REG	5
#define ATA_DRIVE_REG	        6
#define ATA_STATUS_REG          7
#define ATA_CMD_REG             7

#define ATA_DOR_PORT         0x3F6

/* ATA Status Register bits */
#define ATA_ERR              0x01
#define ATA_IDX              0x02
#define ATA_CORR             0x04
#define ATA_DRQ              0x08
#define ATA_DSC              0x10
#define ATA_WFT              0x20
#define ATA_DRDY	     0x40
#define ATA_BSY	             0x80


/* ATA Error Register bits */

/* Block device status */
#define ATA_IDLE	        0
#define ATA_BUSY	        1
#define ATA_READ_SUCCESS	2
#define ATA_WRITE_SUCCESS	3
#define ATA_READ_ERROR	        4
#define ATA_WRITE_ERROR	        5
#define ATA_ERROR	        6

#define ATA_READ_CMD            0x020
#define ATA_WRITE_CMD           0x030
#define ATA_IDENTIFY_CMD        0x0EC

#define ATA_LBA_CAP             0x0100

struct device_s;

struct ata_identify_s
{
	uint16_t general;
	uint16_t geometry[6];
	uint16_t vendor_id1[3];
	uint16_t info1[37];
	uint16_t vendor_id2;
	uint16_t info2;
	uint16_t capabilities;
	uint16_t info3[10];
	uint32_t sectors;
	//uint16_t lblock_lo;
	//uint16_t lblock_hi;
	uint32_t info4;
	uint16_t reserved1[64];
	uint16_t vendor_id3[32];
	uint16_t reserved2[96];
} __attribute__ ((packed));

struct ata_params_s
{
	uint32_t blk_count;
	uint32_t blk_size;
	struct ata_identify_s info;
};

struct ata_context_s
{
	struct list_entry request_queue;
	struct wait_queue_s pending;
	struct ata_params_s params;
};

void ibmpc_ata_init(struct device_s *hdd, void *base, uint_t irq);

#endif /* _ATA_H_ */
