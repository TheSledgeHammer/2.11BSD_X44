/*	$NetBSD: bioscall.s,v 1.2 1997/10/19 22:59:50 jtk Exp $ */
/*
 *  Copyright (c) 1997 John T. Kohl
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include <machine/param.h>
#include <machine/bioscall.h>

#include <machine/asm.h>

		.globl	_PTDpaddr	/* from locore.s */
	
_biostramp_image:
		.globl	_biostramp_image

8:
#include "i386/bioscall/biostramp.inc"
9:

_biostramp_image_size:
		.globl	_biostramp_image_size
		.long	9b - 8b

/*
 * void bioscall(int function, struct bioscallregs *regs):
 * 	call the BIOS interrupt "function" from real mode with
 *	registers as specified in "regs"
 *	for the flags, though, only these flags are passed to the BIOS--
 *	the remainder come from the flags register at the time of the call:
 *	(PSL_C|PSL_PF|PSL_AF|PSL_Z|PSL_N|PSL_D|PSL_V)
 *
 *	Fills in *regs with registers as returned by BIOS.
 */
ENTRY(bioscall)
		pushl	%ebp
		movl	%esp,%ebp				/* set up frame ptr */

		movl	%cr3,%eax				/* save PTDB register */
		pushl	%eax

		movl	_PTDpaddr,%eax			/* install proc0 PTD */
		movl	%eax,%cr3
	
		movl  	$(BIOSTRAMP_BASE),%eax	/* address of trampoline area */
		pushl 	12(%ebp)
		pushl 	8(%ebp)
		call  	%eax					/* machdep.c initializes it */
		addl  	$8,%esp					/* clear args from stack */

		popl 	%eax
		movl 	%eax,%cr3				/* restore PTDB register */
		
		leave
		ret

/*-
 * Copyright (c) 1997 Jonathan Lemon
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/*
 * Functions for calling x86 BIOS functions from the BSD kernel
 */


		.data
		ALIGN_DATA
bioscall_frame:		.long	0
bioscall_stack:		.long	0

		.text
/*
 * bios32(regs, offset, segment)
 *	struct bios_regs *regs;
 *	u_int offset;
 * 	u_short segment;
 */
ENTRY(bios32)
		pushl	%ebp
		movl	16(%esp),%ebp
		mov		%bp,bioscall_vector+4
		movl	12(%esp),%ebp
		movl	%ebp,bioscall_vector
		movl	8(%esp),%ebp
		pushl	%ebx
		pushl	%esi
		pushl	%edi
		movl	0(%ebp),%eax
		movl	4(%ebp),%ebx
		movl	8(%ebp),%ecx
		movl	12(%ebp),%edx
		movl	16(%ebp),%esi
		movl	20(%ebp),%edi
		pushl	%ebp
		lcall	*bioscall_vector
		popl	%ebp
		movl	%eax,0(%ebp)
		movl	%ebx,4(%ebp)
		movl	%ecx,8(%ebp)
		movl	%edx,12(%ebp)
		movl	%esi,16(%ebp)
		movl	%edi,20(%ebp)
		movl	$0,%eax			/* presume success */
		jnc		1f
		movl	$1,%eax			/* nope */
1:
		popl	%edi
		popl	%esi
		popl	%ebx
		popl	%ebp
		ret


/*
 * bios16_call(regs, stack)
 *	struct bios_regs *regs;
 *	char *stack;
 */
ENTRY(bios16_call)
		pushl	%ebp
		movl	%esp,%ebp
		addl	$4,%ebp				/* frame pointer */
		movl	%ebp,bioscall_frame	/* ... save it */
		pushl	%ebx
		pushl	%esi
		pushl	%edi
/*
 * the problem with a full 32-bit stack segment is that 16-bit code
 * tends to do a pushf, which only pushes %sp, not %esp.  This value
 * is then popped off (into %esp) which causes a page fault because
 * it is the wrong address.
 *
 * the reverse problem happens for 16-bit stack addresses; the kernel
 * code attempts to get the address of something on the stack, and the
 * value returned is the address relative to %ss, not %ds.
 *
 * we fix this by installing a temporary stack at page 0, so the
 * addresses are always valid in both 32 bit and 16 bit modes.
 */
		movl	%esp,bioscall_stack	/* save current stack location */
		movl	8(%ebp),%esp		/* switch to page 0 stack */

		movl	4(%ebp),%ebp		/* regs */

		movl	0(%ebp),%eax
		movl	4(%ebp),%ebx
		movl	8(%ebp),%ecx
		movl	12(%ebp),%edx
		movl	16(%ebp),%esi
		movl	20(%ebp),%edi

		pushl	$GSEL(GBIOSCODE32_SEL, SEL_KPL)
		leal	bios16_jmp,%ebp
		andl	$PAGE_MASK,%ebp
		pushl	%ebp				/* reload %cs and */
		lret						/* ...continue below */


ENTRY(bios16_jmp):
		lcallw	*bioscall_vector	/* 16-bit call */

		jc	1f
		pushl	$0					/* success */
		jmp	2f
1:
		pushl	$1					/* failure */
2:
		movl	bioscall_frame,%ebp

		movl	4(%ebp),%ebp		/* regs */

		movl	%eax,0(%ebp)
		movl	%ebx,4(%ebp)
		movl	%ecx,8(%ebp)
		movl	%edx,12(%ebp)
		movl	%esi,16(%ebp)
		movl	%edi,20(%ebp)

		popl	%eax				/* recover return value */
		movl	bioscall_stack,%esp	/* return to normal stack */

		popl	%edi
		popl	%esi
		popl	%ebx
		popl	%ebp

		movl	(%esp),%ecx
		pushl	%ecx				/* return address */
		movl	$GSEL(GCODE_SEL, SEL_KPL),4(%esp)
		lret						/* reload %cs on the way out */
