/*
   This file is part of MutekP.
  
   MutekP is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   MutekP is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with MutekP; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
   UPMC / LIP6 / SOC (c) 2009
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#ifndef _CPU_SYSCALL_H_
#define _CPU_SYSCALL_H_

#include <sys/types.h>


static inline void cpu_set_tls(void *ptr)
{
  asm volatile ("or   $27,  $0,  %0" :: "r" ((unsigned long)ptr));
}

static inline void* cpu_get_tls(void)
{
  register unsigned long ptr;
  asm volatile ("or   %0,  $0,  $27" :"=&r" (ptr));
  return (void*) ptr;
}

static inline void cpu_invalid_dcache_line(void *ptr)
{
  __asm__ volatile
    ("cache    %0,     (%1)              \n"
     "sync                               \n"
     : : "i" (0x11) , "r" (ptr)
    );
}

static inline bool_t cpu_atomic_cas(void *ptr, sint_t old, sint_t new)
{
  bool_t isAtomic;

  __asm__ volatile
    (".set noreorder                     \n"
     "sync                               \n"
     "or      $8,      $0,       %3      \n"
     "lw      $3,      (%1)              \n"
     "bne     $3,      %2,       1f      \n"
     "li      %0,      0                 \n"
     "ll      $3,      (%1)              \n"
     "bne     $3,      %2,       1f      \n"
     "li      %0,      0                 \n"
     "sc      $8,      (%1)              \n"
     "or      %0,      $8,       $0      \n"
     "sync                               \n"
     ".set reorder                       \n"
     "1:                                 \n"
     : "=&r" (isAtomic): "r" (ptr), "r" (old) , "r" (new) : "$3", "$8"
     );

  return isAtomic;
}

static inline sint_t cpu_atomic_add(void *ptr, sint_t val)
{
  sint_t current;
 
   __asm__ volatile
     ("1:                                 \n"
      "ll      %0,      (%1)              \n"
      "addu    $3,      %0,       %2      \n"
      "sc      $3,      (%1)              \n"
      "beq     $3,      $0,       1b      \n"
      "sync                               \n"
      "cache   0x11,    (%1)              \n"
      :"=&r"(current) : "r" (ptr), "r" (val) : "$3"
      );

  return current;
}

static inline bool_t cpu_spinlock_trylock(void *lock)
{ 
  register uint_t retval;

  retval = false;

  //if(*((volatile uint_t *)lock) == 0)
  retval = cpu_atomic_cas(lock, 0, 1);
  
  if(retval == false) return true;

  cpu_invalid_dcache_line(lock);
  return false;
}

static inline void cpu_spinlock_lock(void *lock)
{
  register bool_t retval;

  while(1)
  {
    if(*((volatile uint_t *)lock) == 0)
    {
      retval = cpu_atomic_cas(lock, 0, 1);
      if(retval == true) break;
    }
  }

  cpu_invalid_dcache_line(lock);
}

static inline uint_t cpu_load_word(void *ptr)
{
  register uint_t val;
  
  __asm__ volatile
    ("lw      %0,      0(%1)             \n"
     : "=&r" (val) : "r"(ptr));

  return val;
}

static inline void cpu_active_wait(uint_t val)
{
  __asm__ volatile
    ("1:                                 \n"
     "addiu   $2,      %0,      -1       \n"
     "bne     $2,      $0,      1b       \n"
     "nop                                \n"
     :: "r"(val) : "$2");
}

static inline void cpu_wbflush(void)
{
  __asm__ volatile
    ("sync                               \n"::);
}

#if 0
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
     "ori     $3,      $0,       1       \n"
     "sc      $3,      ($2)              \n"
     "nop                                \n"
     "beq     $3,      $0,       1b      \n"
     "sync                               \n"
     "3:                                 \n"
     ".set reorder                       \n"
     : "=&r" (state) : "r" (lock) : "$2","$3"
     );
  
  return state;
}
#endif

/* Unlock a spinlock */
static inline void cpu_spinlock_unlock (void *lock)
{ 
  __asm__ volatile
    ("or  $2,   $0,   %0   \n"
     "sync                 \n"
     "sw  $0,   0($2)      \n"
     "sync                 \n": : "r" (lock) : "$2"
     );
}


/* System call */
void* cpu_syscall(void *arg0, void *arg1, void *arg2, void *arg3, int service_nr);

#define CPU_SET_GP(val)					\
  asm volatile ("add   $28,   $0,   %0": :"r" ((val)))


#endif	/* _CPU_SYSCALL_H_ */
