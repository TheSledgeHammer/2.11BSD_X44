/*-
 * Copyright (c) 2003 Peter Wemm <peter@FreeBSD.org>
 * All rights reserved.
 *
 * Copyright (c) 2020, 2021 The FreeBSD Foundation
 *
 * Portions of this software were developed by
 * Konstantin Belousov <kib@FreeBSD.org> under sponsorship from
 * the FreeBSD Foundation.
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

 		.globl	kernbase
		.set	kernbase, _C_LABEL(KERNBASE)
		.globl	kernload
		.set	kernload, _C_LABEL(KERNLOAD)

		.text
/**********************************************************************
 *
 * This is where the loader trampoline start us, set the ball rolling...
 *
 * We are called with the stack looking like this:
 * 0(%rsp) = 32 bit return address (cannot be used)
 * 4(%rsp) = 32 bit modulep
 * 8(%rsp) = 32 bit kernend
 *
 * We are already in long mode, on a 64 bit %cs and running at KERNBASE.
 */
ENTRY(start)

		/* Tell the bios to warmboot next time */
		movw	$0x1234,0x472

		/* Don't trust what the loader gives for rflags. */
		pushq	$PSL_MBO
		popfq

		/* Get onto a stack that we can trust - there is no going back now. */
		movq	%rsp, %rbp
		movq	$bootstack,%rsp

		/* Grab metadata pointers from the loader. */
		movl	4(%rbp),%edi			/* modulep (arg 1) */
		movl	8(%rbp),%esi			/* kernend (arg 2) */
		xorq	%rbp, %rbp

		call	_C_LABEL(initamd64)		/* set up cpu for unix operation */
		movq	%rax,%rsp				/* set up kstack for mi_startup() */
		call	_C_LABEL(main)			/* autoconfiguration, mountroot etc */
0:		hlt
		jmp		0b


		.bss
		ALIGN_DATA					/* just to be sure */
		.space	0x1000				/* space for bootstack - temporary stack */
bootstack:
