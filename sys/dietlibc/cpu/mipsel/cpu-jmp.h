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

#ifndef _CPU_JMP_H_
#define _CPU_JMP_H_

typedef unsigned int reg_t;

struct __jmp_buffer_s
{
  reg_t s0_16;
  reg_t s1_17;
  reg_t s2_18;
  reg_t s3_19;
  reg_t s4_20;
  reg_t s5_21;
  reg_t s6_22;
  reg_t s7_23;
  reg_t gp_28;
  reg_t sp_29;
  reg_t fp_30;
  reg_t ra_31;
}__attribute__ ((packed));


extern int  __setjmp (struct __jmp_buffer_s *env);
extern void __longjmp(struct __jmp_buffer_s *env, int val);

#endif	/* __CPU_JMP_H_ */
