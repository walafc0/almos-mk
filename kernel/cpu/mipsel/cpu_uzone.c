/*
 * cpu_uspace.c - user-space related operations
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
#include <task.h>
#include <vmm.h>
#include <thread.h>
#include <string.h>
#include <pmm.h>
#include <cpu-regs.h>


void cpu_uzone_init(struct cpu_uzone_s *uzone)
{
}

void cpu_uzone_destroy(struct cpu_uzone_s *uzone)
{
}

void cpu_uzone_dup(struct cpu_uzone_s *dst, struct cpu_uzone_s *src)
{
	memcpy(dst, src, sizeof(*src));
}

error_t cpu_uzone_getattr(struct cpu_uzone_s *uzone, struct cpu_uzone_attr_s *attr)
{
	attr->tls = uzone->regs[TLS_K1];
	attr->stack_top = uzone->regs[SP];
	return 0;
}

error_t cpu_uzone_setattr(struct cpu_uzone_s *uzone, struct cpu_uzone_attr_s *attr)
{
	uzone->regs[TLS_K1] = attr->tls;
	uzone->regs[SP] = attr->stack_top;
	return 0;
}


void cpu_signal_notify(struct thread_s *this, void *handler, uint_t sig)
{
	struct cpu_context_s *ctx;
	struct cpu_s *cpu;
	reg_t stack_top;
	error_t err;
  
	cpu = current_cpu;

	if(cpu->fpu_owner != NULL)
		cpu_fpu_context_save(&cpu->fpu_owner->uzone);

	cpu->fpu_owner = this;
  
	stack_top  = this->pws.sigstack_top;
	stack_top -= sizeof(this->uzone);

	err = cpu_copy_to_uspace((void*)stack_top, &this->uzone, sizeof(this->uzone));
    
	if(err)
	{
		printk(WARNING, "WARNING: %s: cannot delever signal %d, handler @%x, stack_top %x\n",
		       __FUNCTION__, 
		       sig, 
		       (uint_t)handler, 
		       stack_top);
		return;
	}

	ctx            = &this->pws;
	ctx->c0_sr     = __CPU_USR_MODE_FPU;
	ctx->sp_29     = stack_top;
	ctx->fp_30     = this->uzone.regs[TLS_K1];
	ctx->ra_31     = (reg_t) handler;
	ctx->tid       = (reg_t) this->info.attr.tid;
	ctx->exit_func = (reg_t) ctx->sigreturn_func;
	ctx->arg1      = (reg_t) sig;
	ctx->arg2      = (reg_t) 0;
	ctx->loadable  = 1;

	cpu_context_load(ctx);
}


void cpu_context_set_sigreturn(struct cpu_context_s *ctx, void *sigreturn_func)
{
	ctx->sigreturn_func = (reg_t) sigreturn_func;
}
