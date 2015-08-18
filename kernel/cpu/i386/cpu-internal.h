/*
 * cpu-internal.h - cpu internal data structures
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

#ifndef _CPU_INTERNAL_H_
#define _CPU_INTERNAL_H_

#include <types.h>

enum
{
	GDT_NULL_ENTRY = 0,		/* 0x00 */

	GDT_KTEXT_ENTRY,		/* 0x08 */
	GDT_KDATA_ENTRY,		/* 0x10 */

	GDT_UTEXT_ENTRY,		/* 0x18 used in cpu_context.c::__cpu_context_init */
	GDT_UDATA_ENTRY,		/* 0x20 used in cpu_context.c::__cpu_context_init */
	GDT_UTLS_ENTRY,                 /* 0x28 used in cpu_context.c::__cpu_context_init */
	GDT_FIXED_NR
};

/* GDT table entry type */
struct cpu_gdt_entry_s
{
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t  base_middle;
	uint8_t  access;
	uint8_t  granularity;
	uint8_t  base_high;
} __attribute__((packed));

/* GDT pointer type */
struct cpu_gdt_ptr_s
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

void cpu_gdt_init();
void cpu_idt_init();

struct cpu_gdt_entry_s* cpu_get_gdt_entry(int entry);
struct cpu_gdt_ptr_s*   cpu_get_gdt_ptr(void);

/* IDT table entry type */
struct cpu_idt_entry_s
{
	uint16_t offset_low;
	uint16_t selector;
	uint8_t  reserved;
	uint8_t  flags;
	uint16_t offset_high;
} __attribute__((packed));

/* IDT pointer type */
struct cpu_idt_ptr_s
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

/* Task State Segment type */
struct cpu_tss_s
{
	uint16_t pred_link;		/* 0 */
	uint16_t reserved_0;
  
	uint32_t esp_r0;		/* 1 */
	uint16_t ss_r0;		        /* 2 */
	uint16_t reserved_1;
  
	uint32_t esp_r1;		/* 3 */
	uint16_t ss_r1;		        /* 4 */
	uint16_t reserved_2;
  
	uint32_t esp_r2;		/* 5 */
	uint16_t ss_r2;		        /* 6 */
	uint16_t reserved_3;

	uint32_t cr3_pdbr;		/* 7 */
	uint32_t eip;			
	uint32_t eflags;

	uint32_t eax,ecx,edx,ebx;	/* 10 */
	uint32_t esp,ebp,esi,edi;	/* 14 */
  
	uint16_t es, reserved_4;	/* 18 */
	uint16_t cs, reserved_5;	/* 19 */
	uint16_t ss, reserved_6;	/* 20 */
	uint16_t ds, reserved_7;	/* 21 */
	uint16_t fs, reserved_8;	/* 22 */
	uint16_t gs, reserved_9;	/* 23 */
	uint16_t ldt,reserved_10;	/* 24 */
 
	uint16_t T:1;			/* 25 */
	uint16_t reserved_11:15;
	uint16_t io_map;
} __attribute__ ((packed));


struct cpu_tss_s* cpu_get_tss(int cpu_id);
void cpu_set_utls(uint_t addr);

/* CPU Registers */
struct cpu_regs_s
{
	uint_t gs;
	uint_t fs;
	uint_t es;
	uint_t ds;
	uint_t edi;
	uint_t esi;
	uint_t ebp;
	uint_t esp;
	uint_t ebx;
	uint_t edx;
	uint_t ecx;
	uint_t eax;
	uint_t int_nr;
	uint_t err_code;
	uint_t ret_eip;
	uint_t ret_cs;
	uint_t eflags;
	uint_t old_esp;
	uint_t old_ss;
} __attribute__ ((packed));





#endif	/* _CPU_INTERNAL_H_ */
