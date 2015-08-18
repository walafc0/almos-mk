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
#error this file connot be included directly, use cpu.h instead
#endif

#include <config.h>
#include <cpu-internal.h>

/* CPU IRQ macros */
#undef  CPU_HW_IRQ_NR
#define CPU_HW_IRQ_NR  1

#undef  CPU_IRQ_NR
#define CPU_IRQ_NR  CPU_HW_IRQ_NR

/* SR USR/SYS mask */
#undef  CPU_USR_MODE
#define CPU_USR_MODE  0x01

#undef  CPU_SYS_MODE
#define CPU_SYS_MODE  0x00

/* Porcessor Context*/
struct cpu_context_s
{
	/* Current CPU Context */
	reg_t ebx,esp,ebp,edi,esi;
	reg_t eip,cr3;
	reg_t loadable;
	reg_t tss_ptr;
	reg_t thread_ptr;
	reg_t kstack_ptr;

	/* Initialization Info */
	reg_t stack_ptr;
	reg_t cpu_id;
	reg_t mode;
	reg_t entry_func;
	reg_t exit_func;
	reg_t arg1;
	reg_t arg2;
}__attribute__ ((packed));

extern void __cpu_context_init(struct cpu_context_s* ctx, struct cpu_context_attr_s *attr);

static inline void cpu_context_init(struct cpu_context_s* ctx, struct cpu_context_attr_s *attr)
{
	__cpu_context_init(ctx,attr);
}

inline void cpu_context_destroy(struct cpu_context_s *ctx)
{
}


extern void *memcpy(void *, void*, unsigned long);
static void cpu_context_dup(struct cpu_context_s *dest,
			    struct cpu_context_s *src)
{
	memcpy(dest, src, (unsigned long)sizeof(*src));
}


/* Return processor id */
static inline uint_t cpu_get_id(void)
{
	register unsigned int proc_id = 0;

	//  asm volatile ("mfc0    %0,  $0" : "=r" (proc_id));
  
	return proc_id;
}

/* Return current execution cycle number */
static inline uint_t cpu_time_stamp(void)
{
	register uint32_t lo;
	register uint32_t hi;

	__asm__ volatile 
		("rdtsc" : "=a" (lo), "=d" (hi));

	return lo;
}

/* Return pointer to current thread */
inline struct thread_s* cpu_current_thread (void)
{
	register struct cpu_tss_s *tss;
  
	tss = cpu_get_tss(cpu_get_id());
	return (struct thread_s*)tss->esp_r1;
}

/* Set current thread pointer */
static inline void cpu_set_current_thread (struct thread_s *thread)
{ 
	//asm volatile ("mtc0    %0,  $4,  2" : : "r" (thread));
}

static inline bool_t cpu_isBad_addr(void *addr)
{
	return false;
}

static inline void cpu_fpu_enable(void)
{
}

static inline void cpu_fpu_disable(void)
{
}

static inline uint_t cpu_get_stack(void)
{
	return 0;
}

static inline uint_t cpu_set_stack(void* val)
{
	return 0;
}

/* Disable all IRQs and save old IF state */
static inline void cpu_disable_all_irq (uint_t *old)
{
	register unsigned int eflags;

	__asm__ volatile
		("pushfl               \n"
		 "popl        %0       \n"
		 "cli                  \n"
		 : "=r" (eflags));

	if(old) *old = (eflags & 0x200) ? 1 : 0;
}

/* Enable all IRQs and save old IF state */
static inline void cpu_enable_all_irq (uint_t *old)
{
	register unsigned int eflags;

	__asm__ volatile
		("pushfl               \n"
		 "popl        %0       \n"
		 "sti                  \n"
		 : "=r" (eflags));

	if(old) *old = (eflags & 0x200) ? 1 : 0;
}

/* Disable a specific IRQ number and save old IF state */
static inline void cpu_disable_single_irq (uint_t irq_num, uint_t *old)
{
	cpu_disable_all_irq(old);
}

/* Enable a specific IRQ number and save old IF state */
static inline void cpu_enable_single_irq(uint_t irq_num, uint_t *old)
{
	cpu_enable_all_irq(old);
}

/* Restore old IF state, saved by precedents functions */
static inline void cpu_restore_irq(uint_t old)
{
	register unsigned int isIF;

	isIF = (old == 1) ? 1 : 0;

	__asm__ volatile
		("bt          $0,        %0             \n"
		 "jc          1f                        \n"
		 "cli                                   \n"
		 "jmp         2f                        \n"
		 "1:                                    \n"
		 "sti                                   \n"
		 "2:                                    \n"
		 : : "r" (isIF));
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
	register int8_t state;
   
	__asm__ volatile
		("lock bts    $1,        (%1)      \n"
		 "setc        %0                   \n"
		 : "=r" (state) : "r" (lock));

	return state;
}

/** Lock a spinlock  */
extern uint_t rand();
//extern int __kperror (const char *fmt, ...);
static inline void cpu_spinlock_lock(void *lock)
{
#if CONFIG_CPU_BACKOFF_SPINLOCK
	volatile uint_t i;
	register uint_t backoff;

	backoff = cpu_get_id();//rand() % 5;
#endif
	while((cpu_spinlock_trylock(lock)))
	{
#if CONFIG_CPU_BACKOFF_SPINLOCK
		i = 1 << backoff;
		backoff ++;
		for(; i > 0; i--);
#endif
	}
}


/* Unlock a spinlock */
static inline void cpu_spinlock_unlock (void *lock)
{ 
	__asm__ volatile
		("lock btr    $1,        (%0)      \n"
		 : : "r" (lock));
}

static inline sint_t cpu_atomic_add(void *ptr, sint_t val)
{
	register  sint_t current;

	__asm__ volatile
		("movl       %1,     %%ecx        \n"
		 "movl       %2,     %%eax        \n"
		 "lock xaddl %%eax,  (%%ecx)      \n"
		 "movl       %%eax,  %0           \n" 
		 : "=r" (current) : "r" (ptr), "r" (val) : "%ecx", "%eax");

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

static inline sint_t cpu_atomic_set(void *ptr, sint_t new_val)
{
	register sint_t old_val = 0;

	__asm__ volatile
		("movl        %1,        %%ecx        \n"
		 "movl        %2,        %%eax        \n"
		 "lock xchg   %%eax,      (%%ecx)      \n"
		 "movl        %%eax,      %0          \n"        
		 : "=r" (old_val) : "r" (ptr), "r" (new_val) : "%ecx", "%eax");
	
	return old_val;
}

static inline sint_t cpu_atomic_get(void *ptr)
{
	return *((sint_t*)ptr);
}

/* Cache operations */
static inline void cpu_invalid_dcache_line(void *ptr)
{
#if 0
	__asm__ volatile
		("cache    %0,     (%1)             \n"
		 : : "i" (0x11) , "r" (ptr));
#endif
}


#endif	/* _CPU_ASM_H_ */
