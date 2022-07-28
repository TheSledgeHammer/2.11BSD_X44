/*	$NetBSD: vector.S,v 1.88 2022/05/24 15:55:19 bouyer Exp $	*/
/* $OpenBSD: apicvec.s,v 1.35 2018/06/18 23:15:05 bluhm Exp $ */

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

#include "assym.h"

#include <machine/asm.h>
#include <machine/apic/i8259.h>
#include <machine/apic/i82093.h>

#include <i386/isa/icu.h>

#include "ioapic.h"
#include "lapic.h"

IDTVEC(spurious)
		/*
		 * The Pentium Pro local APIC may erroneously call this vector for a
		 * default IR7.  Just ignore it.
		 *
		 * (The local APIC does this when CPL is raised while it's on the
		 * way to delivering an interrupt.. presumably enough has been set
		 * up that it's inconvenient to abort delivery completely..)
		 */
		iret

#ifdef SMP
#if NLAPIC > 0
/* Interrupt from the local APIC IPI */
IDTVEC(lapic_recurse_ipi)
		pushfl
		pushl	%cs
		pushl	%esi
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
IDTVEC(lapic_intr_ipi)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		movl	$0,local_apic+LAPIC_EOI
		movl	_C_LABEL(cpl),%ebx
		cmpl	$IPL_IPI,%ebx
		jae		2f
1:
		incl	_C_LABEL(idepth)
		movl	$IPL_IPI,_C_LABEL(cpl)
        sti
		pushl	%ebx
		call	_C_LABEL(i386_ipi_handler)
		jmp		_C_LABEL(Xdoreti)
2:
		orl		$(1 << LIR_IPI),_C_LABEL(ipending)
		sti
		INTRFASTEXIT
IDTVEC(x2apic_intr_ipi)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
		xorl	%eax,%eax
		xorl	%edx,%edx
		wrmsr
		movl	_C_LABEL(cpl),%ebx
		cmpl	$IPL_HIGH,%ebx
		jae		2f
		jmp		1f
IDTVEC(lapic_resume_ipi)
		cli
		jmp		1f

/* Interrupt from the local APIC TLB */
IDTVEC(lapic_intr_tlb)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		call	_C_LABEL(invltlb_handler)
		movl	_C_LABEL_(local_apic_va),%eax
		movl	$0,LAPIC_EOI(%eax)
		INTRFASTEXIT

IDTVEC(x2apic_intr_tlb)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		call	_C_LABEL(invltlb_handler)
		movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
		xorl	%eax,%eax
		xorl	%edx,%edx
		wrmsr
		INTRFASTEXIT
#endif /* SMP */

/* Interrupt from the local APIC timer. */
IDTVEC(lapic_recurse_ltimer)
		pushfl
		pushl	%cs
		pushl	%esi
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
IDTVEC(lapic_intr_ltimer)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		movl	_C_LABEL(local_apic_va),%ebx
		movl	$0,LAPIC_EOI(%ebx)
		movl	_C_LABEL(cpl),%ebx
		cmpl	$IPL_CLOCK,%ebx
		jae		2f
1:
		incl	_C_LABEL(idepth)
		movl	$IPL_CLOCK, _C_LABEL(cpl)
		sti
		pushl	%ebx
		pushl	$0
		call	_C_LABEL(lapic_clockintr)
		addl	$4,%esp
		jmp		_C_LABEL(Xdoreti)
2:
		orl		$(1 << LIR_TIMER),_C_LABEL(ipending)
		sti
		INTRFASTEXIT
IDTVEC(x2apic_intr_ltimer)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
		xorl	%eax,%eax
		xorl	%edx,%edx
		wrmsr
		movl	_C_LABEL(cpl),%ebx
		cmpl	$IPL_CLOCK,%ebx
		jae		2f
		jmp		1f
IDTVEC(lapic_resume_ltimer)
		cli
		jmp		1f

#endif /* NLAPIC > 0 */

#define voidop(irq_num)

#define	INTRSTUB(name, irq_num, early_icu, late_icu, mask, unmask, level_mask)	\
IDTVEC(name_/**/recurse/**/irq_num)												;\
		pushfl																	;\
		pushl	%cs																;\
		pushl	%esi															;\
		subl	$4,%esp															;\
		pushl	$T_ASTFLT						/* trap # for doing ASTs */		;\
		INTRENTRY																;\
