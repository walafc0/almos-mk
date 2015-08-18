/*
 * cpu-internal.c - cpu internal data structures
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
#include <cpu.h>
#include <cpu-internal.h>
#include <libk.h>
#include <hardware.h>
#include <pic.h>


#define GDT_ENTRIES_NR  GDT_FIXED_NR + CPU_NR
#define IDT_ENTRIES_NR  PIC_IRQ_MAX + 32 + 1           /* excepts + IRQs + syscall */
struct cpu_gdt_ptr_s cpu_gdt_ptr __attribute__((aligned(8)));
struct cpu_idt_ptr_s idt_ptr __attribute__((aligned(8)));
struct cpu_tss_s cpu_tss_tbl[CPU_NR] __attribute__((aligned(8)));
struct cpu_gdt_entry_s cpu_gdt[GDT_ENTRIES_NR];
struct cpu_idt_entry_s cpu_idt[IDT_ENTRIES_NR];



void gdt_entry_set(struct cpu_gdt_entry_s *entry,
		   uint_t base, 
		   uint_t limit, 
		   uint8_t access, 
		   uint8_t granularity)
{
	entry->limit_low = (limit & 0xFFFF);
	entry->base_low = (base & 0xFFFF);
	entry->base_middle = (base >> 16) & 0xFF;
	entry->access = access;
	entry->granularity = ((granularity & 0xF) << 4) | ((limit >> 16) & 0xF);
	entry->base_high = (base >> 24) & 0xFF;
}


void gdt_print_entry(struct cpu_gdt_entry_s *entry)
{
	printf("limit_low %x,  base %x", 
	       entry->limit_low, 
	       (entry->base_high << 24) | (entry->base_middle << 16) | entry->base_low);

	printf("\t acess %x, gran %x\n", 
	       entry->access, 
	       entry->granularity);
}

void gdt_print(struct cpu_gdt_entry_s *gdt, int nr)
{
	int i;

	for(i=0; i < nr; i++)
		gdt_print_entry(&gdt[i]);
}


void idt_entry_set(struct cpu_idt_entry_s *entry,
		   uint_t offset, 
		   uint_t selector, 
		   uint8_t flags)
{
	entry->offset_low  = (offset & 0xFFFF);
	entry->selector    = selector;
	entry->flags       = flags;
	entry->reserved    = 0;
	entry->offset_high = offset >> 16;
}

extern uint_t __except;
extern uint_t __irq;

void cpu_idt_init()
{
	uint_t i;
	volatile uint32_t *ptr;
	char *except_start = (char*)&__except;
	char *irq_start    = (char*)&__irq;
	size_t size        = (irq_start - except_start) / 32;

#if 0
	printf("size of exception entry %d, irq_start %x, exep_start %x\n", size, irq_start, except_start);
#endif

	for(i=0; i < 32; i++)
		idt_entry_set(&cpu_idt[i], (uint_t)(except_start + i*size), 0x8, 0x8E);
  
	for(i=0; i < PIC_IRQ_MAX; i++)
	{
#if 0
		if(i>=60)
			printf("irq%d handler @%x, irq_start %x, i*size = %d\n", 
			       i, (uint_t)(irq_start + i*size), irq_start, i*size);
#endif
		idt_entry_set(&cpu_idt[i + 32], (uint_t)(irq_start + i*size), 0x8, 0x8E);
	}

	//while(1);
	idt_entry_set(&cpu_idt[i + 32], (uint_t)(irq_start + i*size), 0x8, 0xEF);

	idt_ptr.limit = ((IDT_ENTRIES_NR) * 8) - 1;
	idt_ptr.base  = (uint32_t)cpu_idt;
	ptr = (volatile uint32_t*)(&idt_ptr);

	__asm__ __volatile__ ("" ::: "memory");
	__asm__ volatile ("lidt   %0\n" :: "m" (*ptr) );
	//  while(1);
}

void cpu_gdt_init()
{
	int i;

	gdt_entry_set(&cpu_gdt[GDT_NULL_ENTRY], 0x0, 0x0, 0x0, 0x0);

	gdt_entry_set(&cpu_gdt[GDT_KTEXT_ENTRY], 0x00, 0xFFFFF, 0x9B, 0x0D);
	gdt_entry_set(&cpu_gdt[GDT_KDATA_ENTRY], 0x00, 0xFFFFF, 0x93, 0x0D);
  
	gdt_entry_set(&cpu_gdt[GDT_UTEXT_ENTRY], 0x00, 0xFFFFF, 0xFF, 0x0D);
	gdt_entry_set(&cpu_gdt[GDT_UDATA_ENTRY], 0x00, 0xFFFFF, 0xF3, 0x0D);
	//  gdt_entry_set(&cpu_gdt[GDT_UTLS_ENTRY], 0x00, 0xFFFFF, 0xF3, 0x0D);
  
	for(i=GDT_FIXED_NR; i < GDT_ENTRIES_NR; i++)
		gdt_entry_set(&cpu_gdt[i],(uint32_t)&cpu_tss_tbl[i - GDT_FIXED_NR], 0x67, 0x89, 0x00);

	cpu_gdt_ptr.limit = ((GDT_ENTRIES_NR) * 8) - 1;
	cpu_gdt_ptr.base  = (uint32_t)&cpu_gdt[0];

#if 0
	printf("cpu_gdt %x\n", val);
	printf("cpu_gdt_ptr.base = %x\n", cpu_gdt_ptr.base);
	printf("\n\n\n\nentries %d, gdt:\n|", GDT_ENTRIES_NR);
	//dump((uint8_t*)&cpu_gdt[0], (GDT_ENTRIES_NR)*8);
	//printf("|\n----------------------------------------------\n");

	printf("gdt_ptr %x:\n|", &cpu_gdt_ptr);
	//dump((uint8_t*)&cpu_gdt_ptr, 6);
	printf("|\n");
	//while(1);
#endif
}

void cpu_set_utls(uint_t addr)
{
	gdt_entry_set(&cpu_gdt[GDT_UTLS_ENTRY], addr, 0xFFFFF, 0xF3, 0x0D);
}


struct cpu_gdt_ptr_s* cpu_get_gdt_ptr(void)
{
	return &cpu_gdt_ptr;
}

struct cpu_gdt_entry_s* cpu_get_gdt_entry(int entry)
{
	return &cpu_gdt[entry];
}

//#include <kdmsg.h>
struct cpu_tss_s* cpu_get_tss(int cpu_id)
{
	//boot_dmsg("cpu_id %x, tss %x\n", cpu_id, &cpu_tss_tbl[cpu_id]);
	return &cpu_tss_tbl[cpu_id];
}
