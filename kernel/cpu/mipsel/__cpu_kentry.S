/*
 * __cpu_kentry.S - unified kernel entry point
 * 
 * Copyright (c) 2007,2008,2009,2010,2011,2012 Ghassan Almaless
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

#include <cpu-regs.h>
#define _ALMOS_ASM_
#include <boot-info.h>

#-------------------------------------------------------------------------------
# Kernel entry point (Exception/Interrupt/System call) for MIPS32 ISA compliant 
# processors. The base address of the segment containing this code
#-------------------------------------------------------------------------------
	.section   .kentry,"ax",@progbits
	.extern cpu_do_interrupt
	.extern cpu_do_exception
	.extern cpu_do_syscall
	.extern cpu_kentry
	.extern cpu_kexit
	.org 0x180
	.ent kentry
	.global kentry
	.global kentry_load
	.set noat
	.set noreorder

#define SAVE_SIZE      REGS_NR*4

#FIXME: PROC_WIDTH support only 4 proc clusters!
#define PROC_WIDTH 2
#define XY_WIDTH  8
#define XY_MASK   0xFF
#define MMU_MD_MASK  0xF
	
# Kernel Entry point
#-------------------------------------------------------------------------------
kentry:
	mfc0    $26,    $12			# read SR
	andi    $26,    $26,	0x10		# KSU bitmask
	beq     $26,    $0,	KERNEL_MODE
	ori     $26,    $0,	0x3		# MMU OFF value
	

LOAD_KERNEL_STACK:
	mtc2    $26,    $1			# set MMU OFF
	nop
	mfc0    $26,    $4,        2	        # read current thread pointer ($26)
	sw      $27,    (TLS_K1*4)($26)		# save user value

	#$27 register can now be used
	sw      $29,    (SP*4)($26)     	# save user stack
	lw	$29,	(KSP*4)($26)		# read kernel stack

	ori     $27,    $0,	0xF		# MMU old value: assumed ON
	sw      $27,    (MMU_MD*4)($26)		# save MMU MODE 

	j	UNIFIED_MODE
	or	$27,	$0,	$26		# pointer to uzone ($27)
	
#Use only $26 to save two values: MMU_MODE (4 bits) and XY_DATA_EXT (8bits)
#$26 content (in hex): 0x00000MXY (M is the value of MMU_MODE and XY is ...)
#N.B: MMU MODE is also modified by cpu_do_exception
KERNEL_MODE:
	#save old PADDR_EXT (in $26 at bits 0-7)  
	and	$26,    $26,	$0		# $26 <= 0x00000000#?
	mfc2	$26,    $24			# $26 <= 0x??????XY 
	andi	$26,    $26,	XY_MASK		# $26 <= 0x000000XY 
	#set PADDR_EXT to point to local mem
	mfc0    $27,    $15,	1	        # read ebase
	andi	$27,	$27,	0xFFF		# extract arch cpu id		
	srl	$27,	$27,	PROC_WIDTH	# extract cluster xy
	mtc2	$27,    $24			# set XY PADDR_EXT

	#save old MMU MODE (in $26 at bits 8-11)
	mfc2    $27,    $1			# get MMU MODE old value
	andi	$27,	$27,	MMU_MD_MASK	# $27 <= 0x0000000M
	sll	$27,	$27,	XY_WIDTH	# $27 <= 0x00000M00
	or	$26,    $26,	$27		# $26 <= 0x00000MXY <= (0x000000XY or 0x00000M00)
	#set MMU OFF
	ori     $27,    $0,	0x3		# MMU OFF value
	mtc2    $27,    $1			# set MMU OFF

	#save old stack, MMU_MODE and PADDR_EXT
	addiu   $27,    $29,	-(SAVE_SIZE)	# allocate a region on stack ($27)
	sw	$29,	(SP*4)($27)		# save old stack ($29 can be used)

	srl	$29,	$26,	XY_WIDTH	# $29 <= 0x0000000M 
	sw      $29,    (MMU_MD*4)($27)		# save MMU MODE
	andi	$26,	$26,	XY_MASK		# $26 <= 0x000000XY
	sw	$26,	(DP_EXT*4)($27)		# save old DATA PADDR EXT (0x000000XY) 

	or	$29,	$27,	$0		# set new stack
	mfc0    $26,    $4,	2	        # read current thread pointer ($26)
	
#$27: zone for saving the cpu registers
#$26: current thread pointer ($this)
#$29: kstack
UNIFIED_MODE:	
	sw	$1,	(AT*4)($27)
	sw	$2,	(V0*4)($27)
	sw	$3,	(V1*4)($27)
	sw	$4,	(A0*4)($27)
	sw	$5,	(A1*4)($27)
	sw	$6,	(A2*4)($27)
	sw	$7,	(A3*4)($27)
	sw	$8,	(T0*4)($27)
	sw	$9,	(T1*4)($27)
	sw	$10,	(T2*4)($27)
	sw	$11,	(T3*4)($27)
	sw	$12,	(T4*4)($27)
	sw	$13,	(T5*4)($27)
	sw	$14,	(T6*4)($27)
	sw	$15,	(T7*4)($27)
	sw	$24,	(T8*4)($27)
	sw	$25,	(T9*4)($27)
	sw	$16,	(S0*4)($27)
	sw	$17,	(S1*4)($27)
	sw	$18,	(S2*4)($27)
	sw	$19,	(S3*4)($27)
	sw	$20,	(S4*4)($27)
	sw	$21,	(S5*4)($27)
	sw	$22,	(S6*4)($27)
	sw	$23,	(S7*4)($27)
	sw	$30,	(S8*4)($27)
	sw	$28,	(GP*4)($27)
	sw	$31,	(RA*4)($27)		# save RA

	mfc0	$16,    $14			# Read EPC
	sw	$16,	(EPC*4)($27)		# Save EPC
	mflo	$14				# read LO
	sw	$14,	(LO*4)($27)		# save LO
	mfhi	$15				# read HI
	sw	$15,	(HI*4)($27)		# save HI

	mfc0	$18,	$12			# Read current SR
	sw	$18,	(SR*4)($27)		# Save SR
	srl	$3,	$18,	5		# put SR in kernel mode, IRQ disabled, clear exl
	sll	$3,	$3,	5		# ...
	mtc0	$3,	$12			# Set new SR

	mfc0    $17,    $13            		# read CR
	sw	$17,	(CR*4)($27)		# Save CR
	andi    $1,     $17,	0x3F		# apply exception cause mask (used later)

#if CPU_IN_KERNEL
	# First signal that we are entring the kernel
	mfc0	$4,	$15,	1		# arg is cpu_id	
	andi    $4,     $4,     0x3		# mask all but CPU_local_id
	mfc0    $5,     $4,	2	        # read current thread pointer 
	jal	cpu_kentry
	addiu	$29,	$29,	-8
	addiu	$29,	$29,	8
#endif

	# Second, depending on the cause, call
	# the apropriate function 
	ori	$8,     $0,     0x20		# cause syscall
	beq	$8,	$1,	cause_sys	
	or	$19,	$0,	$27		# for kentry_exit
	mfc0	$5,	$15,	1		# arg is cpu_id	
	andi    $5,     $5,     0x1FF		# mask all but CPUnum (X,Y,lid)
	or	$4,	$0,	$26		# first arg is this
	beq     $1,    	$0,	cause_int
	or	$6,	$0,	$27		# 3th arg is regs_tbl
	
# Exceptions Handler
# ---------------------------------------------	
	la	$1,	cpu_do_exception
	jalr	$1	
	addiu	$29,	$29,   -3*4	
	j	kentry_exit
	addiu	$29,	$29,	3*4

# System Call Handler
# ------------------------------------------------
cause_sys:
	la	$14,	cpu_do_syscall
	addiu	$29,	$29,	-4
	jalr	$14
	or	$4,	$0,	$27
	j	kentry_exit
	or	$19,	$0,	$2
	
# Interrupts Handler
# ----------------------------------------------
cause_int:
	la	$1,	cpu_do_interrupt
	srl	$7,	$17,	10		# extract IP state, 4th arg
	addiu	$29,	$29,	-4*4		# cpu_interrupt has 4 arg
	jal	$1
	andi    $7,     $7,	0x3F		# 6 HW IRQ LINES, 2th arg is irq_state

# Kentry exit
# ----------------------------------------------
kentry_exit:
#if CPU_IN_KERNEL
	# First signal that we are exiting from the kernel
	mfc0	$4,	$15,	1		# arg is cpu_id	
	andi    $4,     $4,     0x3             # mask all but CPU_local_id
	mfc0    $5,     $4,	2	        # read current thread pointer 
	jal	cpu_kexit
	addiu	$29,	$29,	-8
	addiu	$29,	$29,	8
#endif
	# Then load context ...
	or	$27,	$0,	$19
	lw	$1,	(AT*4)($27)		
	lw	$2,	(V0*4)($27)
	lw	$3,	(V1*4)($27)
	lw	$4,	(A0*4)($27)
	lw	$5,	(A1*4)($27)
	lw	$6,	(A2*4)($27)
	lw	$7,	(A3*4)($27)
	lw	$8,	(T0*4)($27)
	lw	$9,	(T1*4)($27)
	lw	$10,	(T2*4)($27)
	lw	$11,	(T3*4)($27)
	lw	$12,	(T4*4)($27)
	lw	$13,	(T5*4)($27)
	lw	$14,	(T6*4)($27)
	lw	$15,	(T7*4)($27)
	lw	$24,	(T8*4)($27)
	lw	$25,	(T9*4)($27)	
	lw	$18,	(S2*4)($27)
	lw	$19,	(S3*4)($27)
	lw	$20,	(S4*4)($27)
	lw	$21,	(S5*4)($27)
	lw	$22,	(S6*4)($27)
	lw	$23,	(S7*4)($27)
	lw	$30,	(S8*4)($27)
	lw	$28,	(GP*4)($27)
	lw	$31,	(RA*4)($27)
	lw	$16,	(EPC*4)($27)		# load EPC
	lw	$29,	(SP*4)($27)		# restore SP
	lw	$17,	(SR*4)($27)		# load SR
	mfc0	$26,	$12			# Read current SR
	andi	$17,	$17,	0x1F
	mtc0	$16,	$14			# restore EPC
	or	$26,	$26,	$17
	lw	$16,	(LO*4)($27)		# load LO
	mtc0	$26,	$12			# setup new SR
	lw	$17,	(HI*4)($27)		# load HI
	mtlo	$16				# restore LO
	mthi	$17				# restore HI	

	lw	$16,	(S0*4)($27)
	lw	$17,	(S1*4)($27)

#TODO: optimize
	lw	$26,	(MMU_MD*4)($27)		# load MMU MODE
	andi	$26,	$26,	0xc
	beq     $26,    $0,	OUT_MMU_3	# both MSB are 0 (the first two LSB are always set)
	andi	$26,	$26,	0x8
	beq     $26,    $0,	OUT_MMU_7	# first MSB is 0 (bit 2 is set)
	lw	$26,	(DP_EXT*4)($27)		# load DP_EXT

# Possible value for MMU:
#In kernel mode : 0x7/0x3
#In user mode : 0xF

# DP_EXT can either be local or remote
#Once these register set we can no longuer 
#access global data

	#If MMU mode was 0xF
	lw      $27,    (TLS_K1*4)($27)		 
	mtc2	$26,    $24			# restore DP EXT
	ori	$26,	$26,	0xF
	mtc2    $26,    $1			# set MMU MODE
	j	OUT_KENTRY
	nop

OUT_MMU_7:
	#If MMU mode was 0x7 (uspace)
	lw      $27,    (TLS_K1*4)($27)		 
	mtc2	$26,    $24			# restore DP EXT
	ori	$26,	$26,	0x7
	mtc2    $26,    $1			# set MMU MODE
	j	OUT_KENTRY
	nop

OUT_MMU_3:
	#If MMU mode was 0xF
	lw	$26,	(DP_EXT*4)($27)		# load DP_EXT
	lw      $27,    (TLS_K1*4)($27)		 
	mtc2	$26,    $24			# restore DP EXT
	ori	$26,	$26,	0x3
	mtc2    $26,    $1			# set MMU MODE
	nop

OUT_KENTRY:
	nop
        eret


  .end kentry
  .set reorder
  .set at

	.ent kentry_load
kentry_load:
	#theses nops are required to load the eret instruction
	#while we are in virtual mode (processor pipeline) ?
	mtc2     $26,    $1	       # set MMU MODE
	nop			
	nop
        eret
  .end kentry_load

#-------------------------------------------------------------------------------