IDTVEC(name_/**/resume/**/irq_num)												;\
		movl	$IREENT_MAGIC,TF_ERR(%esp)										;\
		movl	%ebx,%esi														;\
		movl	_C_LABEL(intrsource[irq_num]), %ebp								;\
		movl	IS_MAXLEVEL(%ebp),%ebx											;\
		jmp		1f																;\
IDTVEC(name_/**/intr/**/irq_num)                                	       		;\
pushl	$0										/* dummy error code */			;\
		pushl	$T_ASTFLT						/* trap # for doing ASTs */		;\
		INTRENTRY																;\
		movl	_C_LABEL(intrsource[irq_num]),%ebp								;\
		mask(irq_num)															;\
		early_icu(num)							/* and allow other intrs */		;\
		testl	%ebp,%ebp														;\
		jz		9f								/* stray */						;\
		movl	IS_MAXLEVEL(%ebp),%ebx											;\
		movl	_C_LABEL(cpl),%esi												;\
		cmpl	%ebx,%esi														;\
		jae		10f								/* currently masked; hold it */	;\
		incl	MY_COUNT+V_INTR					/* statistical info */			;\
		sti																		;\
1:																				;\
		pushl	%esi															;\
		movl	%ebx,_C_LABEL(cpl)												;\
		sti																		;\
		incl	_C_LABEL(idepth)												;\
		movl	IS_HANDLERS(%ebp),%ebx											;\
6:																				;\
		movl	IH_LEVEL(%ebx),%edi												;\
		cmpl	%esi,%edi														;\
		jle		7f																;\
		pushl	IH_ARG(%ebx)													;\
		movl	IH_FUN(%ebx),%eax												;\
		movl	%edi,_C_LABEL(cpl)												;\
		movl	IH_NEXT(%ebx),%ebx				/* next handler in chain */		;\
		call	*%eax							/* call it */					;\
		addl	$4,%esp							/* toss the arg */				;\
		testl	%ebx,%ebx														;\
		jnz		6b																;\
5:																				;\
		cli																		;\
		unmask(irq_num)							/* unmask it in hardware */		;\
		late_icu(irq_num)														;\
		sti																		;\
		jmp		_C_LABEL(Xdoreti)				/* lower spl and do ASTs */		;\
7:																				;\
		cli																		;\
		orl     $(1 << irq_num),_C_LABEL(ipending)								;\
		level_mask(irq_num)														;\
		late_icu(irq_num)														;\
		sti																		;\
		jmp		_C_LABEL(Xdoreti)				/* lower spl and do ASTs */		;\
10:																				;\
		cli																		;\
		orl     $(1 << irq_num),_C_LABEL(ipending)								;\
		level_mask(irq_num)														;\
		late_icu(irq_num)														;\
		sti																		;\
		INTRFASTEXIT															;\
9:																				;\
		pushl	%esp							/* for unmask */				;\
		unmask(irq_num)															;\
		late_icu(irq_num)														;\
		addl	$4,%esp															;\
		INTRFASTEXIT															;\

#define ICUADDR IO_ICU1

INTRSTUB(legacy, 0, ENABLE_ICU1, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 1, ENABLE_ICU1, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 2, ENABLE_ICU1, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 3, ENABLE_ICU1, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 4, ENABLE_ICU1, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 5, ENABLE_ICU1, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 6, ENABLE_ICU1, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 7, ENABLE_ICU1, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)

#undef ICUADDR
#define ICUADDR IO_ICU2

INTRSTUB(legacy, 8, ENABLE_ICU2, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 9, ENABLE_ICU2, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 10, ENABLE_ICU2, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 11, ENABLE_ICU2, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 12, ENABLE_ICU2, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 13, ENABLE_ICU2, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 14, ENABLE_ICU2, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)
INTRSTUB(legacy, 15, ENABLE_ICU2, voidop, LEGACY_MASK, LEGACY_UNMASK, voidop)

#if NIOAPIC > 0

