/*
 * __cpu_context.S - low-level context operations (see kern/hal-cpu.h)
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

#-------------------------------------------------------
##      cpu_context_load:                                  
#       load a processor context passed by argument.
#               $4:     struct cpu_context *ctx                       
#-------------------------------------------------------
        .section .text, "ax", @progbits
	.extern cpu_kexit
        .align 2
        .globl  cpu_context_load
        .ent    cpu_context_load
	
cpu_context_load:
        .set noreorder

#if CPU_IN_KERNEL
	# First signal that we are exiting from the kernel
	or	$16,	$4,	$0		# save $4
	lw      $5,	12*4($4)		# second arg is thread pointer
	mfc0	$4,	$15,	1		# first arg is cpu_lid	
	andi    $4,     $4,     0x3             # mask all but CPU_local_id
	jal	cpu_kexit
	addiu	$29,	$29,	-8
	addiu	$29,	$29,	8
	or	$4,	$16,	$0		# restore $4
#endif

	lw       $16,    8*4($4)       # get stack ptr
	mfc2     $26,    $0            # read current pgdir
	lw       $10,    12*4($4)      # get thread pointer
	lw       $27,    13*4($4)      # get pgdir
	lw	 $29,	 0($10)        # setup kernel stack
	mtc0     $10,    $4,   	  2    # set thread pointer	
	sw       $0,     14*4($4)      # reset loadable flag
	beq      $26,    $27,     1f
	lw	 $31,	 15*4($4)      # get exit_func
	mtc2     $27,    $0            # setup new pgdir
1:
	lw	 $29,	 0($10)        # setup kernel stack
	lw       $26,    23*4($4)      # MMU MODE (used by kentry_load)
	lw       $27,    9*4($4)       # get user value
	lw       $9,     10*4($4)      # get entry_func <-> $9
	lw       $8,     11*4($4)      # get mode <-> $8
	addiu    $16,    $16,     -2*4
	lw       $5,     17*4($4)      # get thread arg2
	lw       $4,     16*4($4)      # get thread arg1
	sw       $5,     1*4($16)
	sw       $4,     0*4($16)
	ori      $8,     $8,      0x2  # set EXL flag
	mtc0     $8,     $12           # set SR
	nop
	nop
	mtc0     $9,     $14           # set EPC with entry_func
	or	 $29,	 $0,	  $16  # set SP
	j	 kentry_load	       # kentry is accessible in virtual mode
	nop
        .set reorder
        .end cpu_context_load

#-------------------------------------------------------
#       cpu_context_save:                                 
#       Partial save of processor's registers.          
#       into Processor Work State (PWS).                
#       arguments:                                      
#               $4:     &PWS                            
#-------------------------------------------------------
        .section .text,"ax", @progbits
        .align 2
        .globl cpu_context_save
        .ent   cpu_context_save
        
cpu_context_save:
	.set noreorder
	sw      $16,   0*4($4)
        sw      $17,   1*4($4)
        sw      $18,   2*4($4)
        sw      $19,   3*4($4)
        sw      $20,   4*4($4)
        sw      $21,   5*4($4)
        sw      $22,   6*4($4)
        sw      $23,   7*4($4)
	mfc0	$26,   $4,	2      # get current thread pointer
        sw      $29,   8*4($4)
	mfc2    $27,   $0              # get pgdir
        sw      $30,   9*4($4)
        sw      $31,   10*4($4)
	sw      $26,   12*4($4)        # save current thread pointer
	sw      $27,   13*4($4)        # save pgdir
        jr      $31
	xor     $2,    $2,     $2
	.set reorder
        .end    cpu_context_save

#-------------------------------------------------------
#       cpu_context_restore
#       resume PWS.                                     
#       arguments:                                      
#               $4:     &PWS
#               $5:     val
#-------------------------------------------------------
        .section .text,"ax", @progbits
        .align 2

        .globl cpu_context_restore
        .ent   cpu_context_restore
	
cpu_context_restore:
        .set noreorder
	mfc2    $17,   $0              # read current pgdir
	lw      $30,   13*4($4)        # get pgdir
	lw      $16,   0*4($4)
	beq     $17,   $30,    	1f
	lw      $27,   14*4($4)        # get loadable flag
	mtc2    $30,   $0              # setup new pgdir
1:	
	bne     $27,   $0,      cpu_context_load
	lw      $17,   1*4($4)
        lw      $18,   2*4($4)
        lw      $19,   3*4($4)
        lw      $20,   4*4($4)
        lw      $21,   5*4($4)
        lw      $22,   6*4($4)
        lw      $23,   7*4($4)
	lw      $26,   12*4($4)        # get thread pointer	
	lw      $29,   8*4($4)         # get stack ptr
	mtc0	$26,   $4,	2
	lw      $31,   10*4($4)
	lw      $30,   9*4($4)
        jr      $31
        or      $2,    $0,     $5
        .set reorder
        .end    cpu_context_restore
#-------------------------------------------------------
#define FPU    	REGS_NR
#define op_reg(_op,_r)	_op	$f##_r,	((FPU + _r)*4)($4) 
	
#define FPU_CONTEXT_COMPUTE(op)	\
	op_reg(op,0) 			; \
	op_reg(op,1) 			; \
	op_reg(op,2) 			; \
	op_reg(op,3) 			; \
	op_reg(op,4) 			; \
	op_reg(op,5) 			; \
	op_reg(op,6) 			; \
	op_reg(op,7) 			; \
	op_reg(op,8) 			; \
	op_reg(op,9) 			; \
	op_reg(op,10) 			; \
	op_reg(op,11) 			; \
	op_reg(op,12) 			; \
	op_reg(op,13) 			; \
	op_reg(op,14) 			; \
	op_reg(op,15) 			; \
	op_reg(op,16) 			; \
	op_reg(op,17) 			; \
	op_reg(op,18) 			; \
	op_reg(op,19) 			; \
	op_reg(op,20) 			; \
	op_reg(op,21) 			; \
	op_reg(op,22) 			; \
	op_reg(op,23) 			; \
	op_reg(op,24) 			; \
	op_reg(op,25) 			; \
	op_reg(op,26) 			; \
	op_reg(op,27) 			; \
	op_reg(op,28) 			; \
	op_reg(op,29) 			; \
	op_reg(op,30) 			; \
	jr	$31			; \
	op_reg(op,31)
	
#-------------------------------------------------------
#       cpu_fpu_context_save
#       save fpu context of current thread.                                     
#       arguments:                                      
#               $4:     &cpu_uzone
#-------------------------------------------------------
        .section .text,"ax", @progbits
        .align 2

        .globl cpu_fpu_context_save
        .ent   cpu_fpu_context_save
	
cpu_fpu_context_save:
        .set noreorder
	FPU_CONTEXT_COMPUTE(swc1)
        .set reorder
        .end    cpu_fpu_context_save
#-------------------------------------------------------
	
#-------------------------------------------------------
#       cpu_fpu_context_restore
#       restore fpu of resumed context.                                     
#       arguments:                                      
#               $4:     &cpu_uzone
#-------------------------------------------------------
        .section .text,"ax", @progbits
        .align 2

        .globl cpu_fpu_context_restore
        .ent   cpu_fpu_context_restore
	
cpu_fpu_context_restore:
        .set noreorder
	FPU_CONTEXT_COMPUTE(lwc1)
        .set reorder
        .end    cpu_fpu_context_restore
#-------------------------------------------------------

