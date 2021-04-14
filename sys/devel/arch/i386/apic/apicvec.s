/* $OpenBSD: apicvec.s,v 1.35 2018/06/18 23:15:05 bluhm Exp $ */
/* $NetBSD: apicvec.s,v 1.1.2.2 2000/02/21 21:54:01 sommerfeld Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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

#include "ioapic.h"

#include <devel/arch/i386/apic/apicreg.h>
#include <devel/arch/i386/apic/apicvar.h>

#define CPL lapic_tpr

	.globl  apic_stray
#ifdef SMP

#endif

#if NIOAPIC > 0

#define voidop(num)

	/*
	 * I/O APIC interrupt.
	 * We sort out which one is which based on the value of
	 * the processor priority register.
	 *
	 * XXX use cmove when appropriate.
	 */

#define APICINTR(name, num, early_ack, late_ack, mask, unmask, level_mask) 	\
IDTVEC(intr_##name##num)													\
		subl	$8,%esp				/* space for tf_{err,trapno} */			;\
		INTRENTRY(intr_##name##num)											;\
		pushl	lapic_tpr													;\
		movl	lapic_ppr,%eax												;\
		orl		$num,%eax													;\
		movl	apic_maxlevel(,%eax,4),%ebx									;\
		movl	%ebx,CPL													;\
		mask(num)					/* mask it in hardware */				;\
		early_ack(num)				/* and allow other intrs */				;\
		incl	uvmexp+V_INTR		/* statistical info */					;\
		sti																	;\
		movl	apic_intrhand(,%eax,4),%ebx /* chain head */				;\
		testl	%ebx,%ebx													;\
		jz      Xstray_##name##num											;\
		APIC_STRAY_INIT				/* nobody claimed it yet */				;\
7:		incl	CPUVAR(IDEPTH)												;\
		movl	%esp, %eax			/* save frame pointer in eax */			;\
		pushl	%ebx				/* arg 2: ih structure */				;\
		pushl	%eax				/* arg 1: frame pointer */				;\
		call	intr_handler		/* call it */							;\
		addl	$8, %esp			/* toss args */							;\
		APIC_STRAY_INTEGRATE		/* maybe he claimed it */				;\
		orl		%eax,%eax			/* should it be counted? */				;\
		jz		4f															;\
		addl	$1,IH_COUNT(%ebx)	/* count the intrs */					;\
		adcl	$0,IH_COUNT+4(%ebx)											;\
		cmpl	$0,intr_shared_edge											;\
		jne		4f					/* if no shared edges ... */			;\
		orl		%eax,%eax			/* ... 1 means stop trying */			;\
		js		4f															;\
1:		decl	CPUVAR(IDEPTH)												;\
		jmp		8f															;\
4:		decl	CPUVAR(IDEPTH)												;\
		movl	IH_NEXT(%ebx),%ebx	/* next handler in chain */				;\
		testl	%ebx,%ebx													;\
		jnz		7b															;\
		APIC_STRAY_TEST(name,num)	/* see if it's a stray */				;\
8:									 										\
		unmask(num)					/* unmask it in hardware */				;\
		late_ack(num)														;\
		jmp		Xdoreti														;\
Xstray_##name##num:					 										\
		pushl	$num														;\
		call	apic_stray													;\
		addl	$4,%esp														;\
		jmp		8b															;\
		
#if defined(DEBUG)
#define APIC_STRAY_INIT 													\
		xorl	%esi,%esi
#define	APIC_STRAY_INTEGRATE 												\
		orl		%eax,%esi
#define APIC_STRAY_TEST(name,num) 											\
		testl 	%esi,%esi													;\
		jz 		Xstray_##name##num
#else /* !DEBUG */
#define APIC_STRAY_INIT
#define APIC_STRAY_INTEGRATE
#define APIC_STRAY_TEST(name,num)
#endif /* DEBUG */

APICINTR(ioapic,0, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,1, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,2, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,3, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,4, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,5, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,6, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,7, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,8, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,9, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,10, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,11, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,12, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,13, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,14, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(ioapic,15, voidop, ioapic_asm_ack, voidop, voidop, voidop)

	.globl	Xintr_ioapic0,Xintr_ioapic1
	.globl	Xintr_ioapic2,Xintr_ioapic3
	.globl	Xintr_ioapic4,Xintr_ioapic5
	.globl	Xintr_ioapic6,Xintr_ioapic7
	.globl	Xintr_ioapic8,Xintr_ioapic9
	.globl	Xintr_ioapic10,Xintr_ioapic11
	.globl	Xintr_ioapic12,Xintr_ioapic13
	.globl	Xintr_ioapic14,Xintr_ioapic15
	.globl apichandler

apichandler:
	.long	Xintr_ioapic0,Xintr_ioapic1
	.long	Xintr_ioapic2,Xintr_ioapic3
	.long	Xintr_ioapic4,Xintr_ioapic5
	.long	Xintr_ioapic6,Xintr_ioapic7
	.long	Xintr_ioapic8,Xintr_ioapic9
	.long	Xintr_ioapic10,Xintr_ioapic11
	.long	Xintr_ioapic12,Xintr_ioapic13
	.long	Xintr_ioapic14,Xintr_ioapic15

#endif
