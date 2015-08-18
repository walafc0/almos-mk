/* Copyright (c) 2007-2009, Stanford University
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Stanford University nor the names of its 
*       contributors may be used to endorse or promote products derived from 
*       this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/ 

/**
 * define __x86_64__ for x86-64
 * define CPU_V9 for sparc
 */
#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>
#include <cpu-syscall.h>

static inline void spin_wait(int n)
{
  volatile int tmp = n;
  
  while(tmp > 0)
  {
    tmp--;
    cpu_wbflush();
  }
}

#define set_and_flush(x,y)		\
  do {					\
    void*	p;			\
    p = &(x);				\
    cpu_wbflush();			\
    (x) = (y);				\
    flush(p);				\
  } while (0)

/* a bunch of atomic ops follow... */

static inline void flush(void* addr)
{
  __asm__ volatile
    ("cache    %0,     (%1)              \n"
     "sync                               \n"
     : : "i" (0x11) , "r" (addr)
    );
}

static inline uintptr_t atomic_read(void* addr) { return cpu_load_word(addr); }


/* returns zero if already set, returns nonzero if not set */
/* we don't use bts because it is super-slow */


/* adds 1 to value pointed to in 'n', returns old value */
static inline unsigned int fetch_and_inc(unsigned int* n)
{
  unsigned int	oldval;
  
  return (unsigned) cpu_atomic_add(n, 1);
}

/* returns true on swap */
static inline int cmp_and_swp(uintptr_t v, uintptr_t* cmper, uintptr_t matcher)
{
  return cpu_atomic_cas(cmper, matcher, v);
}

static inline int test_and_set(uintptr_t* n)
{
  return cmp_and_swp(1, n, 0);
}
/**
 * @n - value to store
 * @v - location to store into
 * @return previous value in v
 */
static inline uintptr_t atomic_xchg(uintptr_t n, uintptr_t* v)
{
  uintptr_t old;
  bool_t isAtomic;
  
  do
  {
    old = cpu_load_word(v);
    isAtomic = cpu_atomic_cas(v, *v, n);
  }while(isAtomic == false);
  
  return old;
}

#endif
// vim: ts=4