INTRSTUB(apic, 0, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 1, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 2, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 3, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 4, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 5, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 6, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 7, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 8, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 9, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 10, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 11, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 12, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 13, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 14, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 15, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 16, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 17, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 18, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 19, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 20, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 21, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 22, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 23, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 24, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 25, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 26, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 27, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 28, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 29, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 30, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic, 31, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)

INTRSTUB(x2apic, 0, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 1, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 2, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 3, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 4, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 5, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 6, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 7, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 8, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 9, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 10, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 11, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 12, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 13, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 14, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 15, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 16, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 17, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 18, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 19, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 20, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 21, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 22, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 23, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 24, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 25, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 26, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 27, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 28, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 29, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 30, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic, 31, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)

#endif

		.globl _C_LABEL(i8259_stubs)
_C_LABEL(i8259_stubs):
		.long	_C_LABEL(Xlegacy_intr), _C_LABEL(Xlegacy_intr1)
		.long	_C_LABEL(Xlegacy_intr2), _C_LABEL(Xlegacy_intr3)
		.long	_C_LABEL(Xlegacy_intr4), _C_LABEL(Xlegacy_intr5)
		.long	_C_LABEL(Xlegacy_intr6), _C_LABEL(Xlegacy_intr7)
		.long	_C_LABEL(Xlegacy_intr8), _C_LABEL(Xlegacy_intr9)
		.long	_C_LABEL(Xlegacy_intr10), _C_LABEL(Xlegacy_intr11)
		.long	_C_LABEL(Xlegacy_intr12), _C_LABEL(Xlegacy_intr13)
		.long	_C_LABEL(Xlegacy_intr14), _C_LABEL(Xlegacy_intr15)
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

#if NIOAPIC > 0
		.globl _C_LABEL(apic_level_stubs)
