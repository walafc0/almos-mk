/*
 * __cpu-context.S - low-level context operations
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

#define KSP		10*4
#define TID		9*4
#define TSS		8*4
#define LOADABLE	7*4
#define CR3		6*4
#define EIP		5*4
#define ESI		4*4
#define EDI		3*4
#define EBP		2*4
#define ESP		1*4
#define EBX		0*4

#define ESP_R0		1*4
#define ESP_R1		3*4

.text
.global cpu_context_save
cpu_context_save:
	movl	0(%esp),	%edx		/* Return Address */
	movl	4(%esp),	%ecx       	/* ctx pointer */
	movl	%ebx,		(EBX)(%ecx)  	        
	movl	%esp,		(ESP)(%ecx)  	
	movl	%ebp,		(EBP)(%ecx)  	
	movl	%edi,		(EDI)(%ecx)
  	movl	%esi,		(ESI)(%ecx)  	
	movl	%edx,		(EIP)(%ecx)
/*  	movl    %cr3,		(CR3)(%ecx)	*/
	movl	$0x0,		%eax
	ret

.global	cpu_context_restore
.extern cpu_context_load
cpu_context_restore:
	movl	4(%esp),		%ecx 		/* ctx pointer */
	movl	(LOADABLE)(%ecx),	%eax
	test    $0x1,			%eax
	jz	restore
	movl	$0x0,			(LOADABLE)(%ecx)
	push	%ecx
	call	cpu_context_load
restore:
	movl	8(%esp),		%eax 		/* Return Value */
	movl	(TSS)(%ecx),		%ebx		
	movl	(TID)(%ecx),		%edx
	movl	%edx,		(ESP_R1)(%ebx)
	movl	(KSP)(%ecx),		%edx
	movl	%edx,		(ESP_R0)(%ebx)
	movl	(EBX)(%ecx),		%ebx
	movl	(ESP)(%ecx),		%esp
	movl	(EBP)(%ecx),		%ebp
	movl	(EDI)(%ecx),		%edi
	movl	(ESI)(%ecx),		%esi
	movl	(EIP)(%ecx),		%edx
/*	movl	(CR3)(%ecx),		%cr3	*/
	movl	%edx,			0(%esp)
	ret