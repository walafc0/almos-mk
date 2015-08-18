/*
 * cpu-context.c - thread's execution context operations
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
#include <cpu.h>
#include <thread.h>
#include <cpu-internal.h>

extern struct cpu_idt_ptr_s idt_ptr;

void __cpu_context_init(struct cpu_context_s* ctx, struct cpu_context_attr_s *attr)
{
	register struct thread_s *thread;
	register uint_t *ptr; 
	register uint_t isUserMode;
	register reg_t cpu_id;

	thread = (struct thread_s*)attr->tid;
	cpu_id = thread->local_cpu->gid;
	ptr    = (uint_t*) attr->stack_ptr;
	thread->info.usr_tls = (uint_t)thread;
	isUserMode = (attr->mode == CPU_USR_MODE) ? 1 : 0;
  
	*(--ptr)        = attr->arg2;
	*(--ptr)        = attr->arg1;
	*(--ptr)        = attr->exit_func;
	attr->stack_ptr = (reg_t)ptr;
	*(--ptr)        = (isUserMode) ? 0x23 : 0x10;
	*(--ptr)        = attr->stack_ptr;
	*(--ptr)        = 0x0200;
	*(--ptr)        = (isUserMode) ? 0x1b : 0x08;
	*(--ptr)        = attr->entry_func;

	ctx->loadable   = 1;
	ctx->tss_ptr    = (reg_t)cpu_get_tss(cpu_id);
	ctx->cpu_id     = cpu_id;
	ctx->kstack_ptr = (isUserMode) ? (reg_t)thread->sys_stack_top : attr->stack_ptr;
	ctx->mode       = 0;
	ctx->stack_ptr  = (reg_t)ptr;
	ctx->thread_ptr = attr->tid;
	ctx->entry_func = attr->entry_func;
	ctx->exit_func  = attr->exit_func;
	ctx->arg1       = attr->arg1;
	ctx->arg2       = attr->arg2;
}

extern void dump(uint8_t*, int);
extern void gdt_print_entry(struct cpu_gdt_entry_s*);

void cpu_context_load(struct cpu_context_s *ctx)
{
	struct cpu_tss_s *tss = (struct cpu_tss_s*)ctx->tss_ptr;
	ctx->loadable = 0;
  
	tss->ss_r0 = 0x10;	/* KERNEL DATA/STACK */
	tss->esp_r0 = ctx->kstack_ptr;
	tss->esp_r1 = ctx->thread_ptr;

	//printk(DEBUG,"cpu_idt.base %x, cpu_idt.limit %d\n",idt_ptr.base, idt_ptr.limit);

#if 0
	int tss_index = (ctx->cpu_id + GDT_FIXED_NR) << 3;
	uint32_t *ptr;
	int32_t i;
	gdt_print_entry(cpu_get_gdt_entry(5));
	ptr = (uint32_t*)ctx->stack_ptr;
	printk(DEBUG, "Thread %x, stack %x, mode %x, tss idx %x\t", ctx->thread_ptr, ptr, ctx->mode, tss_index);
	for(i=6; i >= 0; i--)
	{
		printk(DEBUG, "stack[%d] = %x\n", i, ptr[i]);
	}
#endif
	__asm__ __volatile__ ("" ::: "memory");

	__asm__ volatile
		("movl    %0,      %%esp      \n"
		 "xor     $0x0,    %1         \n"
		 "jz      1f                  \n"
		 "movl    %%edx,   %%ecx      \n"
		 "shr     $16,     %%ecx      \n"
		 "movw    $0x23,   %%ax	  \n" /* UDATA */
		 "movw    %%ax,    %%ds	  \n"
		 "movw    %%ax,    %%es	  \n"
		 "movw    %%dx,    %%fs       \n"
		 "movw    %%cx,    %%gs       \n" 
		 "1:                          \n"
		 "iret                        \n"
		 ::"r"(ctx->stack_ptr), "r"(ctx->mode), "d"(ctx->thread_ptr));
}
