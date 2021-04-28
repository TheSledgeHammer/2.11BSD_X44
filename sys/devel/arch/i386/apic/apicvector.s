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
 
#include <devel/arch/i386/isa/icu.h>
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#define	IRQ_BIT(irq_num)	    	(1 << ((irq_num) % 8))
#define	IRQ_BYTE(irq_num)	    	((irq_num) >> 3)

#define INTR_ADDR(intr, irq_num) 	((intr)+(irq_num) * 4)

#define	MASK(irq_num, icu)                                          	\
		movb	_imen + IRQ_BYTE(irq_num),%al							;\
		orb		$IRQ_BIT(irq_num),%al									;\
		movb	%al,_imen + IRQ_BYTE(irq_num)							;\
		FASTER_NOP														;\
		outb	%al,$(icu+1)

#define	UNMASK(irq_num, icu)                                        	\
		cli																;\
		movb	_imen + IRQ_BYTE(irq_num),%al							;\
		andb	$~IRQ_BIT(irq_num),%al									;\
		movb	%al,_imen + IRQ_BYTE(irq_num)							;\
		FASTER_NOP														;\
		outb	%al,$(icu+1)											;\
		sti

#define APIC_STRAY(irq_num)                                         	\
IDTVEC(stray/**/irq_num)                                            	;\
		call	_isa_strayintr					                        ;\
		addl	$4,%esp						    	                    ;\
		jmp		5b		

#define APIC_HOLD(irq_num)                                          	\
IDTVEC(hold/**/irq_num)													;\
		orb	$IRQ_BIT(irq_num),_ipending + IRQ_BYTE(irq_num)				;\
		INTRFASTEXIT

#define APIC_RECURSE(num)                                     			\
IDTVEC(recurse_/**/num)													;\
		pushfl															;\
		pushl	%cs														;\
		pushl	%esi													;\
		subl	$4,%esp													;\
		pushl	$T_ASTFLT				/* trap # for doing ASTs */		;\
		INTRENTRY														;\

#define APIC_RESUME(irq_num)                                         	\
IDTVEC(resume_/**/num)													;\
		movl	$IREENT_MAGIC,TF_ERR(%esp)								;\
		movl	%ebx,%esi												;\
		movl	INTR_ADDR(intrsource, irq_num), %ebp					;\
		movl	IS_MAXLEVEL(%ebp),%ebx									;\
		jmp		1f														;\

#define	APICINTR(name, irq_num, icu, enable_icus)                        \
IDTVEC(name ## _intr/**/irq_num)                                        ;\
    	pushl	$0					    /* dummy error code */			;\
		pushl	$T_ASTFLT			    /* trap # for doing ASTs */		;\
		INTRENTRY													    ;\
    	MASK(irq_num, icu)				/* mask it in hardware */		;\
		enable_icus(irq_num)			/* and allow other intrs */		;\
    	testb	$IRQ_BIT(irq_num),_cpl + IRQ_BYTE(irq_num)				;\
		APIC_HOLD(irq_num)		        /* currently masked; hold it */	;\
    	APIC_RESUME(irq_num)                                            ;\
    	movl	_cpl,%eax				/* cpl to restore on exit */	;\
		pushl	%eax													;\
		orl		INTR_ADDR(intrmask, irq_num),%eax						;\
		movl	%eax,_cpl				/* add in this intr's mask */	;\
		sti								/* safe to take intrs now */	;\
		movl	INTR_ADDR(intrhand, irq_num),%ebx	/* head of chain */	;\
		testl	%ebx,%ebx												;\
    	jz      APIC_STRAY(irq_num)                                     ;\
    	APIC_STRAY_INITIALIZE											;\
7:		movl	IH_ARG(%ebx),%eax		/* get handler arg */			;\
		testl	%eax,%eax												;\
		jnz		4f														;\
		movl	%esp,%eax				/* 0 means frame pointer */		;\
4:		pushl	%eax													;\
		call	IH_FUN(%ebx)			/* call it */					;\
		addl	$4,%esp					/* toss the arg */				;\
		STRAY_INTEGRATE													;\
		incl	IH_COUNT(%ebx)			/* count the intrs */			;\
		movl	IH_NEXT(%ebx),%ebx		/* next handler in chain */		;\
		testl	%ebx,%ebx												;\
		jnz		7b														;\
		STRAY_TEST														;\
5:		UNMASK(irq_num, icu)			/* unmask it in hardware */		;\
		INTREXIT						/* lower spl and do ASTs */		;\

#define	APIC_STRAY_INITIALIZE 	                                        \
		xorl	%esi,%esi				/* nobody claimed it yet */
#define	APIC_STRAY_INTEGRATE 	                                        \
		orl		%eax,%esi				/* maybe he claimed it */	
#define	APIC_STRAY_TEST 			                                    \
		testl	%esi,%esi				/* no more handlers */			;\
		jz		APIC_STRAY(irq_num)		/* nobody claimed it */


APICINTR(apic, 0, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 1, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 2, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 3, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 4, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 5, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 6, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 7, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 8, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 9, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 10, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 11, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 12, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 13, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 14, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 15, IO_ICU2, ENABLE_ICU1_AND_2)

APICINTR(x2apic, 0, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 1, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 2, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 3, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 4, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 5, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 6, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 7, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 8, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 9, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 10, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 11, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 12, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 13, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 14, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 15, IO_ICU2, ENABLE_ICU1_AND_2)
