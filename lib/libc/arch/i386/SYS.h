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
#define _GLOBL(x)   	.globl _## x
#else
#define	_SYSNAM(x)	SYS_/**/x
#define _CALLNAM(x)	call _/**/x
#define _GLOBL(x)   	.globl _/**/x
#endif

#ifdef __ELF__
#define CERROR		_C_LABEL(cerror)
#else
#define CERROR		_ASM_LABEL(cerror)
#endif

#define ALIGNTEXT _ALIGN_TEXT

#define	CALL(x,y)			        \
	    _CALLNAM(y)			        ;\
	    addl	$4*x, %esp

#define LCALL(x,y)			        \
	    .byte	0x9a			    ;\
	    .long 	y			        ;\
	    .word	x

#define DO_SYSTRAP(x)			    \
    	movl	_SYSNAM(x), %eax	;\
        int     $0x80

#define SYSTRAP(x)	                \
        DO_SYSTRAP(x)

#define DO_SYSCALL_NOERROR(x,y)     \
    	ENTRY(x)                    ;\
    	SYSTRAP(y)			

#ifdef PIC
#define DO_SYSCALL_ERROR            \
    	PIC_PROLOGUE                ;\
    	mov PIC_GOT(CERROR), %ecx   ;\
    	PIC_EPILOGUE                ;\
    	jmp     *%ecx
#else
#define DO_SYSCALL_ERROR            \
    	jmp	CERROR
#endif

#define DO_SYSCALL(x,y)           	\
    	.text                       ;\
    	ALIGNTEXT                   ;\
2:  	DO_SYSCALL_ERROR            ;\
    	DO_SYSCALL_NOERROR(x,y)     ;\
    	jb	    2b         

#define SYSCALL_NOERROR(x)		    \
	    DO_SYSCALL_NOERROR(x,x)

#define SYSCALL(x)			        \
    	DO_SYSCALL(x,x)

#define SYSCALL2(x,y)			    \
    	DO_SYSCALL(x,y)

#define RSYSCALL_NOERROR(x)		    \
    	SYSCALL_NOERROR(x)          ;\
    	ret

#define	RSYSCALL(x)			        \
	    SYSCALL(x)  			    ;\
	    ret

#define PSEUDO_NOERROR(x,y)		    \
	    DO_SYSCALL_NOERROR(x,y)     ;\
	    ret

#define	PSEUDO(x,y)			        \
	    SYSCALL2(x,y)			    ;\
	    ret

#ifdef WEAK_ALIAS
#define	WSYSCALL(weak,strong)		\
	WEAK_ALIAS(weak,strong)         ;\
	PSEUDO(strong,weak)
#else
#define	WSYSCALL(weak,strong)		\
	PSEUDO(weak,weak)
#endif

    	.globl	 CERROR
