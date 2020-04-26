/*	$NetBSD: locore.s,v 1.172.2.7 1998/05/08 10:07:01 mycroft Exp $	*/

/*-
 * Copyright (c) 1993, 1994, 1995, 1997
 *	Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)locore.s	7.3 (Berkeley) 5/13/91
 */


/*
 * Trap and fault vector routines
 */

 // New syscalls??

#define	INTRENTRY \
	pushl	%eax		; \
	pushl	%ecx		; \
	pushl	%edx		; \
	pushl	%ebx		; \
	pushl	%ebp		; \
	pushl	%esi		; \
	pushl	%edi		; \
	pushl	%ds			; \
	pushl	%es			; \
	movl	$GSEL(GDATA_SEL, SEL_KPL),%eax	; \
	movl	%ax,%ds		; \
	movl	%ax,%es
#define	INTRFASTEXIT \
	popl	%es			; \
	popl	%ds			; \
	popl	%edi		; \
	popl	%esi		; \
	popl	%ebp		; \
	popl	%ebx		; \
	popl	%edx		; \
	popl	%ecx		; \
	popl	%eax		; \
	addl	$8,%esp		; \
	iret


IDTVEC(exceptions)
		.long	_Xtrap00, _Xtrap01, _Xtrap02, _Xtrap03
		.long	_Xtrap04, _Xtrap05, _Xtrap06, _Xtrap07
		.long	_Xtrap08, _Xtrap09, _Xtrap0a, _Xtrap0b
		.long	_Xtrap0c, _Xtrap0d, _Xtrap0e, _Xtrap0f
		.long	_Xtrap10, _Xtrap11, _Xtrap12, _Xtrap13
		.long	_Xtrap14, _Xtrap15, _Xtrap16, _Xtrap17
		.long	_Xtrap18, _Xtrap19, _Xtrap1a, _Xtrap1b
		.long	_Xtrap1c, _Xtrap1d, _Xtrap1e, _Xtrap1f

calltrap:

#ifdef VM86
		jnz		5f
		testl	$PSL_VM,TF_EFLAGS(%esp)
#endif

/*
 * Old call gate entry for syscall
 */
IDTVEC(osyscall)
		/* Set eflags in trap frame. */
		pushfl
		popl	8(%esp)
		/* Turn off trace flag and nested task. */
		pushfl
		andb	$~((PSL_T|PSL_NT)>>8),1(%esp)
		popfl
		pushl	$7						# size of instruction for restart
		jmp		syscall1
