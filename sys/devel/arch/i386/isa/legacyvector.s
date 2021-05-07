/*-
 * Copyright (C) 1989, 1990 W. Jolitz
 * Copyright (c) 1992, 1993
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
 *	$Id: vector.s,v 1.18.2.1 1994/10/11 09:46:40 mycroft Exp $
 */

#include "assym.h"

#include <i386/isa/icu.h>
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <devel/i386/apic/apicvector.s.h>

#define ICU_HARDWARE_MASK

/*
 * These macros are fairly self explanatory.  If SPECIAL_MASK_MODE is defined,
 * we try to take advantage of the ICU's `special mask mode' by only EOIing
 * the interrupts on return.  This avoids the requirement of masking and
 * unmasking.  We can't do this without special mask mode, because the ICU
 * would also hold interrupts that it thinks are of lower priority.
 *
 * Many machines do not support special mask mode, so by default we don't try
 * to use it.
 */
 
#define	IRQ_BIT(irq_num)	(1 << ((irq_num) % 8))
#define	IRQ_BYTE(irq_num)	((irq_num) >> 3)

#ifdef ICU_SPECIAL_MASK_MODE

#define	ENABLE_ICU1(irq_num)
#define	ENABLE_ICU1_AND_2(irq_num) 											 \
		movb	$(0x60|2),%al					/* specific EOI for IRQ2 */	;\
		outb	%al,$IO_ICU1
#define	LEGACY_MASK(irq_num, icu)
#define	LEGACY_UNMASK(irq_num, icu) 										 \
		movb	$(0x60|(irq_num%8)),%al			/* specific EOI */			;\
		outb	%al,$icu

#else /* ICU_SPECIAL_MASK_MODE */

#ifndef	AUTO_EOI_1
#define	ENABLE_ICU1(irq_num) 												 \
		movb	$(0x60|(irq_num%8)),%al			/* specific EOI */			;\
		outb	%al,$IO_ICU1
#else
#define	ENABLE_ICU1(irq_num)
#endif

#ifndef AUTO_EOI_2
#define	ENABLE_ICU1_AND_2(irq_num) 											 \
		movb	$(0x60|(irq_num%8)),%al		/* specific EOI */				;\
		outb	%al,$IO_ICU2				/* do the second ICU first */	;\
		movb	$(0x60|2),%al				/* specific EOI for IRQ2 */		;\
		outb	%al,$IO_ICU1
#else
#define	ENABLE_ICU1_AND_2(irq_num)
#endif

#ifdef ICU_HARDWARE_MASK

#define	LEGACY_MASK(irq_num, icu) 											 \
		movb	_C_LABEL(imen) + IRQ_BYTE(irq_num),%al						;\
		orb		$IRQ_BIT(irq_num),%al										;\
		movb	%al,_C_LABEL(imen) + IRQ_BYTE(irq_num)						;\
		FASTER_NOP															;\
		outb	%al,$(icu+1)

#define	LEGACY_UNMASK(irq_num, icu) 										 \
		cli																	;\
		movb	_C_LABEL(imen) + IRQ_BYTE(irq_num),%al						;\
		andb	$~IRQ_BIT(irq_num),%al										;\
		movb	%al,_C_LABEL(imen) + IRQ_BYTE(irq_num)						;\
		FASTER_NOP															;\
		outb	%al,$(icu+1)												;\
		sti

#else /* ICU_HARDWARE_MASK */

#define	LEGACY_MASK(irq_num, icu)
#define	LEGACY_UNMASK(irq_num, icu)

#endif /* ICU_HARDWARE_MASK */
#endif /* ICU_SPECIAL_MASK_MODE */

