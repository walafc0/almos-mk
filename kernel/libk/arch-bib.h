/*
 * arch-bib.h - Boot Information Block definition
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

#ifndef _ARCH_BIB_H_
#define _ARCH_BIB_H_

#include <stdint.h>

/////////////////////////////////////////////////////////////////
//           Architecture BIB (Boot Information Block)         //
//                   Specification Revision 1                  //
/////////////////////////////////////////////////////////////////

/* BIB Signature */
#ifdef _ARCH_BIB_SIGNATURE_
const char* arch_bib_signature = "@ALMOS ARCH BIB";
#endif

/* Header Info */
struct arch_bib_header_s
{
	/* addresses correspond to the local 32 bit of boot proc *
	 * except for the tty and dma (although they should normally *
	 * be in the boot cluster). */
	uint32_t   arch_bootstrap_cpu;	/* Boot Frozen Order */
	uint32_t   rsrvd_limit;	        /* Boot Frozen Order */
	uint32_t   cpu_nr;		/* Boot Frozen Order */ //local_cpu_nr!

	uint32_t   rsrvd_start;		//FG
	uint32_t   bootstrap_cpu;	//FG : almos_id
	char       signature[16];
	uint16_t   revision;
	char       arch[16];
	uint16_t   size;
	uint16_t   x_max;
	uint16_t   y_max;
	uint16_t   onln_cpu_nr;
	uint16_t   onln_clstr_nr;
	uint16_t   iopic_cluster;	//FG
	uint16_t   with_cpu_clstr_nr;	//FG
	uint64_t   bootstrap_dma;	//FG
	uint64_t   bootstrap_tty;	//FG
} __attribute__ ((packed));

/* Cluster Info */
struct arch_bib_cluster_s
{
	uint8_t cid;
	uint8_t arch_cid;
	uint8_t  cpu_nr;
	uint8_t  dev_nr;
	uint16_t offset;
} __attribute__((packed));

/* Device Info */
struct arch_bib_device_s
{
	uint8_t id;
	uint64_t base;
	uint32_t size;
	int16_t  irq;
} __attribute__((packed));


#endif	/* _ARCH_BIB_H_ */
