/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <machine/asm.h>
#if defined(SYSLIBC_SCCS) && !defined(lint)
	.asciz "@(#)brk.s	8.1 (Berkeley) 6/4/93"
#endif /* SYSLIBC_SCCS and not lint */

#include "SYS.h"

#define	SYS_brk		SYS_break

	.globl	curbrk
	.globl	minbrk
ENTRY(_brk)
	jmp	    ok

ENTRY(brk)
#ifdef PIC
	movl	4(%esp),%eax
	PIC_PROLOGUE
	movl	PIC_GOT(minbrk),%edx
	PIC_EPILOGUE
	cmpl	%eax,(%edx)
	jl		ok
	movl	(%edx),%eax
	movl	%eax,4(%esp)
ok:
	SYSTRAP(brk)
	jb		err
	PIC_PROLOGUE
	movl	PIC_GOT(curbrk),%edx
	PIC_EPILOGUE
	movl	%eax,(%edx)
	movl	$0,%eax
	ret
err:
	PIC_PROLOGUE
	mov	    PIC_GOT(CERROR),%edx
	PIC_EPILOGUE
	jmp	    *%edx
#else
	movl	4(%esp),%eax
	cmpl	%eax,minbrk
	jl		ok
	movl	minbrk,%eax
	movl	%eax,4(%esp)
ok:
	SYSTRAP(brk)
	jb		err
	movl	4(%esp),%eax
	movl	%eax,curbrk
	movl	$0,%eax
	ret
err:
	jmp		CERROR
#endif
