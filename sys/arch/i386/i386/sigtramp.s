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
 *	from: @(#)locore.s	7.3 (Berkeley) 5/13/91
 *
 *	originally from: locore.s, by William F. Jolitz
 */

#include <sys/syscall.h>
#include <machine/asmacros.h>
#include <machine/psl.h>

#include "assym.s"


#define	LCALL(x,y)	.byte 0x9a ; .long y; .word x

/*
 * Signal trampoline, copied to top of user stack
 */

/*
 * Icode is copied out to process 1 to exec /etc/init.
 * If the exec fails, process 1 exits.
 */
		 ALIGN_TEXT
_icode:
		# pushl	$argv-_icode	# gas fucks up again
		movl	$argv,%eax
		subl	$_icode,%eax
		pushl	%eax

		# pushl	$init-_icode
		movl	$init,%eax
		subl	$_icode,%eax
		pushl	%eax
		pushl	%eax	# dummy out rta

		movl	%esp,%ebp
		movl	$exec,%eax
		LCALL(0x7,0x0)
		pushl	%eax
		movl	$exit,%eax
		pushl	%eax	# dummy out rta
		LCALL(0x7,0x0)

init:
		.asciz	"/sbin/init"
		.align	2
argv:
		.long	init+6-_icode		# argv[0] = "init" ("/sbin/init" + 6)
		.long	eicode-_icode		# argv[1] follows icode after copyout
		.long	0
eicode:
		.data
		.globl	szsigcode
_szicode:
		.long	_szicode-_icode

		.globl	_sigcode,_szsigcode
_sigcode:
		movl	12(%esp),%eax	# unsure if call will dec stack 1st
		call	%eax
		xorl	%eax,%eax		# smaller movl $103,%eax
		movb	$103,%al		# sigreturn()
		LCALL(0x7,0)			# enter kernel with args on stack
		hlt						# never gets here

_szsigcode:
		.long	_szsigcode-_sigcode
