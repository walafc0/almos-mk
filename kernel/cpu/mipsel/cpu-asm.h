/*
 * cpu-asm.h - implementation of CPU related Hardware Abstraction Layer
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

#ifndef  _CPU_ASM_H_
#define  _CPU_ASM_H_


#ifndef _HAL_CPU_H_
#ifndef _REMOTE_ACCESS_H_
#error this file cannot be included directly, use cpu.h instead
#endif
#endif

#include <system.h>
#include <config.h>
#include <cpu-regs.h>

/* CPU IRQ macros */
#undef  CPU_HW_IRQ_NR
#define CPU_HW_IRQ_NR  6

#undef  CPU_IRQ_NR
#define CPU_IRQ_NR  CPU_HW_IRQ_NR

/* SR USR/SYS mask */
#undef  CPU_USR_MODE
#define CPU_USR_MODE  0xFC11

#define __CPU_USR_MODE_FPU 0x2000FC11

#undef  CPU_SYS_MODE
#define CPU_SYS_MODE  0xFC00

/* Processor Context */
struct cpu_context_s
{
	reg_t s0_16;
	reg_t s1_17;
	reg_t s2_18;
	reg_t s3_19;
	reg_t s4_20;
	reg_t s5_21;
	reg_t s6_22;
	reg_t s7_23;
	reg_t sp_29;			/* 8 */
	reg_t fp_30;			/* 9 */
	reg_t ra_31;			/* 10 */
	reg_t c0_sr;			/* 11 */
	reg_t tid;                      /* 12 */
	reg_t pgdir;			/* 13 */
	reg_t loadable;		        /* 14 */
	reg_t exit_func;		/* 15 */
	reg_t arg1;			/* 16 */
	reg_t arg2;			/* 17 */
	reg_t stack_addr;		/* 18 */
	reg_t stack_size;		/* 19 */
	reg_t sigreturn_func;           /* 20 */
	reg_t sigstack_top;             /* 21 */
	reg_t sigstack_size;		/* 22 */
	reg_t mmu_mode;			/* 23 */
}__attribute__ ((packed));

/* User Zone Saved Registers */
struct cpu_uzone_s
{
	reg_t regs[REGS_NR];		/* REGS_NR in mips-regs.h */
	reg_t fpu_regs[32];           /* offset: +REGS_NR */
}__attribute__ ((packed));

inline void cpu_context_init(struct cpu_context_s *ctx, struct thread_s *thread);

inline void cpu_context_destroy(struct cpu_context_s *ctx);

inline void cpu_context_set_tid(struct cpu_context_s *ctx, reg_t tid);

inline void cpu_context_set_pmm(struct cpu_context_s *ctx, struct pmm_s *pmm);

inline void cpu_context_set_stackaddr(struct cpu_context_s *ctx, reg_t addr);

inline uint_t cpu_context_get_stackaddr(struct cpu_context_s *ctx);

inline void cpu_context_dup_finlize(struct cpu_context_s *dst, struct cpu_context_s *src);


/* Return processor id */
#if 0
static inline uint_t cpu_get_id(void)
{
	register uint_t proc_id;
	register uint_t arch_cid;

	__asm__ volatile ("mfc0    %0,  $15, 1" : "=&r" (proc_id));

	proc_id &= 0x3;//FIXME: assuming 4 proc per cluster

	return arch_cpu_gid(cid, lid)
  
	return (proc_id & 0xFFF);
}
#endif

uint_t cpu_gid_tbl[CPU_PER_CLUSTER] CACHELINE;

static inline gid_t cpu_get_lid(void)
{
	register uint_t proc_id;

	__asm__ volatile ("mfc0    %0,  $15, 1" : "=&r" (proc_id));

	return (proc_id & 0x3);//FIXME: assuming 4 proc per cluster
}

//return cpu_gid!
static inline uint_t cpu_get_id(void)
{
	return cpu_gid_tbl[cpu_get_lid()];

}

extern 
global_t __current_cid;

static inline cid_t cpu_get_cid(void)
{
	return __current_cid.value;
}

/* Return current value of stack register */
static inline uint_t cpu_get_stack(void)
{
	register uint_t sp;
  
	__asm__ volatile
		("or    %0,   $0,      $29            \n" : "=&r" (sp));
  
	return sp;
}


/* Set new stack pointer and return old one */
static inline uint_t cpu_set_stack(void* new_val)
{
	register uint_t sp;
  
	__asm__ volatile
		("or    %0,   $0,      $29            \n"
		 "or    $29,  $0,      %1             \n"
		 : "=&r" (sp) : "r" (new_val));
  
	return sp;
}


/* Return current execution cycle number */
static inline uint_t cpu_time_stamp(void)
{
	register uint_t cycles;
  
	__asm__ volatile 
		(".set noreorder                      \n"
		 "mfc0   %0,  $9                      \n"
		 "nop                                 \n"
		 ".set reorder                        \n"
		 : "=&r" (cycles));
  
	return cycles;
}

/* Return pointer to current thread */
static inline struct thread_s* cpu_current_thread (void)
{
	register void * thread_ptr;
 
	__asm__ volatile ("mfc0    %0,  $4,  2"   : "=&r" (thread_ptr));

	return thread_ptr;
}

/* Set current thread pointer */
static inline void cpu_set_current_thread (struct thread_s *thread)
{ 
	__asm__ volatile ("mtc0    %0,  $4,  2" : : "r" (thread));
}

static inline bool_t cpu_isBad_addr(void *addr)
{
	return (reg_t)addr & 0x3;
}

static inline void cpu_fpu_enable(void)
{
	__asm__ volatile 
		(
			".set noat                         \n"
			"lui    $27,    0x2000             \n"
			"mfc0   $1,     $12                \n"
			"or     $27,    $1,    $27         \n"
			"mtc0   $27,    $12                \n"
			".set at                           \n");
}

static inline void cpu_fpu_disable(void)
{
	__asm__ volatile 
		(
			".set noat                         \n"
			"lui    $27,    0xDFFF             \n"
			"ori    $27,    $27,   0xFFFF      \n"
			"mfc0   $1,     $12                \n"
			"and    $27,    $1,    $27         \n"
			"mtc0   $27,    $12                \n"
			".set at                           \n");
}

/* Disable a specific IRQ number and save old SR state */
static inline void cpu_disable_single_irq (uint_t irq_num, uint_t *old)
{
	register uint_t sr;

	__asm__ volatile
		(".set noat                           \n"
		 "mfc0   $2,     $12                  \n"
		 "or     $1,     $0,     %1           \n"
		 "addiu  $1,     $1,     10           \n"
		 "ori    $3,     $0,     1            \n"
		 "sllv   $3,     $3,     $1           \n"
		 "li     $1,     0xFFFFFFFF           \n"
		 "xor    $1,     $3,     $1           \n"
		 "and    $1,     $1,     $2           \n"
		 "mtc0   $1,     $12                  \n"
		 "or     %0,     $0,     $2           \n"
		 ".set at                             \n"
		 : "=&r" (sr) : "r" (irq_num) : "$2", "$3");

	if(old) *old = sr;
}

/* Enable a specific IRQ number and save old SR state */
static inline void cpu_enable_single_irq(uint_t irq_num, uint_t *old)
{
	register uint_t sr;
  
	__asm__ volatile 
		(".set noat                          \n"
		 "mfc0   $2,     $12                 \n"
		 "or     $1,     $0,     %1          \n"
		 "addiu  $1,     $1,     10          \n"
		 "ori    $3,     $0,     1           \n"
		 "sllv   $3,     $3,     $1          \n"
		 "or     $3,     $3,     $2          \n"
		 "mtc0   $3,     $12                 \n"
		 "or     %0,     $0,     $2          \n"
		 ".set at                            \n"
		 : "=&r" (sr) : "r" (irq_num) : "$2","$3");
  
	if(old) *old = sr;
}

/* Disable all IRQs and save old SR state */
/* TODO: this function discard the CU3 access bit */
static inline void cpu_disable_all_irq (uint_t *old)
{
	register unsigned int sr;
	
	__asm__ volatile 
		(".set noat                          \n"
		 ".set noreorder                     \n"
		 "mfc0   $1,     $12                 \n"
		 "nop                                \n"
		 ".set reorder                       \n"
		 "or     %0,     $0,     $1          \n"
		 "srl    $1,     $1,     1           \n"
		 "sll    $1,     $1,     1           \n"
		 "mtc0   $1,     $12                 \n"
		 ".set at                            \n"
		 : "=&r" (sr));

	if(old) *old = sr;
}

