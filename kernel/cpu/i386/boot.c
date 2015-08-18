/*
 * boot.c - kernel low-level boot entry
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

#include <kdmsg.h>
#include <hardware.h>
#include <device.h>
#include <cpu.h>
#include <cpu-internal.h>

struct mb_partial_info_s 
{
	unsigned long flags;
	unsigned long low_mem;
	unsigned long high_mem;
	unsigned long boot_device;
	unsigned long cmdline;
};

extern uint_t *_end;

#define ARROUND_UP(val, size) (val) & ((size) -1) ? ((val) & ~((size)-1)) + (size) : (val)

static uint_t kheap_start;
static uint_t kheap_limit;
extern void __do_init(uint_t, uint_t);
extern void cpu_idt_init();

static void tty_clear()
{
	register uint_t i = 0;
	uint8_t *vga_ram = (uint8_t*)__tty_addr;

	for(i = 0; i < TTY_SIZE; i+=2)
	{
		vga_ram[i] = 0;
		vga_ram[i+1] = 0x7;
	}
}

void _boot_init()
{
	printf("[ OK ]\n");
	__do_init(kheap_start, kheap_limit);
}
	

void gdt_print(struct cpu_gdt_entry_s *gdt, int nr);

void kboot_entry(struct mb_partial_info_s *mbi)
{
  
	uint32_t gdt_ptr;
	uint32_t tss_index;
  
	kheap_start = ARROUND_UP((uint_t)&_end, 4096);
	kheap_limit = kheap_start + (8<<20);

	tty_clear();

	printf("AlmOS Kernel has been loaded\n");

	printf("RAM detected : %uk (lower), %uk (upper)\n", mbi->low_mem, mbi->high_mem);
	printf("end of data, kheap start @%x, end @%x\n", kheap_start, kheap_limit);
	printf("Initializaiton of CPU context ...\t");
	cpu_gdt_init();
	printf("[ OK ]\nloading IDT ...\t");
	cpu_idt_init();
	printf("[ OK ]\nloading GDT ...\t");

	gdt_ptr = (uint32_t) cpu_get_gdt_ptr();
	tss_index = (cpu_get_id() + GDT_FIXED_NR) << 3;
  
#if 0
	gdt_print((struct cpu_gdt_entry_s*)cpu_get_gdt_ptr()->base, GDT_FIXED_NR + CPU_NR);
	printf("\n\ngdt_ptr %x\n", gdt_ptr);
#endif

	__asm__ __volatile__ ("" ::: "memory");

#if 1	
	asm volatile 
		(".extern __boot_stack        \n"
		 ".extern _init_              \n"
		 "movl  %0,      %%ebx        \n"
		 "movl  %1,      %%ecx        \n"
		 "lgdt  (%%ebx)               \n"
		 "movw  $0x10,   %%ax	  \n" /* KDATA */
		 "movw  %%ax,    %%ds	  \n"
		 "movw  %%ax,    %%es	  \n"
		 "movw  %%ax,    %%fs	  \n"
		 "movw  %%ax,    %%gs	  \n"
		 "ljmp  $0x08,   $next	  \n" /* KTEXT */
		 "next:		          \n"
		 "xor   %%eax,  %%eax         \n"
		 "leal  (__boot_stack), %%esi \n"
		 "movw  $0x10,  %%ax          \n" /* KSTACK */
		 "movw  %%ax,   %%ss          \n"
		 "movl  %%esi,  %%esp         \n"
		 "ltr   %%cx                  \n"
		 "call  _boot_init            \n"
		 :: "r" (gdt_ptr), "r"(tss_index));
#endif
}
