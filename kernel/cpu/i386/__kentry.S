/*
 * __kentry.S - unified kernel entry point
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

#define DECLARE_EXCEPT_ERR(n)	  \
.align 16			; \
	cli			; \
	pushl	$##n		; \
	jmp	unified_except_entry ; \
	movl	%eax,	%eax
	
#define DECLARE_EXCEPT(n)	\
.align 16			; \
	cli			; \
	pushl	$0x0		; \
	pushl	$##n		; \
	jmp	unified_except_entry

#define DECLARE_IRQ(n)		\
.align 16			; \
	cli			; \
	pushl	$0x0		; \
	pushl	$##n		; \
	jmp	unified_irq_entry
	
.text
.global __except
.align 16
__except:	
DECLARE_EXCEPT(0)		/* Division By Zero Exception 	*/
DECLARE_EXCEPT(1)		/* Debug Exception	 	*/
DECLARE_EXCEPT(2)		/* Non Maskable Interrupt Exception */
DECLARE_EXCEPT(3)		/* Breakpoint Exception		*/
DECLARE_EXCEPT(4)		/* Into Detected Overflow Exception */
DECLARE_EXCEPT(5)		/* Out of Bounds Exception */
DECLARE_EXCEPT(6)		/* Invalid Opcode Exception */
DECLARE_EXCEPT(7)		/* No Coprocessor Exception */
DECLARE_EXCEPT_ERR(8)		/* Double Fault Exception */
DECLARE_EXCEPT(9)		/* Coprocessor Segment Overrun Exception */
DECLARE_EXCEPT_ERR(10)		/* Bad TSS Exception */
DECLARE_EXCEPT_ERR(11)		/* Segment Not Present Exception */
DECLARE_EXCEPT_ERR(12)		/* Stack Fault Exception */
DECLARE_EXCEPT_ERR(13)		/* General Protection Fault Exception */
DECLARE_EXCEPT_ERR(14)		/* Page Fault Exception */
DECLARE_EXCEPT(15)		/* Unknown Interrupt Exception */
DECLARE_EXCEPT(16)		/* Coprocessor Fault Exception */
DECLARE_EXCEPT(17)		/* Alignment Check Exception (486+) */
DECLARE_EXCEPT(18)		/* Machine Check Exception (Pentium/586+) */
DECLARE_EXCEPT(19)		/* Reserved */
DECLARE_EXCEPT(20)		/* Reserved */
DECLARE_EXCEPT(21)		/* Reserved */
DECLARE_EXCEPT(22)		/* Reserved */
DECLARE_EXCEPT(23)		/* Reserved */
DECLARE_EXCEPT(24)		/* Reserved */
DECLARE_EXCEPT(25)		/* Reserved */
DECLARE_EXCEPT(26)		/* Reserved */
DECLARE_EXCEPT(27)		/* Reserved */
DECLARE_EXCEPT(28)		/* Reserved */
DECLARE_EXCEPT(29)		/* Reserved */
DECLARE_EXCEPT(30)		/* Reserved */
DECLARE_EXCEPT(31)		/* Reserved */

.text
.global __irq
.align 16
__irq:	
DECLARE_IRQ(0)
DECLARE_IRQ(1)
DECLARE_IRQ(2)
DECLARE_IRQ(3)
DECLARE_IRQ(4)
DECLARE_IRQ(5)
DECLARE_IRQ(6)
DECLARE_IRQ(7)
DECLARE_IRQ(8)
DECLARE_IRQ(9)
DECLARE_IRQ(10)
DECLARE_IRQ(11)
DECLARE_IRQ(12)
DECLARE_IRQ(13)
DECLARE_IRQ(14)
DECLARE_IRQ(15)
DECLARE_IRQ(16)
DECLARE_IRQ(17)
DECLARE_IRQ(18)
DECLARE_IRQ(19)
DECLARE_IRQ(20)
DECLARE_IRQ(21)
DECLARE_IRQ(22)
DECLARE_IRQ(23)
DECLARE_IRQ(24)
DECLARE_IRQ(25)
DECLARE_IRQ(26)
DECLARE_IRQ(27)
DECLARE_IRQ(28)
DECLARE_IRQ(29)
DECLARE_IRQ(30)
DECLARE_IRQ(31)
DECLARE_IRQ(32)
DECLARE_IRQ(33)
DECLARE_IRQ(34)
DECLARE_IRQ(35)
DECLARE_IRQ(36)
DECLARE_IRQ(37)
DECLARE_IRQ(38)
DECLARE_IRQ(39)
DECLARE_IRQ(40)
DECLARE_IRQ(41)
DECLARE_IRQ(42)
DECLARE_IRQ(43)
DECLARE_IRQ(44)
DECLARE_IRQ(45)
DECLARE_IRQ(46)
DECLARE_IRQ(47)
DECLARE_IRQ(48)
DECLARE_IRQ(49)
DECLARE_IRQ(50)
DECLARE_IRQ(51)
DECLARE_IRQ(52)
DECLARE_IRQ(53)
DECLARE_IRQ(54)
DECLARE_IRQ(55)
DECLARE_IRQ(56)
DECLARE_IRQ(57)
DECLARE_IRQ(58)
DECLARE_IRQ(59)
DECLARE_IRQ(60)
DECLARE_IRQ(61)
DECLARE_IRQ(62)
DECLARE_IRQ(63)
DECLARE_IRQ(64) /* End of Available IRQ in this version, this is also the syscall gate (INT 96).
	         * this value is used by PIC_IRQ_MAX macor */

#define SAVE_REGS         \
	pusha		; \
	push 	%ds	; \
	push 	%es	; \
	push 	%fs	; \
	push 	%gs	; \

#define RESTORE_REGS	  \
	pop 	%gs	; \
	pop 	%fs	; \
	pop 	%es	; \
	pop 	%ds	; \
	popa

#define LOAD_KERNEL_SEGMENT	  \
	mov 	$0x10,	%ax 	; \
	mov 	%ax,	%ds	; \
	mov 	%ax,	%es	; \
	mov 	%ax,	%fs	; \
	mov 	%ax,	%gs	; \
	
.extern __do_exception
.extern __do_interrupt
	
unified_except_entry:
	SAVE_REGS
	LOAD_KERNEL_SEGMENT
	mov 	%esp,	%eax
	push 	%eax
	call 	__do_exception
	pop 	%eax
	RESTORE_REGS
	add 	$0x8,	%esp
	iret

unified_irq_entry:	
	SAVE_REGS
	LOAD_KERNEL_SEGMENT	
	mov 	%esp,	%eax
	push 	%eax
	call 	__do_interrupt
	pop 	%eax
	RESTORE_REGS
	add 	$0x8,	%esp
	iret
	