/* Enable all IRQs and save old SR state */
static inline void cpu_enable_all_irq (uint_t *old)
{
	register unsigned int sr;
 
	__asm__ volatile 
		(".set noat                          \n"
		 ".set noreorder                     \n"
		 "mfc0   $1,     $12                 \n"
		 "nop                                \n"
		 "or     %0,     $0,     $1          \n"
		 "ori    $1,     $1,     0x1         \n"
		 "mtc0   $1,     $12                 \n"
		 "nop                                \n"
		 ".set reorder                       \n"
		 ".set at                            \n"
		 : "=&r" (sr));
  
	if(old) *old = sr;
}

/* Restore old SR state, saved by precedents functions */
static inline void cpu_restore_irq(uint_t old)
{
	__asm__ volatile 
		(".set noat                          \n"
		 ".set noreorder                     \n"
		 "mfc0    $1,    $12                 \n"
		 "ori     $2,    $0,      0xFF       \n"
		 "and     $2,    $2,      %0         \n"
		 "or      $1,    $1,      $2         \n"
		 "mtc0    $1,    $12                 \n"
		 ".set reorder                       \n" 
		 ".set at                            \n"
		 : : "r" (old) : "$2");
}


static inline void cpu_spinlock_init(void *lock, uint_t val)
{
	*((uint_t*)lock) = val;
}

static inline void cpu_spinlock_destroy(void *lock)
{
	*((uint_t*)lock) = 0;       /* ToDo: Put dead value instead */
}

/* Try to take a spinlock */
static inline bool_t cpu_spinlock_trylock (void *lock)
{ 
	register bool_t state = 0;
  
	__asm__ volatile
		("or      $2,      $0,       %1      \n"
		 "1:                                 \n"
		 "ll      $3,      ($2)              \n"
		 ".set noreorder                     \n"
		 "beq     $3,      $0,       2f      \n"
		 "or      %0,      $0,       $3      \n"                        
		 "j       3f                         \n"
		 "2:                                 \n"
		 "addi    $3,      $0,       -1      \n"
		 "sc      $3,      ($2)              \n"
		 "nop                                \n"
		 "beq     $3,      $0,       1b      \n"
		 "sync                               \n"
		 "3:                                 \n"
		 ".set reorder                       \n"
		 : "=&r" (state) : "r" (lock) : "$2","$3");
  
	return state;
}

#if 0
/** Lock a spinlock  */
extern uint_t rand();
//extern int __kperror (const char *fmt, ...);
static inline void cpu_spinlock_lock(void *lock)
{
#if CONFIG_CPU_BACKOFF_SPINLOCK
	volatile uint_t i;
  
	//register uint_t backoff;

#endif
	while((cpu_spinlock_trylock(lock)))
	{
#if CONFIG_CPU_BACKOFF_SPINLOCK
		//i = 1 << backoff;
		i = rand() % 100;
		for(; i > 0; i--);
#endif
	}
}
#endif

#if 0
/* Unlock a spinlock */
static inline void cpu_spinlock_unlock (void *lock)
{ 
  
	__asm__ volatile
		("or      $2,      $0,       %0      \n"
		 "1:                                 \n"
		 "ll      $3,      ($2)              \n"
		 "nop                                \n"
		 "xor     $3,      $3,       $3      \n"
		 "nop                                \n"
		 "sc      $3,      ($2)              \n"
		 "nop                                \n"
		 "beq     $3,      $0,       1b      \n"
		 "nop                                \n"
		 : : "r" (lock) : "$2","$3");
}
#else
static inline void cpu_spinlock_unlock (void *lock)
{ 
	__asm__ volatile
		("or  $2,   $0,   %0   \n"
		 "sync                 \n"
		 "sw  $0,   0($2)      \n"
		 "sync                 \n": : "r" (lock) : "$2");
}
#endif


