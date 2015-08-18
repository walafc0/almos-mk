/*
 * cpu-regs.h - mips register map
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

#ifndef _CPU_REGS_H_
#define _CPU_REGS_H_

#define      KSP          0
#define      AT           1
#define      V0           2
#define      V1           3
#define      A0           4 
#define      A1           5
#define      A2           6
#define      A3           7
#define      T0           8
#define      T1           9
#define      T2           10
#define      T3           11
#define      T4           12
#define      T5           13
#define      T6           14
#define      T7           15
#define      T8           16
#define      T9           17
#define      S0           18
#define      S1           19
#define      S2           20
#define      S3           21
#define      S4           22
#define      S5           23
#define      S6           24
#define      S7           25
#define      S8           26
#define      GP           27
#define      RA           28
#define      EPC          29
#define      CR           30
#define      SP           31
#define      SR           32
#define      LO           33
#define      HI           34
#define      TLS_K1       35
#define      DP_EXT	  36 //DATA PADDR EXTENSION
#define      MMU_MD	  37 //MMU MODE
#define      REGS_NR      38

#define CPU_IN_KERNEL 1

#endif	/* _CPU_REGS_H_ */
