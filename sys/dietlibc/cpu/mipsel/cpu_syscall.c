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
  
   UPMC / LIP6 / SOC (c) 2008
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#include <errno.h>
#include <sys/syscall.h>
#include <cpu-syscall.h>
#include <unistd.h>

#ifdef __mips__

void* __attribute__((noinline)) 
cpu_syscall(void *arg0, void *arg1, void *arg2, void *arg3, int service_nr)
{
  register unsigned long err;
  register unsigned long val;
  register void *errno_ptr;

  /* First of all, call the kernel */
  __asm__ volatile
    ("or   $2,   $0,    %2   \n"
     "syscall                \n"
     "or   %0,   $0,    $2   \n"
     "or   %1,   $0,    $3   \n"
     :"=&r"(val), "=&r"(err): "r"(service_nr): "$2","$3","$4","$5","$6","$7");
  /* ----------------------------- */

  errno_ptr = __errno_location();
  *(int*)errno_ptr = err;
  return (void*)val;
}
#endif


#if defined(__i386__) || defined(__x86_64__) || defined(__ia64__)
void* __attribute__((noinline))
cpu_syscall(void *arg0, void *arg1, void *arg2, void *arg3, int service_nr)
{
  register unsigned long err;
  register unsigned long val;

  __asm__ volatile
    ("int  $0x60         \n"
     "movl %%eax, %0     \n"
     "movl %%esi, %1     \n"
     :"=r"(val), "=r"(err)
     :"a"(service_nr), "b"(arg0), "c"(arg1), "d"(arg2), "D"(arg3)
     :"%esi");

  errno = err;
  return (void*)val;
}
#endif