static inline sint_t cpu_atomic_add(void *ptr, sint_t val)
{
	sint_t current;
 
	__asm__ volatile
		("1:                                 \n"
		 "ll      %0,      (%1)              \n"
		 "addu    $3,      %0,       %2      \n"
		 "sc      $3,      (%1)              \n"
		 "beq     $3,      $0,       1b      \n"
		 "nop                                \n"
		 "sync                               \n"
		 :"=&r"(current) : "r" (ptr), "r" (val) : "$3");

	return current;
}

static inline sint_t cpu_atomic_inc(void *ptr)
{
	return cpu_atomic_add(ptr, 1);
}

static inline sint_t cpu_atomic_dec(void *ptr)
{
	return cpu_atomic_add(ptr, -1);
}

static inline bool_t cpu_atomic_cas(void *ptr, sint_t old, sint_t new)
{
	bool_t isAtomic;
  
	__asm__ volatile
		(".set noreorder                     \n"
		 "sync                               \n"
		 "or      $8,      $0,       %3      \n"
		 "ll      $3,      (%1)              \n"
		 "bne     $3,      %2,       1f      \n"
		 "li      $7,      0                 \n"
		 "sc      $8,      (%1)              \n"
		 "or      $7,      $8,       $0      \n"
		 "sync                               \n"
		 ".set reorder                       \n"
		 "1:                                 \n"
		 "or      %0,      $7,       $0      \n"
		 : "=&r" (isAtomic): "r" (ptr), "r" (old) , "r" (new) : "$3", "$7", "$8");

	return isAtomic;
}

static inline void cpu_spinlock_lock(void *lock)
{
	register bool_t retval;

	while(1)
	{
		if(*((volatile uint_t *)lock) == 0)
		{
			retval = cpu_atomic_cas(lock, 0, -1);
			if(retval == true) break;
		}
	}
}

static inline uint_t cpu_load_word(void *ptr)
{
	register uint_t val;
  
	__asm__ volatile
		("lw      %0,      0(%1)             \n"
		 : "=&r" (val) : "r"(ptr));

	return val;
}

static inline void cpu_wait(uint_t val)
{
	__asm__ volatile
		("1:                                 \n"
		 "addiu   $2,      %0,      -1       \n"
		 "bne     $2,      $0,      1b       \n"
		 "nop                                \n"
		 :: "r"(val) : "$2");
}

static inline sint_t cpu_atomic_get(void *ptr)
{
	register sint_t val;

	__asm__ volatile
		("sync                               \n"
		 "or      $2,      $0,       %1      \n"
		 "lw      %0,      0($2)             \n"
		 :"=&r"(val) : "r" (ptr) : "$2");

	return val;
}

/* Cache operations */

static inline uint_t cpu_uncached_read(void *ptr)
{
	register uint_t val;

	__asm__ volatile
		("ll    %0,     (%1)                \n"
		 : "=&r"(val) : "r" (ptr)
			);

	return val;
}

static inline void cpu_invalid_dcache_line(void *ptr)
{
	__asm__ volatile
		("cache    %0,     (%1)              \n"
		 "sync                               \n"
		 : : "i" (0x11) , "r" (ptr)
			);
}


static inline void cpu_wbflush(void)
{
	__asm__ volatile
		("sync                               \n"::);
}

static inline void cpu_rdbar(void)
{
	__asm__ volatile("" ::: "memory");
}

/* Others private operations */
#define mips_set_cp2(reg,val,select)					\
	do{								\
		__asm__ volatile					\
			("mtc2     %0,     $%1,       %2      \n"	\
			 : : "r"(val), "i"(reg), "i"(select));		\
	}while(0)

#define mips_get_cp2(reg,select)					\
	({								\
		register reg_t __val;					\
		__asm__ volatile					\
			("mfc2     %0,     $%1,        %2      \n"	\
			 : "=&r"(__val) : "i"(reg), "i"(select));	\
		__val;							\
	})


static inline void cpu_power_idle(void)
{
	__asm__ volatile
		("wait                               \n"::);
}

static inline uint_t cpu_get_bad_vaddr(void)
{
	register uint_t bad_va;

	__asm__ volatile ("mfc0    %0,  $8" : "=&r" (bad_va));

	return bad_va;
}

#endif	/* _CPU_ASM_H_ */
