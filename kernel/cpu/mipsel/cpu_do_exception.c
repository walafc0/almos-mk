/*
 * cpu_do_exception.c - first exceptions handler
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
#include <task.h>
#include <thread.h>
#include <kdmsg.h>
#include <pmm.h>
#include <vmm.h>
#include <errno.h>
#include <scheduler.h>
#include <cpu.h>
#include <spinlock.h>
#include <distlock.h>
#include <cpu-trace.h>
#include <cpu-regs.h>

#define X_AdEL 4
#define X_AdES 5
#define X_IBE  6
#define X_DBE  7
#define X_CpU  11

#define  DELAYED_SLOT_MASK 0x80000000

DISTLOCK_DECLARE(exception_lock);

#if 1
static void print_thread(struct thread_s *th)
{
	except_dmsg("Thread %x\n\tsys_stack_top %x\n\tusr_stack %x\n\tutls %x\n\tstate %s\n\tlocks %d\n",
		    th,
		    th->uzone.regs[KSP], 
		    th->uzone.regs[SP], 
		    th->uzone.regs[TLS_K1],
		    thread_state_name[th->state], 
		    th->locks_count);

	except_dmsg("\texit_val %x\n\tjoin %x\n\ttype %s\n\tCPU %d\n\twakeup_date %d\n\tlist %x\n\t  next %x\n\t  pred %x\n",
		    th->info.exit_value,
		    th->info.join, 
		    thread_attr_name[th->type], 
		    thread_current_cpu(th)->gid, 
		    th->info.wakeup_date,
		    th->list, th->list.next, 
		    th->list.pred);
}

#else
static void print_thread(struct thread_s* ths)
{
}
#endif


struct cpu_except_info_s
{
	uint_t code;
	char *name;
	char *desc;
};

typedef struct cpu_except_info_s cpu_except_info_t;
        
#define CPU_UNKNOWN_ERR  13
static cpu_except_info_t cpu_except_db[] = 
{{0, "INT", "Interrupt"},
 {1, "RES?", "Reserved for MMU exception"},
 {2, "RES?", "Reserved for MMU exception"},
 {3, "RES?", "Reserved for MMU exception"},
 {4, "ADDRL", "Load from an illegal address"},
 {5, "ADDRS", "Store to an illegal address"},
 {6, "IBUS", "Bus error on instruction fetch"},
 {7, "DBUS", "Bus error on data reference"},
 {8, "SYSCALL", "syscall instruction executed"},
 {9, "BKPT", "break instruction executed"},
 {10, "RI", "Reserved instruction"},
 {11, "MCP?", "Missing coprocessor"},
 {12, "OVF", "Arithmetic overflow"},
 {13, "UNKNOWN?", "Unknow error"}};
                        
                        
                
inline cpu_except_info_t* cpu_except_get_entry(uint_t excCode)
{
	if(excCode >= CPU_UNKNOWN_ERR)
		excCode = CPU_UNKNOWN_ERR;

        return &cpu_except_db[excCode];
}



error_t CpU_exception_handler(struct thread_s *this, reg_t cpu_id, reg_t *regs_tbl)
{
	register struct cpu_s *cpu;

	if(((regs_tbl[CR] >> 28) & 0x3) != 1)
		return -1;
  
	cpu = current_cpu;
	cpu_fpu_enable();

	if((cpu->fpu_owner != NULL) && (cpu->fpu_owner != this))
	{
		cpu_fpu_context_save(&cpu->fpu_owner->uzone);

#if CONFIG_SHOW_FPU_MSG
		printk(INFO, "INFO: FPU %d, ctx saved for pid %d, tid %d (%x) [%u]\n",
		       cpu->gid,
		       cpu->fpu_owner->task->pid,
		       cpu->fpu_owner->info.order,
		       cpu->fpu_owner,
		       cpu_time_stamp());
#endif
	}

	cpu_fpu_context_restore(&this->uzone);
	cpu->fpu_owner = this;

#if CONFIG_SHOW_FPU_MSG
	printk(INFO, "INFO: FPU %d, ctx restored for pid %d, tid %d (%x) [%u]\n",
	       cpu->gid,
	       this->task->pid,
	       this->info.order,
	       this,
	       cpu_time_stamp());
#endif
	return VMM_ERESOLVED;
}

#define _USE_MMU_INFO_H_
#include <mmu-info.h>

error_t mmu_exception_handler(reg_t cpu_id, reg_t *regs_tbl, uint_t mmu_err_val, uint_t mmu_bad_vaddr)
{
	if(mmu_err_val & MMU_EFATAL)
		return VMM_ESIGBUS;
  
	if(!(regs_tbl[SR] & 0x10))
		mmu_err_val |= MMU_EKMODE;

	return vmm_fault_handler(mmu_bad_vaddr, mmu_err_val);
}

extern uint_t __uspace_start;
extern uint_t __uspace_end;
extern uint_t cpu_uspace_error;

void cpu_do_exception(struct thread_s *this, reg_t cpu_id, reg_t *regs_tbl)
{
	error_t err;
	uint_t uspace_start;
	uint_t uspace_end;
	uint_t excCode;
	uint_t instContent;
	uint_t mmu_ierr_val;
	uint_t mmu_ibad_vaddr;
	uint_t mmu_derr_val;
	uint_t mmu_dbad_vaddr;
	bool_t isInKernelMode;
	mmu_except_info_t *entry;
	cpu_except_info_t *execErr;
	extern uint_t __ktext_start;
	extern uint_t __ktext_end;

	excCode        = (regs_tbl[CR] >> 2) & 0x1F;
	mmu_ierr_val   = mips_get_cp2(MMU_IETR, 0);
	mmu_ibad_vaddr = mips_get_cp2(MMU_IBVAR, 0);
	mmu_derr_val   = mips_get_cp2(MMU_DETR, 0);
	mmu_dbad_vaddr = mips_get_cp2(MMU_DBVAR, 0);

	switch(excCode)
	{
	case X_IBE:
		err = mmu_exception_handler(cpu_id, regs_tbl, mmu_ierr_val, mmu_ibad_vaddr);
		break;
	case X_DBE:
		err = mmu_exception_handler(cpu_id, regs_tbl, mmu_derr_val, mmu_dbad_vaddr);
		break;
	case X_CpU:
		err = CpU_exception_handler(this, cpu_id, regs_tbl);
		break;
	default:
		err = VMM_ESIGBUS;
		break;
	}

	if(err == VMM_ERESOLVED)
		return;

	if(err == VMM_ECHECKUSPACE)
	{
		uspace_start = (uint_t) &__uspace_start;
		uspace_end   = (uint_t) &__uspace_end;

		if((regs_tbl[EPC] >= uspace_start) && (regs_tbl[EPC] <= uspace_end))
		{
			regs_tbl[EPC] = (reg_t) &cpu_uspace_error;
			regs_tbl[MMU_MD] = (reg_t) 0x3;//MMU_MODE OFF
			return;
		}

		err = VMM_ESIGBUS;
	}

	distlock_lock(&exception_lock);
	except_dmsg("====================================================================\n");
	except_dmsg("Exception has occured, thread %x, cpu %d, cycle %d\n", this, cpu_id, cpu_time_stamp());
	except_dmsg("Processor State:\n");
  
	except_dmsg("CR:   %x\tEPC:  %x\tSR:   %x\tSP:    %x\tUSR SP %x\n",
		    regs_tbl[CR],regs_tbl[EPC],regs_tbl[SR],regs_tbl[SP],this->uzone.regs[SP]);

	except_dmsg("at_1  %x\tv0_2  %x\t\tv1_3  %x\ta0_4   %x\ta1_5   %x\n",
		    regs_tbl[AT],regs_tbl[V0],regs_tbl[V1],regs_tbl[A0],regs_tbl[A1]);

	except_dmsg("a2_6  %x\t\ta3_7  %x\tt0_8  %x\tt1_9   %x\tt2_10  %x\n",
		    regs_tbl[A2],regs_tbl[A3],regs_tbl[T0],regs_tbl[T1],regs_tbl[T2]);
  
	except_dmsg("t3_11 %x\tt4_12 %x\t\tt5_13 %x\tt6_14  %x\tt7_15  %x\n",
		    regs_tbl[T3],regs_tbl[T4],regs_tbl[T5],regs_tbl[T6],regs_tbl[T7]);

	except_dmsg("t8_24 %x\t\tt9_25 %x\tgp_28 %x\tc0_hi  %x\tc0_lo  %x\n",
		    regs_tbl[T8],regs_tbl[T9],regs_tbl[GP],regs_tbl[HI],regs_tbl[LO]);

	except_dmsg("s0_16 %x\ts1_17 %x\ts2_18 %x\ts3_19  %x\ts4_20  %x\n",
		    regs_tbl[S0],regs_tbl[S1],regs_tbl[S2],regs_tbl[S3],regs_tbl[S4]);
  
	except_dmsg("s5_21 %x\ts6_22 %x\t\ts7_23 %x\ts8_30  %x\tra_31  %x\n\n",
		    regs_tbl[S5],regs_tbl[S6],regs_tbl[S7],regs_tbl[S8],regs_tbl[RA]);

	print_thread(this);

	cpu_trace_dump(current_cpu);
  
	if(excCode == X_IBE)
		entry = mmu_except_get_entry(mmu_ierr_val);
	else
		entry = mmu_except_get_entry(mmu_derr_val);

	isInKernelMode = (regs_tbl[SR] & 0x10) ? false : true;

	except_dmsg("\nIs in kernel: %s\n", (isInKernelMode) ? "YES" : "NO");

	except_dmsg("\nMMU Exception State:\n\tName: %s\n\tInfo: %s\n\tSeverty: %s\n\tAdditional: iBadVaddr 0x%x, dBadVaddr 0x%x, iErr %x, dErr %x\n\n",
		    entry->name, entry->info, 
		    entry->severty, mmu_ibad_vaddr,mmu_dbad_vaddr, 
		    mmu_ierr_val, mmu_derr_val);

	if(isInKernelMode)
	{
		execErr = cpu_except_get_entry(excCode);
		if(regs_tbl[EPC] >= __ktext_start && regs_tbl[EPC] <= __ktext_end)
			instContent = *((uint_t*) regs_tbl[EPC]);
		else
			instContent = 0;
		except_dmsg("Pid %d, Cpu %d, Inst. %x, Exception : code %d, name %s, description %s, bad address %x\n",
			    this->task->pid,
			    cpu_id,
			    instContent,
			    excCode, 
			    execErr->name, 
			    execErr->desc, 
			    cpu_get_bad_vaddr()
				);
	
	}


	except_dmsg("====================================================================\n");

	distlock_unlock(&exception_lock);

	this->state = S_KERNEL;
	sched_exit(this);
	while(entry != NULL);
}
