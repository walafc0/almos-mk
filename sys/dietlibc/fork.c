/*
   This file is part of AlmOS.
  
   AlmOS is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   AlmOS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with AlmOS; if not, write to the Free Software Foundation,
   Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
  
   UPMC / LIP6 / SOC (c) 2010
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#include <errno.h>
#include <sys/syscall.h>
#include <cpu-syscall.h>
#include <pthread.h>
#include <unistd.h>

pid_t fork(void)
{
  pid_t pid;
  struct __pthread_tls_s *tls;

  tls = cpu_get_tls();

  pid = (pid_t) cpu_syscall((void*)__pthread_tls_get(tls,__PT_TLS_FORK_FLAGS),
			    (void*)__pthread_tls_get(tls,__PT_TLS_FORK_CPUID),
			    NULL,NULL,SYS_FORK);

  if(pid == 0)
	  __pthread_tls_init(tls);
 
  return pid;
}

