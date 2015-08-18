# This file is part of MutekP.

# MutekP is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# MutekP is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with MutekP; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

# UPMC / LIP6 / SOC (c) 2007
# Copyright Ghassan Almaless <ghassan.almaless@gmail.com>
# Copyright Franck Wajsbürt <franck.wajsburt@lip6.fr>



#-------------------------------------------------------
# __setjmp:
# save of current function context.
# arguments:
# $4: struct __jmp_buffer_s *buff
#-------------------------------------------------------
        .section .text,"ax", @progbits
        .align 2
        .globl __setjmp
        .ent __setjmp

__setjmp:
        sw $16, 0*4($4)
        sw $17, 1*4($4)
        sw $18, 2*4($4)
        sw $19, 3*4($4)
        sw $20, 4*4($4)
        sw $21, 5*4($4)
        sw $22, 6*4($4)
        sw $23, 7*4($4)
        sw $28, 8*4($4)
        sw $29, 9*4($4)
        sw $30, 10*4($4)
        sw $31, 11*4($4)
        xor $2, $2, $2
        jr $31
        .end __setjmp

#-------------------------------------------------------
# __longjmp
# resume previously saved context
# arguments:
# $4: struct __jmp_buffer_s *buff
# $5: val
#-------------------------------------------------------
        .section .text,"ax", @progbits
        .align 2

        .globl __longjmp
        .ent __longjmp

__longjmp:
        .set noreorder
        lw $16, 0*4($4)
        lw $17, 1*4($4)
        lw $18, 2*4($4)
        lw $19, 3*4($4)
        lw $20, 4*4($4)
        lw $21, 5*4($4)
        lw $22, 6*4($4)
        lw $23, 7*4($4)
        lw $28, 8*4($4)
        lw $29, 9*4($4) # get stack ptr
        lw $31, 11*4($4)
        bnez $5, __jmp_next
        or $2, $0, $5
        ori $2, $0, 1
__jmp_next:
        jr $31
        lw $30, 10*4($4)
        .set reorder
        .end __longjmp

#-------------------------------------------------------