#define MY_COUNT 					_C_LABEL(cnt)
#define INTR_ADDR(intr, irq_num) 	(_C_LABEL(intr)+(irq_num) * 4)
#define	XINTR(name, irq_num)		_Xname_/**/intr/**/irq_num
#define	XHOLD(name, irq_num)		_Xname_/**/hold/**/irq_num
#define	XSTRAY(name, irq_num)		_Xname_/**/stray/**/irq_num

 /*
 * Normal vectors.
 *
 * We cdr down the intrhand chain, calling each handler with its appropriate
 * argument (0 meaning a pointer to the frame, for clock interrupts).
 *
 * The handler returns one of three values:
 *   0 - This interrupt wasn't for me.
 *   1 - This interrupt was for me.
 *  -1 - This interrupt might have been for me, but I don't know.
 * If there are no handlers, or they all return 0, we flags it as a `stray'
 * interrupt.  On a system with level-triggered interrupts, we could terminate
 * immediately when one of them returns 1; but this is a PC.
 *
 * On exit, we jump to doreti, to process soft interrupts and ASTs.
 */
#define	LEGACYINTR(name, irq_num, icu, early_icu)           			 \
IDTVEC(name_/**/recurse/**/irq_num)										;\
		pushfl															;\
		pushl	%cs														;\
		pushl	%esi													;\
		subl	$4,%esp													;\
		pushl	$T_ASTFLT				/* trap # for doing ASTs */		;\
		INTRENTRY														;\
IDTVEC(name_/**/resume/**/irq_num)										;\
		movl	$IREENT_MAGIC,TF_ERR(%esp)								;\
		movl	%ebx,%esi												;\
		movl	INTR_ADDR(intrsource, irq_num), %ebp					;\
		movl	IS_MAXLEVEL(%ebp),%ebx									;\
		jmp		1f														;\
IDTVEC(name_/**/intr/**/irq_num)                                        ;\
		pushl	$0						/* dummy error code */			;\
		pushl	$T_ASTFLT				/* trap # for doing ASTs */		;\
		INTRENTRY														;\
		movl	INTR_ADDR(intrsource, irq_num),%ebp						;\
		LEGACY_MASK(irq_num, icu)										;\
		early_icu(num)					/* and allow other intrs */		;\
		testl	%ebp,%ebp												;\
		jz		9f						/* stray */						;\
		movl	IS_MAXLEVEL(%ebp),%ebx									;\
		movl	_C_LABEL(cpl),%esi										;\
		cmpl	%ebx,%esi												;\
		jae		10f						/* currently masked; hold it */	;\
		incl	MY_COUNT+V_INTR			/* statistical info */			;\
		sti																;\
1:																		;\
		pushl	%esi													;\
		movl	%ebx,_C_LABEL(cpl)										;\
		sti																;\
		incl	_C_LABEL(idepth)										;\
		movl	IS_HANDLERS(%ebp),%ebx									;\
6:																		;\
		movl	IH_LEVEL(%ebx),%edi										;\
		cmpl	%esi,%edi												;\
		jle		7f														;\
		pushl	IH_ARG(%ebx)											;\
		movl	IH_FUN(%ebx),%eax										;\
		movl	%edi,_C_LABEL(cpl)										;\
		movl	IH_NEXT(%ebx),%ebx		/* next handler in chain */		;\
		call	*%eax					/* call it */					;\
		addl	$4,%esp					/* toss the arg */				;\
		testl	%ebx,%ebx												;\
		jnz		6b														;\
5:																		;\
		cli																;\
		LEGACY_UNMASK(irq_num)			/* unmask it in hardware */		;\
		sti																;\
		jmp		_C_LABEL(Xdoreti)		/* lower spl and do ASTs */		;\
7:																		;\
		cli																;\
		orl     $(1 << irq_num),_C_LABEL(ipending)						;\
		LEGACY_MASK(irq_num)											;\
		sti																;\
		jmp		_C_LABEL(Xdoreti)		/* lower spl and do ASTs */		;\
10:																		;\
		cli																;\
		orl     $(1 << irq_num),_C_LABEL(ipending)						;\
		LEGACY_MASK(irq_num)											;\
		sti																;\
		INTRFASTEXIT													;\
9:																		;\
		LEGACY_UNMASK(irq_num)											;\
		sti																;\
		INTRFASTEXIT

