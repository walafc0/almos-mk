/*
 * kern/hal-cpu.h - CPU related Hardware Abstraction Layer
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

#ifndef  _HAL_CPU_H_
#define  _HAL_CPU_H_

#ifndef  _CPU_H_
#error cpu-hal.h cannot be included directly, use cpu.h instead
#endif

#include <types.h>

/* Structures used */
struct cpu_info_s;
struct irq_action_s;
struct thread_s;
struct pmm_s;
typedef unsigned long reg_t;

/* Structures, functions and macros provided */

/* CPU MAX IRQs lines number */
#define CPU_HW_IRQ_NR

/* CPU used IRQs lines number */
#define CPU_IRQ_NR

/* User level with interrupts enabled */
#define CPU_USR_MODE

/* Kernel level with interrupts disabled */
#define CPU_SYS_MODE

struct cpu_context_s;
struct cpu_uzone_s;
struct cpu_arch_context_s;

struct cpu_uzone_attr_s
{
	reg_t stack_top;
	reg_t tls;
};

/* Signal notification */
void cpu_signal_notify(struct thread_s *this, void *handler, uint_t sig);
void cpu_context_set_sigreturn(struct cpu_context_s *ctx, void *sigreturn_func);

/* Execution context */
void cpu_context_init(struct cpu_context_s *ctx, struct thread_s *thread);
void cpu_context_load(struct cpu_context_s *ctx);
void cpu_context_destroy(struct cpu_context_s *ctx);
void cpu_context_set_tid(struct cpu_context_s *ctx, reg_t tid);
void cpu_context_set_pmm(struct cpu_context_s *ctx, struct pmm_s *pmm);
void cpu_context_set_stackaddr(struct cpu_context_s *ctx, reg_t addr);
uint_t cpu_context_get_stackaddr(struct cpu_context_s *ctx);
void cpu_context_dup_finlize(struct cpu_context_s *dst, struct cpu_context_s *src);
extern uint_t cpu_context_save(struct cpu_context_s *ctx);
extern void cpu_context_restore(struct cpu_context_s *ctx, uint_t val);

/* User Zone */
void cpu_uzone_init(struct cpu_uzone_s *uzone);
void cpu_uzone_destroy(struct cpu_uzone_s*uzone);
void cpu_uzone_dup(struct cpu_uzone_s *dst, struct cpu_uzone_s *src);
error_t cpu_uzone_getattr(struct cpu_uzone_s *uzone, struct cpu_uzone_attr_s *attr);
error_t cpu_uzone_setattr(struct cpu_uzone_s *uzone, struct cpu_uzone_attr_s *attr);
extern void cpu_fpu_context_save(struct cpu_uzone_s *uzone);
extern void cpu_fpu_context_restore(struct cpu_uzone_s *uzone);

/* Time, identity & special registers */
static inline gid_t cpu_get_id(void);	/* return cpu_gid */

static inline gid_t cpu_get_lid(void);	/* return cpu_lid */

static inline cid_t cpu_get_cid(void);

static inline uint_t cpu_time_stamp(void);

static inline struct thread_s* cpu_current_thread(void);

static inline void cpu_set_current_thread(struct thread_s *thread);

static inline bool_t cpu_isBad_addr(void *addr);

static inline void cpu_fpu_enable(void);

static inline void cpu_fpu_disable(void);

static inline uint_t cpu_get_stack(void);

static inline uint_t cpu_set_stack(void* new_val);

extern error_t cpu_copy_from_uspace(void *dst, void *src, uint_t count);//src is in uspace
extern error_t cpu_copy_to_uspace(void *dst, void *src, uint_t count);//dest is in uspace
extern error_t cpu_uspace_strlen(char *str, uint_t *len);//FIXME: rename as cpu_strlen_uspace

/* IRQ register/enable/disable/restore */

static inline void cpu_disable_single_irq(uint_t irq_num, uint_t *old);

static inline void cpu_enable_single_irq(uint_t irq_num, uint_t *old);

static inline void cpu_disable_all_irq(uint_t *old);

static inline void cpu_enable_all_irq(uint_t *old);

static inline void cpu_restore_irq(uint_t old);


/* Spinlock trylock/unlock/init */

static inline void cpu_spinlock_lock(void *lock);

static inline bool_t cpu_spinlock_trylock(void *lock);

static inline void cpu_spinlock_unlock(void *lock);

static inline void cpu_spinlock_init(void *lock, uint_t val);

static inline void cpu_spinlock_destroy(void *lock);


/* Atomic operations */

static inline sint_t cpu_atomic_inc(void *ptr);

static inline sint_t cpu_atomic_add(void *ptr, sint_t val);

static inline sint_t cpu_atomic_dec(void *ptr);

static inline bool_t cpu_atomic_cas(void *ptr, sint_t old, sint_t new);

static inline sint_t cpu_atomic_get(void *ptr);


/* Cache operations */
static inline uint_t cpu_uncached_read(void *ptr);
static inline void cpu_invalid_dcache_line(void *ptr);
static inline void cpu_wbflush(void);
static inline void cpu_rdbar(void);


#include <cpu-asm.h>

#endif	/* _HAL_CPU_H_ */
