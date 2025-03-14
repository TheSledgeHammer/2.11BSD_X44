/*	$NetBSD: _getsetc.S,v 1.3.2.1 2004/11/12 06:48:24 jmc Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#undef GETC
#undef SETC

#define GETC \
	movw	%gs,  (UC_REGS + _REG_GS * 4)(%ecx)	; \
	movw	%fs,  (UC_REGS + _REG_FS * 4)(%ecx)	; \
	movl	%edi,  (UC_REGS + _REG_EDI * 4)(%ecx)	; \
	movl	%esi,  (UC_REGS + _REG_ESI * 4)(%ecx)	; \
	movl	%ebp,  (UC_REGS + _REG_EBP * 4)(%ecx)	; \
	movl	%ebx,  (UC_REGS + _REG_EBX * 4)(%ecx)	; \
	movl	%edx,  (UC_REGS + _REG_EIP * 4)(%ecx)	; \
	FPSAVE(%ecx)					; \
	movl	$(_UC_USER | _UC_CPU | _UC_FPU),%eax	; \
 	movl	%eax,   UC_FLAGS(%ecx)	

#define SETC \
	movl	UC_FLAGS(%ecx), %eax			; \
	btl	$_UC_USER_BIT, %eax			; \
	jnc	1f					; \
							; \
	FPLOAD(%ecx)					; \
	movw	(UC_REGS + _REG_GS * 4)(%ecx), %gs	; \
	movw	(UC_REGS + _REG_FS * 4)(%ecx), %fs	; \
	movl	(UC_REGS + _REG_EDI * 4)(%ecx), %edi	; \
	movl	(UC_REGS + _REG_ESI * 4)(%ecx), %esi	; \
	movl	(UC_REGS + _REG_EBP * 4)(%ecx), %ebp	; \
	movl	(UC_REGS + _REG_EBX * 4)(%ecx), %ebx	; \
	movl	(UC_REGS + _REG_UESP * 4)(%ecx), %edx	; \
	movl	(UC_REGS + _REG_EIP * 4)(%ecx), %eax	; \
	movl	%eax, -4(%edx)				; \
	leal	-4(%edx), %esp				; \
	ret						; \
							; \
1:	btl	$8, (UC_REGS + _REG_EFL * 4)(%ecx)	; \
	jnc	2f					; \
	pushl %ecx					; \
	call    *PIC_GOT(_C_LABEL(setcontext))		; \
2:	and 	$_UC_FPU, %eax				; \
	jz	3f					; \
	FPLOAD(%ecx)					; \
3:	movw	(UC_REGS + _REG_GS * 4)(%ecx), %gs	; \
	movw	(UC_REGS + _REG_FS * 4)(%ecx), %fs	; \
	movw	(UC_REGS + _REG_ES * 4)(%ecx), %es	; \
	movl	(UC_REGS + _REG_EDI * 4)(%ecx), %edi	; \
	movl	(UC_REGS + _REG_ESI * 4)(%ecx), %esi	; \
	movl	(UC_REGS + _REG_EBP * 4)(%ecx), %ebp	; \
	movl	(UC_REGS + _REG_EBX * 4)(%ecx), %ebx	; \
	movl	(UC_REGS + _REG_UESP * 4)(%ecx), %edx	; \
	movl	(UC_REGS + _REG_CS * 4)(%ecx), %eax	; \
	mov	%eax, -4(%edx)				; \
	movl	(UC_REGS + _REG_EIP * 4)(%ecx), %eax	; \
	movl	%eax, -8(%edx)				; \
	movl	(UC_REGS + _REG_DS * 4)(%ecx), %eax	; \
	movl	%eax, -12(%edx)				; \
	movl	(UC_REGS + _REG_EDX * 4)(%ecx), %eax	; \
	movl	%eax, -16(%edx)				; \
	movl	(UC_REGS + _REG_ECX * 4)(%ecx), %eax	; \
	movl	%eax, -20(%edx)				; \
	movl	(UC_REGS + _REG_EAX * 4)(%ecx), %eax	; \
	movl	%eax, -24(%edx)				; \
	movl	(UC_REGS + _REG_EFL * 4)(%ecx), %eax	; \
	movl	%eax, -28(%edx)				; \
	movw	(UC_REGS + _REG_SS * 4)(%ecx), %ss	; \
	leal	-28(%edx), %esp				; \
	popfl						; \
	popl	%eax					; \
	popl	%ecx					; \
	popl	%edx					; \
	popl	%ds					; \
	lret
