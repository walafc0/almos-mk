/*
 * __cpu_uspace.S - low-level user space access
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

#include <errno.h>

#-------------------------------------------------------
# error_t cpu_copy_from_uspace(dst,src,count):
# arguments:
# $4: dst address in kernel space
# $5: src address in user space
# $6: count of 4-bytes aligned words 
# $2: non zero value express an error code
#-------------------------------------------------------
        .section .uspace,"ax", @progbits
        .align 2
        .globl cpu_copy_from_uspace
        .globl cpu_copy_to_uspace
	.globl cpu_uspace_strlen
	.globl cpu_uspace_error
	.ent cpu_copy_from_uspace

cpu_copy_from_uspace:
	.set noreorder
	ori     $8,    $0,	0x7		# MMU MODE: DTLB ON
	ori     $9,    $0,	0x3		# MMU MODE: DTLB OFF

        andi	$2,	$5,	0x3
	andi	$7,	$4,	0x3
	addu	$2,	$2,	$7
	bne	$2,	$0,	2f //if $2 != 0 => non aligned addresses goto 2
	addu	$3,	$5,	$6
	ori	$3,	$0,	0xFFFC   // isize mask
	and	$3,	$6,	$3	 // isize
	addu	$3,	$5,	$3	 // ilimit
1:	
	beq	$5,	$3,	1f
	addiu	$4,	$4,	4
	mtc2    $8,    $1			# set DATA MMU ON
	lw	$7,	0($5)
	mtc2    $9,    $1			# set DATA MMU OFF
	addiu	$5,	$5,	4
	j	1b
	sw	$7,	-4($4)

1:
	andi	$3,	$6,	0x3
	addiu	$4,	$4,	-4
	addu	$3,	$3,	$5
2:	
	beq	$5,	$3,	1f
	addiu	$4,	$4,	1
	mtc2    $8,    $1			# set DATA MMU ON
	lbu	$7,	0($5)
	mtc2    $9,    $1			# set DATA MMU OFF
	addiu	$5,	$5,	1
	j	2b
	sb	$7,	-1($4)
1:	
        jr	$31
	or	$2,	$0,	$0
	.end cpu_copy_from_uspace

#-------------------------------------------------------
# error_t cpu_copy_to_uspace(dst,src,count):
# arguments:
# $4: dst address in user space
# $5: src address in kernel space
# $6: count of 4-bytes aligned words 
# $2: non zero value express an error code
#-------------------------------------------------------
	.ent cpu_copy_to_uspace
cpu_copy_to_uspace:
	.set noreorder
	ori     $8,    $0,	0x7		# MMU MODE: DTLB ON
	ori     $9,    $0,	0x3		# MMU MODE: DTLB OFF

        andi	$2,	$5,	0x3
	andi	$7,	$4,	0x3
	addu	$2,	$2,	$7
	bne	$2,	$0,	2f //if $2 != 0 => non aligned addresses goto 2
	addu	$3,	$5,	$6
	ori	$3,	$0,	0xFFFC   // isize mask
	and	$3,	$6,	$3	 // isize
	addu	$3,	$5,	$3	 // ilimit
1:	
	beq	$5,	$3,	1f
	addiu	$4,	$4,	4
	lw	$7,	0($5)
	addiu	$5,	$5,	4
	mtc2    $8,    $1			# set DATA MMU ON
	sw	$7,	-4($4)
	j	1b
	mtc2    $9,    $1			# set DATA MMU OFF

1:
	andi	$3,	$6,	0x3
	addiu	$4,	$4,	-4
	addu	$3,	$3,	$5
2:	
	beq	$5,	$3,	1f
	addiu	$4,	$4,	1
	lbu	$7,	0($5)
	addiu	$5,	$5,	1
	mtc2    $8,    $1			# set DATA MMU ON
	sb	$7,	-1($4)
	j	2b
	mtc2    $9,    $1			# set DATA MMU OFF
1:	
        jr	$31
	or	$2,	$0,	$0
	.end cpu_copy_to_uspace
#-------------------------------------------------------
# error_t cpu_uspace_strlen(char *str,uint_t *len)
#
# arguments:
# $4: str 
# $5: pointer to len
# $2: non zero return value express an error 
#-------------------------------------------------------
	.ent cpu_uspace_strlen
cpu_uspace_strlen:
	ori     $8,    $0,	0x7		# MMU MODE: DTLB ON
	ori     $9,    $0,	0x3		# MMU MODE: DTLB OFF

	beq	$4,	$0,	5f
	or	$3,	$0,	$4
3:
	mtc2    $8,	$1			# set DATA MMU ON
	lb	$2,	0($4)
	beq	$2,	$0,	5f
	mtc2    $9,	$1			# set DATA MMU OFF
	j	3b
	addiu	$4,	$4,	1
	
5:
	subu	$3,	$4,	$3
	sw	$3,	0($5)
	jr	$31
	or	$2,	$0,	$0
	.end cpu_uspace_strlen
	
	.ent cpu_uspace_error
cpu_uspace_error:
	jr	$31
	addiu	$2,	$0,	EFAULT
	.end cpu_uspace_error
#-------------------------------------------------------