_C_LABEL(apic_level_stubs):
		.long	_C_LABEL(Xapic_level_intr0), _C_LABEL(Xapic_level_intr1)
		.long	_C_LABEL(Xapic_level_intr2), _C_LABEL(Xapic_level_intr3)
		.long	_C_LABEL(Xapic_level_intr4), _C_LABEL(Xapic_level_intr5)
		.long	_C_LABEL(Xapic_level_intr6), _C_LABEL(Xapic_level_intr7)
		.long	_C_LABEL(Xapic_level_intr8), _C_LABEL(Xapic_level_intr9)
		.long	_C_LABEL(Xapic_level_intr10), _C_LABEL(Xapic_level_intr11)
		.long	_C_LABEL(Xapic_level_intr12), _C_LABEL(Xapic_level_intr13)
		.long	_C_LABEL(Xapic_level_intr14), _C_LABEL(Xapic_level_intr15)
		.long	_C_LABEL(Xapic_level_intr16), _C_LABEL(Xapic_level_intr17)
		.long	_C_LABEL(Xapic_level_intr18), _C_LABEL(Xapic_level_intr19)
		.long	_C_LABEL(Xapic_level_intr20), _C_LABEL(Xapic_level_intr21)
		.long	_C_LABEL(Xapic_level_intr22), _C_LABEL(Xapic_level_intr23)
		.long	_C_LABEL(Xapic_level_intr24), _C_LABEL(Xapic_level_intr25)
		.long	_C_LABEL(Xapic_level_intr26), _C_LABEL(Xapic_level_intr27)
		.long	_C_LABEL(Xapic_level_intr28), _C_LABEL(Xapic_level_intr29)
		.long	_C_LABEL(Xapic_level_intr30), _C_LABEL(Xapic_level_intr31)
		/* resume interrupts */
		.long	_C_LABEL(Xapic_level_resume0), _C_LABEL(Xapic_level_resume1)
		.long	_C_LABEL(Xapic_level_resume2), _C_LABEL(Xapic_level_resume3)
		.long	_C_LABEL(Xapic_level_resume4), _C_LABEL(Xapic_level_resume5)
		.long	_C_LABEL(Xapic_level_resume6), _C_LABEL(Xapic_level_resume7)
		.long	_C_LABEL(Xapic_level_resume8), _C_LABEL(Xapic_level_resume9)
		.long	_C_LABEL(Xapic_level_resume10), _C_LABEL(Xapic_level_resume11)
		.long	_C_LABEL(Xapic_level_resume12), _C_LABEL(Xapic_level_resume13)
		.long	_C_LABEL(Xapic_level_resume14), _C_LABEL(Xapic_level_resume15)
		.long	_C_LABEL(Xapic_level_resume16), _C_LABEL(Xapic_level_resume17)
		.long	_C_LABEL(Xapic_level_resume18), _C_LABEL(Xapic_level_resume19)
		.long	_C_LABEL(Xapic_level_resume20), _C_LABEL(Xapic_level_resume21)
		.long	_C_LABEL(Xapic_level_resume22), _C_LABEL(Xapic_level_resume23)
		.long	_C_LABEL(Xapic_level_resume24), _C_LABEL(Xapic_level_resume25)
		.long	_C_LABEL(Xapic_level_resume26), _C_LABEL(Xapic_level_resume27)
		.long	_C_LABEL(Xapic_level_resume28), _C_LABEL(Xapic_level_resume29)
		.long	_C_LABEL(Xapic_level_resume30), _C_LABEL(Xapic_level_resume31)
		/* recurse interrupts */
		.long	_C_LABEL(Xapic_level_recurse0), _C_LABEL(Xapic_level_recurse1)
		.long	_C_LABEL(Xapic_level_recurse2), _C_LABEL(Xapic_level_recurse3)
		.long	_C_LABEL(Xapic_level_recurse4), _C_LABEL(Xapic_level_recurse5)
		.long	_C_LABEL(Xapic_level_recurse6), _C_LABEL(Xapic_level_recurse7)
		.long	_C_LABEL(Xapic_level_recurse8), _C_LABEL(Xapic_level_recurse9)
		.long	_C_LABEL(Xapic_level_recurse10), _C_LABEL(Xapic_level_recurse11)
		.long	_C_LABEL(Xapic_level_recurse12), _C_LABEL(Xapic_level_recurse13)
		.long	_C_LABEL(Xapic_level_recurse14), _C_LABEL(Xapic_level_recurse15)
		.long	_C_LABEL(Xapic_level_recurse16), _C_LABEL(Xapic_level_recurse17)
		.long	_C_LABEL(Xapic_level_recurse18), _C_LABEL(Xapic_level_recurse19)
		.long	_C_LABEL(Xapic_level_recurse20), _C_LABEL(Xapic_level_recurse21)
		.long	_C_LABEL(Xapic_level_recurse22), _C_LABEL(Xapic_level_recurse23)
		.long	_C_LABEL(Xapic_level_recurse24), _C_LABEL(Xapic_level_recurse25)
		.long	_C_LABEL(Xapic_level_recurse26), _C_LABEL(Xapic_level_recurse27)
		.long	_C_LABEL(Xapic_level_recurse28), _C_LABEL(Xapic_level_recurse29)
		.long	_C_LABEL(Xapic_level_recurse30), _C_LABEL(Xapic_level_recurse31)

		.globl _C_LABEL(apic_edge_stubs)