/*
 * Fast vectors.
 *
 * Like a normal vector, but run with all interrupts off.  The handler is
 * expected to be as fast as possible, and is expected to not change the
 * interrupt flag.  We pass an argument in like normal vectors, but we assume
 * that a pointer to the frame is never required.  There can be only one
 * handler on a fast vector.
 *
 * XXX
 * Note that we assume fast vectors don't do anything that would cause an AST
 * or softintr; if so, it will be deferred until the next clock tick (or
 * possibly sooner).
 */
 
#define	LEGACYFAST(name, irq_num, icu, early_icu) 							 \
IDTVEC(name_/**/fast/**/irq_num)											;\
		pushl	%eax						/* save call-used registers */	;\
		pushl	%ecx														;\
		pushl	%edx														;\
		pushl	%ds															;\
		pushl	%es															;\
		movl	$GSEL(GDATA_SEL, SEL_KPL), %eax								;\
		movl	%ax,%ds														;\
		movl	%ax,%es														;\
		/* have to do this here because %eax is lost on call */				;\
		movl	INTR_ADDR(intrsource, irq_num),%eax							;\
		incl	IH_COUNT(%eax)												;\
		pushl	IH_ARG(%eax)												;\
		call	IH_FUN(%eax)												;\
		early_icu(irq_num)													;\
		addl	$4,%esp														;\
		incl	MY_COUNT+V_INTR				/* statistical info */			;\
		popl	%es															;\
		popl	%ds															;\
		popl	%edx														;\
		popl	%ecx														;\
		popl	%eax														;\
		iret

