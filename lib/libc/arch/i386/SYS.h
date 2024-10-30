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
 *
 *	@(#)SYS.h	8.1 (Berkeley) 6/4/93
 */

#include <machine/asm.h>
#include <sys/syscall.h>

#ifdef __STDC__
#define _SYSNAM(x)	SYS_ ## x
#define _CALLNAM(x)	call _## x
#define _GLOBL(x)   .globl _## x
#else
#define	_SYSNAM(x)	SYS_/**/x
#define _CALLNAM(x)	call _/**/x
#define _GLOBL(x)   .globl _/**/x
#endif

#ifdef __ELF__
#define ALIGNTEXT .align 4
#else
#define ALIGNTEXT .align 2
#endif

#define SYSTRAP(x)			    \
	movl _SYSNAM(x),%eax		;\
	int $0x80

#ifdef ENTRY
#undef ENTRY
#define	ENTRY(x)	            \
    _GLOBL(x)                   ;\
    .data                       ;\
1:  .long   0                   ;\
    .text                       ;\
    ALIGNTEXT                   ;\
    _C_LABEL(x):                \
    movl    $1b, %eax           ;\
    _PROF_PROLOGUE
#endif

#ifdef PIC
#define SYSCALL_ERR             \
    PIC_PROLOGUE                ;\
    mov PIC_GOT(cerror), %eax   ;\
    PIC_EPILOGUE                ;\
    jmp     *%eax
#else
#define SYSCALL_ERR             \
    jmp		cerror
#endif

#define SYSCALL(x)              \
2:  SYSCALL_ERR                 ;\
   	ENTRY(x)					;\
	lea		_SYSNAM(x), %eax	;\
	LCALL(7, 0)					;\
	jb		2b

#define	RSYSCALL(x)				\
	SYSCALL(x)					;\
	ret

#define	PSEUDO(x,y)				\
	ENTRY(x)					;\
	lea 	_SYSNAM(y), %eax;   ;\
	LCALL(7,0)					;\
	ret

#define	CALL(x, y)				\
	_CALLNAM(y)					;\
	addl	$4*x, %esp

#define LCALL(x, y)				\
	.byte	0x9a				;\
	.long 	y					;\
	.word	x

#define	ASMSTR	.asciz

	.globl	cerror