_C_LABEL(apic_edge_stubs):
		.long	_C_LABEL(Xapic_edge_intr0), _C_LABEL(Xapic_edge_intr1)
		.long	_C_LABEL(Xapic_edge_intr2), _C_LABEL(Xapic_edge_intr3)
		.long	_C_LABEL(Xapic_edge_intr4), _C_LABEL(Xapic_edge_intr5)
		.long	_C_LABEL(Xapic_edge_intr6), _C_LABEL(Xapic_edge_intr7)
		.long	_C_LABEL(Xapic_edge_intr8), _C_LABEL(Xapic_edge_intr9)
		.long	_C_LABEL(Xapic_edge_intr10), _C_LABEL(Xapic_edge_intr11)
		.long	_C_LABEL(Xapic_edge_intr12), _C_LABEL(Xapic_edge_intr13)
		.long	_C_LABEL(Xapic_edge_intr14), _C_LABEL(Xapic_edge_intr15)
		.long	_C_LABEL(Xapic_edge_intr16), _C_LABEL(Xapic_edge_intr17)
		.long	_C_LABEL(Xapic_edge_intr18), _C_LABEL(Xapic_edge_intr19)
		.long	_C_LABEL(Xapic_edge_intr20), _C_LABEL(Xapic_edge_intr21)
		.long	_C_LABEL(Xapic_edge_intr22), _C_LABEL(Xapic_edge_intr23)
		.long	_C_LABEL(Xapic_edge_intr24), _C_LABEL(Xapic_edge_intr25)
		.long	_C_LABEL(Xapic_edge_intr26), _C_LABEL(Xapic_edge_intr27)
		.long	_C_LABEL(Xapic_edge_intr28), _C_LABEL(Xapic_edge_intr29)
		.long	_C_LABEL(Xapic_edge_intr30), _C_LABEL(Xapic_edge_intr31)
		/* resume interrupts */
		.long	_C_LABEL(Xapic_edge_resume0), _C_LABEL(Xapic_edge_resume1)
		.long	_C_LABEL(Xapic_edge_resume2), _C_LABEL(Xapic_edge_resume3)
		.long	_C_LABEL(Xapic_edge_resume4), _C_LABEL(Xapic_edge_resume5)
		.long	_C_LABEL(Xapic_edge_resume6), _C_LABEL(Xapic_edge_resume7)
		.long	_C_LABEL(Xapic_edge_resume8), _C_LABEL(Xapic_edge_resume9)
		.long	_C_LABEL(Xapic_edge_resume10), _C_LABEL(Xapic_edge_resume11)
		.long	_C_LABEL(Xapic_edge_resume12), _C_LABEL(Xapic_edge_resume13)
		.long	_C_LABEL(Xapic_edge_resume14), _C_LABEL(Xapic_edge_resume15)
		.long	_C_LABEL(Xapic_edge_resume16), _C_LABEL(Xapic_edge_resume17)
		.long	_C_LABEL(Xapic_edge_resume18), _C_LABEL(Xapic_edge_resume19)
		.long	_C_LABEL(Xapic_edge_resume20), _C_LABEL(Xapic_edge_resume21)
		.long	_C_LABEL(Xapic_edge_resume22), _C_LABEL(Xapic_edge_resume23)
		.long	_C_LABEL(Xapic_edge_resume24), _C_LABEL(Xapic_edge_resume25)
		.long	_C_LABEL(Xapic_edge_resume26), _C_LABEL(Xapic_edge_resume27)
		.long	_C_LABEL(Xapic_edge_resume28), _C_LABEL(Xapic_edge_resume29)
		.long	_C_LABEL(Xapic_edge_resume30), _C_LABEL(Xapic_edge_resume31)
		/* recurse interrupts */
		.long	_C_LABEL(Xapic_edge_recurse0), _C_LABEL(Xapic_edge_recurse1)
		.long	_C_LABEL(Xapic_edge_recurse2), _C_LABEL(Xapic_edge_recurse3)
		.long	_C_LABEL(Xapic_edge_recurse4), _C_LABEL(Xapic_edge_recurse5)
		.long	_C_LABEL(Xapic_edge_recurse6), _C_LABEL(Xapic_edge_recurse7)
		.long	_C_LABEL(Xapic_edge_recurse8), _C_LABEL(Xapic_edge_recurse9)
		.long	_C_LABEL(Xapic_edge_recurse10), _C_LABEL(Xapic_edge_recurse11)
		.long	_C_LABEL(Xapic_edge_recurse12), _C_LABEL(Xapic_edge_recurse13)
		.long	_C_LABEL(Xapic_edge_recurse14), _C_LABEL(Xapic_edge_recurse15)
		.long	_C_LABEL(Xapic_edge_recurse16), _C_LABEL(Xapic_edge_recurse17)
		.long	_C_LABEL(Xapic_edge_recurse18), _C_LABEL(Xapic_edge_recurse19)
		.long	_C_LABEL(Xapic_edge_recurse20), _C_LABEL(Xapic_edge_recurse21)
		.long	_C_LABEL(Xapic_edge_recurse22), _C_LABEL(Xapic_edge_recurse23)
		.long	_C_LABEL(Xapic_edge_recurse24), _C_LABEL(Xapic_edge_recurse25)
		.long	_C_LABEL(Xapic_edge_recurse26), _C_LABEL(Xapic_edge_recurse27)
		.long	_C_LABEL(Xapic_edge_recurse28), _C_LABEL(Xapic_edge_recurse29)
		.long	_C_LABEL(Xapic_edge_recurse30), _C_LABEL(Xapic_edge_recurse31)

		.globl _C_LABEL(x2apic_level_stubs)