LEGACYINTR(legacy, 0, IO_ICU1, ENABLE_ICU1)
LEGACYINTR(legacy, 1, IO_ICU1, ENABLE_ICU1)
LEGACYINTR(legacy, 2, IO_ICU1, ENABLE_ICU1)
LEGACYINTR(legacy, 3, IO_ICU1, ENABLE_ICU1)
LEGACYINTR(legacy, 4, IO_ICU1, ENABLE_ICU1)
LEGACYINTR(legacy, 5, IO_ICU1, ENABLE_ICU1)
LEGACYINTR(legacy, 6, IO_ICU1, ENABLE_ICU1)
LEGACYINTR(legacy, 7, IO_ICU1, ENABLE_ICU1)
LEGACYINTR(legacy, 8, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYINTR(legacy, 9, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYINTR(legacy, 10, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYINTR(legacy, 11, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYINTR(legacy, 12, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYINTR(legacy, 13, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYINTR(legacy, 14, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYINTR(legacy, 15, IO_ICU2, ENABLE_ICU1_AND_2)
		
LEGACYFAST(legacy, 0, IO_ICU1, ENABLE_ICU1)
LEGACYFAST(legacy, 1, IO_ICU1, ENABLE_ICU1)
LEGACYFAST(legacy, 2, IO_ICU1, ENABLE_ICU1)
LEGACYFAST(legacy, 3, IO_ICU1, ENABLE_ICU1)
LEGACYFAST(legacy, 4, IO_ICU1, ENABLE_ICU1)
LEGACYFAST(legacy, 5, IO_ICU1, ENABLE_ICU1)
LEGACYFAST(legacy, 6, IO_ICU1, ENABLE_ICU1)
LEGACYFAST(legacy, 7, IO_ICU1, ENABLE_ICU1)
LEGACYFAST(legacy, 8, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYFAST(legacy, 9, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYFAST(legacy, 10, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYFAST(legacy, 11, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYFAST(legacy, 12, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYFAST(legacy, 13, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYFAST(legacy, 14, IO_ICU2, ENABLE_ICU1_AND_2)
LEGACYFAST(legacy, 15, IO_ICU2, ENABLE_ICU1_AND_2)

		.globl _C_LABEL(i8259_stubs)
_C_LABEL(i8259_stubs):
		.long	_C_LABEL(Xlegacy_intr0), _C_LABEL(Xlegacy_intr1)
		.long	_C_LABEL(Xlegacy_intr2), _C_LABEL(Xlegacy_intr3)
		.long	_C_LABEL(Xlegacy_intr4), _C_LABEL(Xlegacy_intr5)
		.long	_C_LABEL(Xlegacy_intr6), _C_LABEL(Xlegacy_intr7)
		.long	_C_LABEL(Xlegacy_intr8), _C_LABEL(Xlegacy_intr9)
		.long	_C_LABEL(Xlegacy_intr10), _C_LABEL(Xlegacy_intr11)
		.long	_C_LABEL(Xlegacy_intr12), _C_LABEL(Xlegacy_intr13)
		.long	_C_LABEL(Xlegacy_intr14), _C_LABEL(Xlegacy_intr15)
		/* fast interrupt routine entry points */
		.long	_C_LABEL(Xlegacy_fast0), _C_LABEL(Xlegacy_fast1)
		.long	_C_LABEL(Xlegacy_fast2), _C_LABEL(Xlegacy_fast3)
		.long	_C_LABEL(Xlegacy_fast4), _C_LABEL(Xlegacy_fast5)
		.long	_C_LABEL(Xlegacy_fast6), _C_LABEL(Xlegacy_fast7)
		.long	_C_LABEL(Xlegacy_fast8), _C_LABEL(Xlegacy_fast9)
		.long	_C_LABEL(Xlegacy_fast10), _C_LABEL(Xlegacy_fast11)
		.long	_C_LABEL(Xlegacy_fast12), _C_LABEL(Xlegacy_fast13)
		.long	_C_LABEL(Xlegacy_fast14), _C_LABEL(Xlegacy_fast15)
		/* resume points for suspended interrupts */
		.long	_C_LABEL(Xlegacy_resume0), _C_LABEL(Xlegacy_resume1)
		.long	_C_LABEL(Xlegacy_resume2), _C_LABEL(Xlegacy_resume3)
		.long	_C_LABEL(Xlegacy_resume4), _C_LABEL(Xlegacy_resume5)
		.long	_C_LABEL(Xlegacy_resume6), _C_LABEL(Xlegacy_resume7)
		.long	_C_LABEL(Xlegacy_resume8), _C_LABEL(Xlegacy_resume9)
		.long	_C_LABEL(Xlegacy_resume10), _C_LABEL(Xlegacy_resume11)
		.long	_C_LABEL(Xlegacy_resume12), _C_LABEL(Xlegacy_resume13)
		.long	_C_LABEL(Xlegacy_resume14), _C_LABEL(Xlegacy_resume15)
		/* fake interrupts to resume from splx() */
		.long	_C_LABEL(Xlegacy_recurse0), _C_LABEL(Xlegacy_recurse1)
		.long	_C_LABEL(Xlegacy_recurse2), _C_LABEL(Xlegacy_recurse3)
		.long	_C_LABEL(Xlegacy_recurse4), _C_LABEL(Xlegacy_recurse5)
		.long	_C_LABEL(Xlegacy_recurse6), _C_LABEL(Xlegacy_recurse7)
		.long	_C_LABEL(Xlegacy_recurse8), _C_LABEL(Xlegacy_recurse9)
		.long	_C_LABEL(Xlegacy_recurse10), _C_LABEL(Xlegacy_recurse11)
		.long	_C_LABEL(Xlegacy_recurse12), _C_LABEL(Xlegacy_recurse13)
		.long	_C_LABEL(Xlegacy_recurse14), _C_LABEL(Xlegacy_recurse15)

/*
 * Symbols that vmstat -i wants, even though they're not used.
 */
		.globl	_C_LABEL(intrnames)
_C_LABEL(intrnames):
		.globl	_C_LABEL(eintrnames)
_C_LABEL(eintrnames):

		.globl	_C_LABEL(intrcnt)
_C_LABEL(intrcnt):
		.globl	_C_LABEL(eintrcnt)
_C_LABEL(eintrcnt):