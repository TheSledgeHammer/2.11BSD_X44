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

#include <machine/asm.h>
#include "assym.s"

#include <machine/psl.h>
#include <machine/asmacros.h>
#include <machine/trap.h>


ENTRY(start_exceptions)

/*
 * Trap and fault vector routines
 */
#define	TRAP(a)		pushl $(a) ; jmp alltraps
#ifdef KGDB
#define	BPTTRAP(a)	pushl $(a) ; jmp bpttraps
#else
#define	BPTTRAP(a)	TRAP(a)
#endif

IDTVEC(div)
		pushl $0; TRAP(T_DIVIDE)
IDTVEC(dbg)
		pushl $0; BPTTRAP(T_TRCTRAP)
IDTVEC(nmi)
		pushl $0; TRAP(T_NMI)
IDTVEC(bpt)
		pushl $0; BPTTRAP(T_BPTFLT)
IDTVEC(ofl)
		pushl $0; TRAP(T_OFLOW)
IDTVEC(bnd)
		pushl $0; TRAP(T_BOUND)
IDTVEC(ill)
		pushl $0; TRAP(T_PRIVINFLT)
IDTVEC(dna)
		pushl $0; TRAP(T_DNA)
IDTVEC(dble)
		TRAP(T_DOUBLEFLT)
		/*PANIC("Double Fault");*/
IDTVEC(fpusegm)
		pushl $0; TRAP(T_FPOPFLT)
IDTVEC(tss)
		TRAP(T_TSSFLT)
		/*PANIC("TSS not valid");*/
IDTVEC(missing)
		TRAP(T_SEGNPFLT)
IDTVEC(stk)
		TRAP(T_STKFLT)
IDTVEC(prot)
		TRAP(T_PROTFLT)
IDTVEC(page)
		TRAP(T_PAGEFLT)
IDTVEC(rsvd)
		pushl $0; TRAP(T_RESERVED)
IDTVEC(fpu)
#ifdef NPX
		/*
		 * Handle like an interrupt so that we can call npxintr to clear the
		 * error.  It would be better to handle npx interrupts as traps but
		 * this is difficult for nested interrupts.
		 */
		pushl	$0		/* dummy error code */
		pushl	$T_ASTFLT
		pushal
		nop				/* silly, the bug is for popal and it only
						 * bites when the next instruction has a
						 * complicated address mode */
		pushl	%ds
		pushl	%es		/* now the stack frame is a trap frame */
		movl	$KDSEL,%eax
		movl	%ax,%ds
		movl	%ax,%es
		pushl	_cpl
		pushl	$0		/* dummy unit to finish building intr frame */
		incl	_cnt+V_TRAP
		call	_npxintr
		jmp		doreti
#else
		pushl $0; TRAP(T_ARITHTRAP)
#endif
	/* 17 - 31 reserved for future exp */
IDTVEC(rsvd0)
		pushl $0; TRAP(17)
IDTVEC(rsvd1)
		pushl $0; TRAP(18)
IDTVEC(rsvd2)
		pushl $0; TRAP(19)
IDTVEC(rsvd3)
		pushl $0; TRAP(20)
IDTVEC(rsvd4)
		pushl $0; TRAP(21)
IDTVEC(rsvd5)
		pushl $0; TRAP(22)
IDTVEC(rsvd6)
		pushl $0; TRAP(23)
IDTVEC(rsvd7)
		pushl $0; TRAP(24)
IDTVEC(rsvd8)
		pushl $0; TRAP(25)
IDTVEC(rsvd9)
		pushl $0; TRAP(26)
IDTVEC(rsvd10)
		pushl $0; TRAP(27)
IDTVEC(rsvd11)
		pushl $0; TRAP(28)
IDTVEC(rsvd12)
		pushl $0; TRAP(29)
IDTVEC(rsvd13)
		pushl $0; TRAP(30)
IDTVEC(rsvd14)
		pushl $0; TRAP(31)

		ALIGN32
alltraps:
		pushal
		nop
		push 	%ds
		push 	%es
		# movw	$KDSEL,%ax
		movw	$0x10,%ax
		movw	%ax,%ds
		movw	%ax,%es
calltrap:
		incl	_cnt+V_TRAP
		call	_trap
		/*
		 * Return through doreti to handle ASTs.  Have to change trap frame
		 * to interrupt frame.
		 */
		movl	$T_ASTFLT,4+4+32(%esp)	/* new trap type (err code not used) */
		pushl	_cpl
		pushl	$0						/* dummy unit */
		jmp		doreti

#ifdef KGDB
	/*
	 * This code checks for a kgdb trap, then falls through
	 * to the regular trap code.
	 */
		ALIGN32
bpttraps:
		pushal
		nop
		push	%es
		push	%ds
		# movw	$KDSEL,%ax
		movw	$0x10,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movzwl	52(%esp),%eax
		test	$3,%eax
		jne		calltrap
		call	_kgdb_trap_glue
		jmp		calltrap
#endif

/*
 * Call gate entry for syscall
 */

		ALIGN32
IDTVEC(syscall)
		pushfl	# only for stupid carry bit and more stupid wait3 cc kludge
		pushal	# only need eax,ecx,edx - trap resaves others
		nop
		movl	$KDSEL,%eax		# switch to kernel segments
		movl	%ax,%ds
		movl	%ax,%es
		incl	_cnt+V_SYSCALL  # kml 3/25/93
		call	_syscall
		/*
		 * Return through doreti to handle ASTs.  Have to change syscall frame
		 * to interrupt frame.
		 *
		 * XXX - we should have set up the frame earlier to avoid the
		 * following popal/pushal (not much can be done to avoid shuffling
		 * the flags).  Consistent frames would simplify things all over.
		 */
		movl	32+0(%esp),%eax	/* old flags, shuffle to above cs:eip */
		movl	32+4(%esp),%ebx	/* `int' frame should have been ef, eip, cs */
		movl	32+8(%esp),%ecx
		movl	%ebx,32+0(%esp)
		movl	%ecx,32+4(%esp)
		movl	%eax,32+8(%esp)
		popal
		nop
		pushl	$0				/* dummy error code */
		pushl	$T_ASTFLT
		pushal
		nop
		movl	__udatasel,%eax	/* switch back to user segments */
		push	%eax			/* XXX - better to preserve originals? */
		push	%eax
		pushl	_cpl
		pushl	$0
		jmp		doreti
