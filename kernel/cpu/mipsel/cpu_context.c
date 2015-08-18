/*
 * cpu_context.c - thread's execution context (see kern/hal-cpu.h)
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
#include <thread.h>
#include <string.h>
#include <task.h>
#include <vmm.h>
#include <pmm.h>
#include <cpu.h>
#include <cluster.h>

#include <cpu-regs.h>

inline void cpu_context_init(struct cpu_context_s *ctx, struct thread_s *thread)
{
	register reg_t stack_top;
	register reg_t kstack_top;
	register reg_t sigstack_top;
	register pthread_attr_t *attr;
  
	attr          = &thread->info.attr;
	stack_top     = ((reg_t)attr->stack_addr) + attr->stack_size;
	kstack_top    = ((reg_t)thread->info.kstack_addr) + thread->info.kstack_size;
	sigstack_top  = ((reg_t)attr->sigstack_addr) + attr->sigstack_size;

	stack_top     = (stack_top - 8UL)  & (~ 0x7);
	kstack_top    = (kstack_top - 8UL) & (~ 0x7);
	sigstack_top &= (sigstack_top - 8UL) & (~ 0x7);

	thread->uzone.regs[KSP] = kstack_top;
	thread->uzone.regs[DP_EXT] = arch_clst_arch_cid(current_cid);
	thread->uzone.regs[MMU_MD] = 0xF;

	ctx->c0_sr          = (thread->type == PTHREAD) ? CPU_USR_MODE : CPU_SYS_MODE;
	ctx->sp_29          = (thread->type == PTHREAD) ? stack_top : kstack_top; 
	ctx->fp_30          = (reg_t)attr->tid;
	ctx->ra_31          = (reg_t)attr->entry_func;
	ctx->tid            = (reg_t)attr->tid;
	ctx->exit_func      = (reg_t)attr->exit_func;
	ctx->arg1           = (reg_t)attr->arg1;
	ctx->arg2           = (reg_t)attr->arg2;
	ctx->loadable       = 1;
	ctx->pgdir          = (reg_t)((thread->task->vmm.pmm.pgdir_ppn) >> 1);
	ctx->stack_addr     = (reg_t)attr->stack_addr;
	ctx->stack_size     = (reg_t)attr->stack_size;
	ctx->sigreturn_func = (reg_t)attr->sigreturn_func;
	ctx->sigstack_top   = sigstack_top;
	ctx->sigstack_size  = (reg_t)attr->sigstack_size;
	ctx->mmu_mode       = (thread->type == PTHREAD) ? 0xF : 0x3;
}

inline void cpu_context_destroy(struct cpu_context_s *ctx)
{
}

inline void cpu_context_set_tid(struct cpu_context_s *ctx, reg_t tid)
{
	ctx->tid = tid;
}

inline void cpu_context_set_pmm(struct cpu_context_s *ctx, struct pmm_s *pmm)
{
	ctx->pgdir = pmm->pgdir_ppn >> 1;
}

inline void cpu_context_set_stackaddr(struct cpu_context_s *ctx, reg_t addr)
{
	ctx->sp_29 = addr;
}

inline uint_t cpu_context_get_stackaddr(struct cpu_context_s *ctx)
{
	return ctx->sp_29;
}

inline void cpu_context_dup_finlize(struct cpu_context_s *dst, struct cpu_context_s *src)
{
	register struct thread_s *thread;
	register reg_t stack_addr;
	register uint_t mask;

	mask = PMM_PAGE_MASK | (PMM_PAGE_MASK << ARCH_THREAD_PAGE_ORDER);

	memcpy(dst, src, sizeof(*dst));
	thread = (struct thread_s*)dst->tid;

	stack_addr  = cpu_context_get_stackaddr(src);
	stack_addr &= mask;
	stack_addr |= ((reg_t)dst->tid & ~mask);
	cpu_context_set_stackaddr(dst, stack_addr);

	stack_addr = (dst->tid | mask) & (~ 0x7);
	thread->uzone.regs[KSP] = stack_addr;
	//after a fork or thread_mig the thread get 
	//to be executed in kernel mode
	dst->mmu_mode = 0x3; 
}
