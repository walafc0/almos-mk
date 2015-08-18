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
  
   UPMC / LIP6 / SOC (c) 2009
   Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
*/

#ifndef _SETJMP_H_
#define _SETJMP_H_

#ifdef USR_LIB_COMPILER
#include <cpu-jmp.h>
#else
#include <sys/cpu-jmp.h>
#endif

typedef struct __jmp_buffer_s jmp_buf[1];

#define setjmp(env)         (__setjmp((env)))
#define longjmp(env, val)   __longjmp(env, val)



#endif	/* _SETJMP_H_ */
