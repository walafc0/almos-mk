/*
 * pci.c - PCI bus related registers and access functions
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

#ifndef _PCI_BUS_H_
#define _PCI_BUS_H_

#include <types.h>
#include <cpu-io.h>

#define PCI_TGT_MAX_CONFIG_SPACE      256
#define PCI_TGT_MAX_CONFIG_REG        64
#define PCI_BUS_MAX_NR                255
#define PCI_BUS_DEV_MAX_NR            32
#define PCI_DEV_FUNC_MAX_NR           8

#define PCI_CONFIG_ADDR               0xCF8
#define PCI_CONFIG_DATA               0xCFC

#define PCI_STD_DEV_TYPE              0x00
#define PCI_BRIDGE_TYPE               0x01
#define PCI_CARDBUS_BRIDGE_TYPE       0x02

#define PCI_VENDOR_ID                 0x00
#define PCI_DEVCIE_ID                 0x02
#define PCI_CMD_REG                   0x04
#define PCI_STATUS_REG                0x06
#define PCI_REVISION_ID               0x08

#define PCI_CLASS_ID                  0x0A
#define PCI_HEADER_TYPE               0x0E

#define PCI_BAR0                      0x10
#define PCI_BAR1                      0x14
#define PCI_BAR2                      0x18
#define PCI_BAR3                      0x1C
#define PCI_BAR4                      0x20
#define PCI_BAR5                      0x24

#define PCI_INT_INFO 0x3C

#define pci_mkconfig1_addr(_bus,_dev,_func,_offset)	\
	((uint32_t)(0x80000000    | ((_bus) << 16) |	\
		    ((_dev) << 11) | ((_func) << 8) |	\
		    ((_offset) & 0xfc)))


static inline uint16_t pci_config_read(uint_t bus, uint_t slot, uint_t func, uint_t offset)
{ 
	uint32_t addr;
	uint32_t val; 

	addr = pci_mkconfig1_addr(bus,slot,func,offset);

	cpu_io_out32(PCI_CONFIG_ADDR, addr);
	val = cpu_io_in32(PCI_CONFIG_DATA);

	return val >> ((offset & 3) * 8);
}

#endif	/* _PCI_BUS_H_ */