_C_LABEL(x2apic_level_stubs):
		.long	_C_LABEL(Xx2apic_level_intr0), _C_LABEL(Xx2apic_level_intr1)
		.long	_C_LABEL(Xx2apic_level_intr2), _C_LABEL(Xx2apic_level_intr3)
		.long	_C_LABEL(Xx2apic_level_intr4), _C_LABEL(Xx2apic_level_intr5)
		.long	_C_LABEL(Xx2apic_level_intr6), _C_LABEL(Xx2apic_level_intr7)
		.long	_C_LABEL(Xx2apic_level_intr8), _C_LABEL(Xx2apic_level_intr9)
		.long	_C_LABEL(Xx2apic_level_intr10), _C_LABEL(Xx2apic_level_intr11)
		.long	_C_LABEL(Xx2apic_level_intr12), _C_LABEL(Xx2apic_level_intr13)
		.long	_C_LABEL(Xx2apic_level_intr14), _C_LABEL(Xx2apic_level_intr15)
		.long	_C_LABEL(Xx2apic_level_intr16), _C_LABEL(Xx2apic_level_intr17)
		.long	_C_LABEL(Xx2apic_level_intr18), _C_LABEL(Xx2apic_level_intr19)
		.long	_C_LABEL(Xx2apic_level_intr20), _C_LABEL(Xx2apic_level_intr21)
		.long	_C_LABEL(Xx2apic_level_intr22), _C_LABEL(Xx2apic_level_intr23)
		.long	_C_LABEL(Xx2apic_level_intr24), _C_LABEL(Xx2apic_level_intr25)
		.long	_C_LABEL(Xx2apic_level_intr26), _C_LABEL(Xx2apic_level_intr27)
		.long	_C_LABEL(Xx2apic_level_intr28), _C_LABEL(Xx2apic_level_intr29)
		.long	_C_LABEL(Xx2apic_level_intr30), _C_LABEL(Xx2apic_level_intr31)
		/* resume interrupts */
		.long	_C_LABEL(Xx2apic_level_resume0), _C_LABEL(Xx2apic_level_resume1)
		.long	_C_LABEL(Xx2apic_level_resume2), _C_LABEL(Xx2apic_level_resume3)
		.long	_C_LABEL(Xx2apic_level_resume4), _C_LABEL(Xx2apic_level_resume5)
		.long	_C_LABEL(Xx2apic_level_resume6), _C_LABEL(Xx2apic_level_resume7)
		.long	_C_LABEL(Xx2apic_level_resume8), _C_LABEL(Xx2apic_level_resume9)
		.long	_C_LABEL(Xx2apic_level_resume10), _C_LABEL(Xx2apic_level_resume11)
		.long	_C_LABEL(Xx2apic_level_resume12), _C_LABEL(Xx2apic_level_resume13)
		.long	_C_LABEL(Xx2apic_level_resume14), _C_LABEL(Xx2apic_level_resume15)
		.long	_C_LABEL(Xx2apic_level_resume16), _C_LABEL(Xx2apic_level_resume17)
		.long	_C_LABEL(Xx2apic_level_resume18), _C_LABEL(Xx2apic_level_resume19)
		.long	_C_LABEL(Xx2apic_level_resume20), _C_LABEL(Xx2apic_level_resume21)
		.long	_C_LABEL(Xx2apic_level_resume22), _C_LABEL(Xx2apic_level_resume23)
		.long	_C_LABEL(Xx2apic_level_resume24), _C_LABEL(Xx2apic_level_resume25)
		.long	_C_LABEL(Xx2apic_level_resume26), _C_LABEL(Xx2apic_level_resume27)
		.long	_C_LABEL(Xx2apic_level_resume28), _C_LABEL(Xx2apic_level_resume29)
		.long	_C_LABEL(Xx2apic_level_resume30), _C_LABEL(Xx2apic_level_resume31)
		/* recurse interrupts */
		.long	_C_LABEL(Xx2apic_level_recurse0), _C_LABEL(Xx2apic_level_recurse1)
		.long	_C_LABEL(Xx2apic_level_recurse2), _C_LABEL(Xx2apic_level_recurse3)
		.long	_C_LABEL(Xx2apic_level_recurse4), _C_LABEL(Xx2apic_level_recurse5)
		.long	_C_LABEL(Xx2apic_level_recurse6), _C_LABEL(Xx2apic_level_recurse7)
		.long	_C_LABEL(Xx2apic_level_recurse8), _C_LABEL(Xx2apic_level_recurse9)
		.long	_C_LABEL(Xx2apic_level_recurse10), _C_LABEL(Xx2apic_level_recurse11)
		.long	_C_LABEL(Xx2apic_level_recurse12), _C_LABEL(Xx2apic_level_recurse13)
		.long	_C_LABEL(Xx2apic_level_recurse14), _C_LABEL(Xx2apic_level_recurse15)
		.long	_C_LABEL(Xx2apic_level_recurse16), _C_LABEL(Xx2apic_level_recurse17)
		.long	_C_LABEL(Xx2apic_level_recurse18), _C_LABEL(Xx2apic_level_recurse19)
		.long	_C_LABEL(Xx2apic_level_recurse20), _C_LABEL(Xx2apic_level_recurse21)
		.long	_C_LABEL(Xx2apic_level_recurse22), _C_LABEL(Xx2apic_level_recurse23)
		.long	_C_LABEL(Xx2apic_level_recurse24), _C_LABEL(Xx2apic_level_recurse25)
		.long	_C_LABEL(Xx2apic_level_recurse26), _C_LABEL(Xx2apic_level_recurse27)
		.long	_C_LABEL(Xx2apic_level_recurse28), _C_LABEL(Xx2apic_level_recurse29)
		.long	_C_LABEL(Xx2apic_level_recurse30), _C_LABEL(Xx2apic_level_recurse31)

		.globl _C_LABEL(x2apic_edge_stubs)
_C_LABEL(x2apic_edge_stubs):
		.long	_C_LABEL(Xx2apic_edge_intr0), _C_LABEL(Xx2apic_edge_intr1)
		.long	_C_LABEL(Xx2apic_edge_intr2), _C_LABEL(Xx2apic_edge_intr3)
		.long	_C_LABEL(Xx2apic_edge_intr4), _C_LABEL(Xx2apic_edge_intr5)
		.long	_C_LABEL(Xx2apic_edge_intr6), _C_LABEL(Xx2apic_edge_intr7)
		.long	_C_LABEL(Xx2apic_edge_intr8), _C_LABEL(Xx2apic_edge_intr9)
		.long	_C_LABEL(Xx2apic_edge_intr10), _C_LABEL(Xx2apic_edge_intr11)
		.long	_C_LABEL(Xx2apic_edge_intr12), _C_LABEL(Xx2apic_edge_intr13)
		.long	_C_LABEL(Xx2apic_edge_intr14), _C_LABEL(Xx2apic_edge_intr15)
		.long	_C_LABEL(Xx2apic_edge_intr16), _C_LABEL(Xx2apic_edge_intr17)
		.long	_C_LABEL(Xx2apic_edge_intr18), _C_LABEL(Xx2apic_edge_intr19)
		.long	_C_LABEL(Xx2apic_edge_intr20), _C_LABEL(Xx2apic_edge_intr21)
		.long	_C_LABEL(Xx2apic_edge_intr22), _C_LABEL(Xx2apic_edge_intr23)
		.long	_C_LABEL(Xx2apic_edge_intr24), _C_LABEL(Xx2apic_edge_intr25)
		.long	_C_LABEL(Xx2apic_edge_intr26), _C_LABEL(Xx2apic_edge_intr27)
		.long	_C_LABEL(Xx2apic_edge_intr28), _C_LABEL(Xx2apic_edge_intr29)
		.long	_C_LABEL(Xx2apic_edge_intr30), _C_LABEL(Xx2apic_edge_intr31)
		/* resume interrupts */
		.long	_C_LABEL(Xx2apic_edge_resume0), _C_LABEL(Xx2apic_edge_resume1)
		.long	_C_LABEL(Xx2apic_edge_resume2), _C_LABEL(Xx2apic_edge_resume3)
		.long	_C_LABEL(Xx2apic_edge_resume4), _C_LABEL(Xx2apic_edge_resume5)
		.long	_C_LABEL(Xx2apic_edge_resume6), _C_LABEL(Xx2apic_edge_resume7)
		.long	_C_LABEL(Xx2apic_edge_resume8), _C_LABEL(Xx2apic_edge_resume9)
		.long	_C_LABEL(Xx2apic_edge_resume10), _C_LABEL(Xx2apic_edge_resume11)
		.long	_C_LABEL(Xx2apic_edge_resume12), _C_LABEL(Xx2apic_edge_resume13)
		.long	_C_LABEL(Xx2apic_edge_resume14), _C_LABEL(Xx2apic_edge_resume15)
		.long	_C_LABEL(Xx2apic_edge_resume16), _C_LABEL(Xx2apic_edge_resume17)
		.long	_C_LABEL(Xx2apic_edge_resume18), _C_LABEL(Xx2apic_edge_resume19)
		.long	_C_LABEL(Xx2apic_edge_resume20), _C_LABEL(Xx2apic_edge_resume21)
		.long	_C_LABEL(Xx2apic_edge_resume22), _C_LABEL(Xx2apic_edge_resume23)
		.long	_C_LABEL(Xx2apic_edge_resume24), _C_LABEL(Xx2apic_edge_resume25)
		.long	_C_LABEL(Xx2apic_edge_resume26), _C_LABEL(Xx2apic_edge_resume27)
		.long	_C_LABEL(Xx2apic_edge_resume28), _C_LABEL(Xx2apic_edge_resume29)
		.long	_C_LABEL(Xx2apic_edge_resume30), _C_LABEL(Xx2apic_edge_resume31)
		/* recurse interrupts */
		.long	_C_LABEL(Xx2apic_edge_recurse0), _C_LABEL(Xx2apic_edge_recurse1)
		.long	_C_LABEL(Xx2apic_edge_recurse2), _C_LABEL(Xx2apic_edge_recurse3)
		.long	_C_LABEL(Xx2apic_edge_recurse4), _C_LABEL(Xx2apic_edge_recurse5)
		.long	_C_LABEL(Xx2apic_edge_recurse6), _C_LABEL(Xx2apic_edge_recurse7)
		.long	_C_LABEL(Xx2apic_edge_recurse8), _C_LABEL(Xx2apic_edge_recurse9)
		.long	_C_LABEL(Xx2apic_edge_recurse10), _C_LABEL(Xx2apic_edge_recurse11)
		.long	_C_LABEL(Xx2apic_edge_recurse12), _C_LABEL(Xx2apic_edge_recurse13)
		.long	_C_LABEL(Xx2apic_edge_recurse14), _C_LABEL(Xx2apic_edge_recurse15)
		.long	_C_LABEL(Xx2apic_edge_recurse16), _C_LABEL(Xx2apic_edge_recurse17)
		.long	_C_LABEL(Xx2apic_edge_recurse18), _C_LABEL(Xx2apic_edge_recurse19)
		.long	_C_LABEL(Xx2apic_edge_recurse20), _C_LABEL(Xx2apic_edge_recurse21)
		.long	_C_LABEL(Xx2apic_edge_recurse22), _C_LABEL(Xx2apic_edge_recurse23)
		.long	_C_LABEL(Xx2apic_edge_recurse24), _C_LABEL(Xx2apic_edge_recurse25)
		.long	_C_LABEL(Xx2apic_edge_recurse26), _C_LABEL(Xx2apic_edge_recurse27)
		.long	_C_LABEL(Xx2apic_edge_recurse28), _C_LABEL(Xx2apic_edge_recurse29)
		.long	_C_LABEL(Xx2apic_edge_recurse30), _C_LABEL(Xx2apic_edge_recurse31)
#endif

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
