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
#include <machine/segments.h>
#include <machine/trap.h>

#include "ioapic.h"
#include "lapic.h"

#define MY_COUNT 			_C_LABEL(cnt)

/* XXX See comment in locore.s */
#ifdef __ELF__
#define	XINTR(name,num)		Xintr_##name##num
#define	XSTRAY(name,num)	Xstray_##name##num
#define XINTR_TSS(irq_num)	Xintr_tss_##irq_num
#else
#define	XINTR(name,num)		_Xintr_##name##num
#define	XSTRAY(name,num)	_Xstray_##name##num
#define XINTR_TSS(irq_num)	Xintr_tss_##irq_num
#endif

IDTVEC(intrspurious)
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
IDTVEC(recurse_lapic_ipi)
		pushfl
		pushl	%cs
		pushl	%esi
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
IDTVEC(intr_lapic_ipi)
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
IDTVEC(intr_x2apic_ipi)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		movl	$(MSR_APIC_000 + MSR_APIC_EOI),%ecx
		xorl	%eax,%eax
		xorl	%edx,%edx
		wrmsr
		movl	_C_LABEL(cpl),%ebx
		cmpl	$IPL_HIGH,%ebx
		jae		2f
		jmp		1f
IDTVEC(resume_lapic_ipi)
		cli
		jmp		1f

/* Interrupt from the local APIC TLB */
IDTVEC(intr_lapic_tlb)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		call	_C_LABEL(invltlb_handler)
		movl	_C_LABEL_(local_apic_va),%eax
		movl	$0,LAPIC_EOI(%eax)
		INTRFASTEXIT

IDTVEC(intr_x2apic_tlb)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		call	_C_LABEL(invltlb_handler)
		movl	$(MSR_APIC_000 + MSR_APIC_EOI),%ecx
		xorl	%eax,%eax
		xorl	%edx,%edx
		wrmsr
		INTRFASTEXIT
#endif /* SMP */

/* Interrupt from the local APIC timer. */
IDTVEC(recurse_lapic_ltimer)
		pushfl
		pushl	%cs
		pushl	%esi
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
IDTVEC(intr_lapic_ltimer)
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
IDTVEC(intr_x2apic_ltimer)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		movl	$(MSR_APIC_000 + MSR_APIC_EOI),%ecx
		xorl	%eax,%eax
		xorl	%edx,%edx
		wrmsr
		movl	_C_LABEL(cpl),%ebx
		cmpl	$IPL_CLOCK,%ebx
		jae		2f
		jmp		1f
IDTVEC(resume_lapic_ltimer)
		cli
		jmp		1f

#endif /* NLAPIC > 0 */

#define voidop(irq_num)


#define	INTRSTUB(name, irq_num, early_icu, late_icu, mask, unmask, level_mask)			\
IDTVEC(recurse_##name##irq_num)													;\
		pushfl															;\
		pushl	%cs															;\
		pushl	%esi															;\
		subl	$4,%esp														;\
		pushl	$T_ASTFLT						/* trap # for doing ASTs */			;\
		INTRENTRY															;\
IDTVEC(resume_##name##irq_num)													\
		movl	$IREENT_MAGIC,TF_ERR(%esp)											;\
		movl	%ebx,%esi														;\
		movl	_C_LABEL(intrsrc) + (irq_num) * 4, %ebp								;\
		movl	IS_MAXLEVEL(%ebp),%ebx											;\
		jmp		1f														;\
IDTVEC(intr_##name##irq_num)                                	       					\
		pushl	$0								/* dummy error code */			;\
		pushl	$T_ASTFLT						/* trap # for doing ASTs */			;\
		INTRENTRY															;\
		movl	_C_LABEL(intrsrc) + (irq_num) * 4,%ebp								;\
		mask(irq_num)														;\
		early_icu(irq_num)						/* and allow other intrs */		;\
		testl	%ebp,%ebp														;\
		jz		9f								/* stray */				;\
		movl	IS_MAXLEVEL(%ebp),%ebx											;\
		movl	_C_LABEL(cpl),%esi												;\
		cmpl	%ebx,%esi														;\
		jae		10f								/* currently masked; hold it */	;\
		incl	MY_COUNT+V_INTR					/* statistical info */				;\
1:																		;\
		pushl	%esi															;\
		movl	%ebx,_C_LABEL(cpl)												;\
		sti																;\
		incl	_C_LABEL(idepth)												;\
		movl	IS_HANDLERS(%ebp),%ebx											;\
6:																		;\
		movl	IH_LEVEL(%ebx),%edi												;\
		cmpl	%esi,%edi														;\
		jle		7f														;\
		pushl	IH_ARG(%ebx)													;\
		movl	IH_FUN(%ebx),%eax												;\
		movl	%edi,_C_LABEL(cpl)												;\
		movl	IH_NEXT(%ebx),%ebx				/* next handler in chain */			;\
		call	*%eax							/* call it */						;\
		addl	$4,%esp							/* toss the arg */				;\
		testl	%ebx,%ebx														;\
		jnz		6b														;\
5:																		;\
		cli																;\
		unmask(irq_num)							/* unmask it in hardware */		;\
		late_icu(irq_num)													;\
		sti																;\
		jmp		_C_LABEL(Xdoreti)				/* lower spl and do ASTs */		;\
7:																		;\
		cli																;\
		orl     $(1 << irq_num),_C_LABEL(ipending)									;\
		level_mask(irq_num)													;\
		late_icu(irq_num)													;\
		sti																;\
		jmp		_C_LABEL(Xdoreti)				/* lower spl and do ASTs */		;\
10:																		;\
		cli																;\
		orl     $(1 << irq_num),_C_LABEL(ipending)									;\
		level_mask(irq_num)													;\
		late_icu(irq_num)													;\
		sti																;\
		INTRFASTEXIT														;\
9:																		;\
		unmask(irq_num)														;\
		late_icu(irq_num)													;\
		INTRFASTEXIT														;\
		
		
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

INTRSTUB(apic_edge, 0, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 1, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 2, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 3, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 4, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 5, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 6, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 7, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 8, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 9, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 10, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 11, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 12, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 13, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 14, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 15, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 16, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 17, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 18, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 19, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 20, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 21, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 22, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 23, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 24, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 25, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 26, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 27, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 28, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 29, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 30, voidop, IOAPIC_ICU, voidop, voidop, voidop)
INTRSTUB(apic_edge, 31, voidop, IOAPIC_ICU, voidop, voidop, voidop)

INTRSTUB(apic_level, 0, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 1, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 2, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 3, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 4, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 5, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 6, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 7, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 8, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 9, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 10, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 11, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 12, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 13, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 14, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 15, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 16, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 17, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 18, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 19, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 20, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 21, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 22, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 23, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 24, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 25, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 26, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 27, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 28, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 29, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 30, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(apic_level, 31, voidop, IOAPIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)

INTRSTUB(x2apic_edge, 0, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 1, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 2, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 3, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 4, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 5, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 6, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 7, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 8, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 9, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 10, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 11, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 12, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 13, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 14, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 15, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 16, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 17, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 18, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 19, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 20, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 21, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 22, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 23, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 24, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 25, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 26, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 27, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 28, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 29, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 30, voidop, X2APIC_ICU, voidop, voidop, voidop)
INTRSTUB(x2apic_edge, 31, voidop, X2APIC_ICU, voidop, voidop, voidop)

INTRSTUB(x2apic_level, 0, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 1, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 2, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 3, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 4, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 5, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 6, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 7, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 8, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 9, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 10, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 11, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 12, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 13, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 14, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 15, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 16, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 17, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 18, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 19, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 20, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 21, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 22, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 23, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 24, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 25, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 26, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 27, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 28, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 29, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 30, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)
INTRSTUB(x2apic_level, 31, voidop, X2APIC_ICU, voidop, IOAPIC_MASK, IOAPIC_UNMASK)

#endif

		.globl _C_LABEL(legacy_stubs)
_C_LABEL(legacy_stubs):
		.long	_C_LABEL(Xintr_legacy0), _C_LABEL(Xintr_legacy1)
		.long	_C_LABEL(Xintr_legacy2), _C_LABEL(Xintr_legacy3)
		.long	_C_LABEL(Xintr_legacy4), _C_LABEL(Xintr_legacy5)
		.long	_C_LABEL(Xintr_legacy6), _C_LABEL(Xintr_legacy7)
		.long	_C_LABEL(Xintr_legacy8), _C_LABEL(Xintr_legacy9)
		.long	_C_LABEL(Xintr_legacy10), _C_LABEL(Xintr_legacy11)
		.long	_C_LABEL(Xintr_legacy12), _C_LABEL(Xintr_legacy13)
		.long	_C_LABEL(Xintr_legacy14), _C_LABEL(Xintr_legacy15)
		/* resume points for suspended interrupts */
		.long	_C_LABEL(Xresume_legacy0), _C_LABEL(Xresume_legacy1)
		.long	_C_LABEL(Xresume_legacy2), _C_LABEL(Xresume_legacy3)
		.long	_C_LABEL(Xresume_legacy4), _C_LABEL(Xresume_legacy5)
		.long	_C_LABEL(Xresume_legacy6), _C_LABEL(Xresume_legacy7)
		.long	_C_LABEL(Xresume_legacy8), _C_LABEL(Xresume_legacy9)
		.long	_C_LABEL(Xresume_legacy10), _C_LABEL(Xresume_legacy11)
		.long	_C_LABEL(Xresume_legacy12), _C_LABEL(Xresume_legacy13)
		.long	_C_LABEL(Xresume_legacy14), _C_LABEL(Xresume_legacy15)
		/* fake interrupts to resume from splx() */
		.long	_C_LABEL(Xrecurse_legacy0), _C_LABEL(Xrecurse_legacy1)
		.long	_C_LABEL(Xrecurse_legacy2), _C_LABEL(Xrecurse_legacy3)
		.long	_C_LABEL(Xrecurse_legacy4), _C_LABEL(Xrecurse_legacy5)
		.long	_C_LABEL(Xrecurse_legacy6), _C_LABEL(Xrecurse_legacy7)
		.long	_C_LABEL(Xrecurse_legacy8), _C_LABEL(Xrecurse_legacy9)
		.long	_C_LABEL(Xrecurse_legacy10), _C_LABEL(Xrecurse_legacy11)
		.long	_C_LABEL(Xrecurse_legacy12), _C_LABEL(Xrecurse_legacy13)
		.long	_C_LABEL(Xrecurse_legacy14), _C_LABEL(Xrecurse_legacy15)

#if NIOAPIC > 0
		.globl _C_LABEL(apic_level_stubs)
_C_LABEL(apic_level_stubs):
		.long	_C_LABEL(Xintr_apic_level0), _C_LABEL(Xintr_apic_level1)
		.long	_C_LABEL(Xintr_apic_level2), _C_LABEL(Xintr_apic_level3)
		.long	_C_LABEL(Xintr_apic_level4), _C_LABEL(Xintr_apic_level5)
		.long	_C_LABEL(Xintr_apic_level6), _C_LABEL(Xintr_apic_level7)
		.long	_C_LABEL(Xintr_apic_level8), _C_LABEL(Xintr_apic_level9)
		.long	_C_LABEL(Xintr_apic_level10), _C_LABEL(Xintr_apic_level11)
		.long	_C_LABEL(Xintr_apic_level12), _C_LABEL(Xintr_apic_level13)
		.long	_C_LABEL(Xintr_apic_level14), _C_LABEL(Xintr_apic_level15)
		.long	_C_LABEL(Xintr_apic_level16), _C_LABEL(Xintr_apic_level17)
		.long	_C_LABEL(Xintr_apic_level18), _C_LABEL(Xintr_apic_level19)
		.long	_C_LABEL(Xintr_apic_level20), _C_LABEL(Xintr_apic_level21)
		.long	_C_LABEL(Xintr_apic_level22), _C_LABEL(Xintr_apic_level23)
		.long	_C_LABEL(Xintr_apic_level24), _C_LABEL(Xintr_apic_level25)
		.long	_C_LABEL(Xintr_apic_level26), _C_LABEL(Xintr_apic_level27)
		.long	_C_LABEL(Xintr_apic_level28), _C_LABEL(Xintr_apic_level29)
		.long	_C_LABEL(Xintr_apic_level30), _C_LABEL(Xintr_apic_level31)
		/* resume interrupts */
		.long	_C_LABEL(Xresume_apic_level0), _C_LABEL(Xresume_apic_level1)
		.long	_C_LABEL(Xresume_apic_level2), _C_LABEL(Xresume_apic_level3)
		.long	_C_LABEL(Xresume_apic_level4), _C_LABEL(Xresume_apic_level5)
		.long	_C_LABEL(Xresume_apic_level6), _C_LABEL(Xresume_apic_level7)
		.long	_C_LABEL(Xresume_apic_level8), _C_LABEL(Xresume_apic_level9)
		.long	_C_LABEL(Xresume_apic_level10), _C_LABEL(Xresume_apic_level11)
		.long	_C_LABEL(Xresume_apic_level12), _C_LABEL(Xresume_apic_level13)
		.long	_C_LABEL(Xresume_apic_level14), _C_LABEL(Xresume_apic_level15)
		.long	_C_LABEL(Xresume_apic_level16), _C_LABEL(Xresume_apic_level17)
		.long	_C_LABEL(Xresume_apic_level18), _C_LABEL(Xresume_apic_level19)
		.long	_C_LABEL(Xresume_apic_level20), _C_LABEL(Xresume_apic_level21)
		.long	_C_LABEL(Xresume_apic_level22), _C_LABEL(Xresume_apic_level23)
		.long	_C_LABEL(Xresume_apic_level24), _C_LABEL(Xresume_apic_level25)
		.long	_C_LABEL(Xresume_apic_level26), _C_LABEL(Xresume_apic_level27)
		.long	_C_LABEL(Xresume_apic_level28), _C_LABEL(Xresume_apic_level29)
		.long	_C_LABEL(Xresume_apic_level30), _C_LABEL(Xresume_apic_level31)
		/* recurse interrupts */
		.long	_C_LABEL(Xrecurse_apic_level0), _C_LABEL(Xrecurse_apic_level1)
		.long	_C_LABEL(Xrecurse_apic_level2), _C_LABEL(Xrecurse_apic_level3)
		.long	_C_LABEL(Xrecurse_apic_level4), _C_LABEL(Xrecurse_apic_level5)
		.long	_C_LABEL(Xrecurse_apic_level6), _C_LABEL(Xrecurse_apic_level7)
		.long	_C_LABEL(Xrecurse_apic_level8), _C_LABEL(Xrecurse_apic_level9)
		.long	_C_LABEL(Xrecurse_apic_level10), _C_LABEL(Xrecurse_apic_level11)
		.long	_C_LABEL(Xrecurse_apic_level12), _C_LABEL(Xrecurse_apic_level13)
		.long	_C_LABEL(Xrecurse_apic_level14), _C_LABEL(Xrecurse_apic_level15)
		.long	_C_LABEL(Xrecurse_apic_level16), _C_LABEL(Xrecurse_apic_level17)
		.long	_C_LABEL(Xrecurse_apic_level18), _C_LABEL(Xrecurse_apic_level19)
		.long	_C_LABEL(Xrecurse_apic_level20), _C_LABEL(Xrecurse_apic_level21)
		.long	_C_LABEL(Xrecurse_apic_level22), _C_LABEL(Xrecurse_apic_level23)
		.long	_C_LABEL(Xrecurse_apic_level24), _C_LABEL(Xrecurse_apic_level25)
		.long	_C_LABEL(Xrecurse_apic_level26), _C_LABEL(Xrecurse_apic_level27)
		.long	_C_LABEL(Xrecurse_apic_level28), _C_LABEL(Xrecurse_apic_level29)
		.long	_C_LABEL(Xrecurse_apic_level30), _C_LABEL(Xrecurse_apic_level31)

		.globl _C_LABEL(apic_edge_stubs)
_C_LABEL(apic_edge_stubs):
		.long	_C_LABEL(Xintr_apic_edge0), _C_LABEL(Xintr_apic_edge1)
		.long	_C_LABEL(Xintr_apic_edge2), _C_LABEL(Xintr_apic_edge3)
		.long	_C_LABEL(Xintr_apic_edge4), _C_LABEL(Xintr_apic_edge5)
		.long	_C_LABEL(Xintr_apic_edge6), _C_LABEL(Xintr_apic_edge7)
		.long	_C_LABEL(Xintr_apic_edge8), _C_LABEL(Xintr_apic_edge9)
		.long	_C_LABEL(Xintr_apic_edge10), _C_LABEL(Xintr_apic_edge11)
		.long	_C_LABEL(Xintr_apic_edge12), _C_LABEL(Xintr_apic_edge13)
		.long	_C_LABEL(Xintr_apic_edge14), _C_LABEL(Xintr_apic_edge15)
		.long	_C_LABEL(Xintr_apic_edge16), _C_LABEL(Xintr_apic_edge17)
		.long	_C_LABEL(Xintr_apic_edge18), _C_LABEL(Xintr_apic_edge19)
		.long	_C_LABEL(Xintr_apic_edge20), _C_LABEL(Xintr_apic_edge21)
		.long	_C_LABEL(Xintr_apic_edge22), _C_LABEL(Xintr_apic_edge23)
		.long	_C_LABEL(Xintr_apic_edge24), _C_LABEL(Xintr_apic_edge25)
		.long	_C_LABEL(Xintr_apic_edge26), _C_LABEL(Xintr_apic_edge27)
		.long	_C_LABEL(Xintr_apic_edge28), _C_LABEL(Xintr_apic_edge29)
		.long	_C_LABEL(Xintr_apic_edge30), _C_LABEL(Xintr_apic_edge31)
		/* resume interrupts */
		.long	_C_LABEL(Xresume_apic_edge0), _C_LABEL(Xresume_apic_edge1)
		.long	_C_LABEL(Xresume_apic_edge2), _C_LABEL(Xresume_apic_edge3)
		.long	_C_LABEL(Xresume_apic_edge4), _C_LABEL(Xresume_apic_edge5)
		.long	_C_LABEL(Xresume_apic_edge6), _C_LABEL(Xresume_apic_edge7)
		.long	_C_LABEL(Xresume_apic_edge8), _C_LABEL(Xresume_apic_edge9)
		.long	_C_LABEL(Xresume_apic_edge10), _C_LABEL(Xresume_apic_edge11)
		.long	_C_LABEL(Xresume_apic_edge12), _C_LABEL(Xresume_apic_edge13)
		.long	_C_LABEL(Xresume_apic_edge14), _C_LABEL(Xresume_apic_edge15)
		.long	_C_LABEL(Xresume_apic_edge16), _C_LABEL(Xresume_apic_edge17)
		.long	_C_LABEL(Xresume_apic_edge18), _C_LABEL(Xresume_apic_edge19)
		.long	_C_LABEL(Xresume_apic_edge20), _C_LABEL(Xresume_apic_edge21)
		.long	_C_LABEL(Xresume_apic_edge22), _C_LABEL(Xresume_apic_edge23)
		.long	_C_LABEL(Xresume_apic_edge24), _C_LABEL(Xresume_apic_edge25)
		.long	_C_LABEL(Xresume_apic_edge26), _C_LABEL(Xresume_apic_edge27)
		.long	_C_LABEL(Xresume_apic_edge28), _C_LABEL(Xresume_apic_edge29)
		.long	_C_LABEL(Xresume_apic_edge30), _C_LABEL(Xresume_apic_edge31)
		/* recurse interrupts */
		.long	_C_LABEL(Xrecurse_apic_edge0), _C_LABEL(Xrecurse_apic_edge1)
		.long	_C_LABEL(Xrecurse_apic_edge2), _C_LABEL(Xrecurse_apic_edge3)
		.long	_C_LABEL(Xrecurse_apic_edge4), _C_LABEL(Xrecurse_apic_edge5)
		.long	_C_LABEL(Xrecurse_apic_edge6), _C_LABEL(Xrecurse_apic_edge7)
		.long	_C_LABEL(Xrecurse_apic_edge8), _C_LABEL(Xrecurse_apic_edge9)
		.long	_C_LABEL(Xrecurse_apic_edge10), _C_LABEL(Xrecurse_apic_edge11)
		.long	_C_LABEL(Xrecurse_apic_edge12), _C_LABEL(Xrecurse_apic_edge13)
		.long	_C_LABEL(Xrecurse_apic_edge14), _C_LABEL(Xrecurse_apic_edge15)
		.long	_C_LABEL(Xrecurse_apic_edge16), _C_LABEL(Xrecurse_apic_edge17)
		.long	_C_LABEL(Xrecurse_apic_edge18), _C_LABEL(Xrecurse_apic_edge19)
		.long	_C_LABEL(Xrecurse_apic_edge20), _C_LABEL(Xrecurse_apic_edge21)
		.long	_C_LABEL(Xrecurse_apic_edge22), _C_LABEL(Xrecurse_apic_edge23)
		.long	_C_LABEL(Xrecurse_apic_edge24), _C_LABEL(Xrecurse_apic_edge25)
		.long	_C_LABEL(Xrecurse_apic_edge26), _C_LABEL(Xrecurse_apic_edge27)
		.long	_C_LABEL(Xrecurse_apic_edge28), _C_LABEL(Xrecurse_apic_edge29)
		.long	_C_LABEL(Xrecurse_apic_edge30), _C_LABEL(Xrecurse_apic_edge31)

		.globl _C_LABEL(x2apic_level_stubs)
_C_LABEL(x2apic_level_stubs):
		.long	_C_LABEL(Xintr_x2apic_level0), _C_LABEL(Xintr_x2apic_level1)
		.long	_C_LABEL(Xintr_x2apic_level2), _C_LABEL(Xintr_x2apic_level3)
		.long	_C_LABEL(Xintr_x2apic_level4), _C_LABEL(Xintr_x2apic_level5)
		.long	_C_LABEL(Xintr_x2apic_level6), _C_LABEL(Xintr_x2apic_level7)
		.long	_C_LABEL(Xintr_x2apic_level8), _C_LABEL(Xintr_x2apic_level9)
		.long	_C_LABEL(Xintr_x2apic_level10), _C_LABEL(Xintr_x2apic_level11)
		.long	_C_LABEL(Xintr_x2apic_level12), _C_LABEL(Xintr_x2apic_level13)
		.long	_C_LABEL(Xintr_x2apic_level14), _C_LABEL(Xintr_x2apic_level15)
		.long	_C_LABEL(Xintr_x2apic_level16), _C_LABEL(Xintr_x2apic_level17)
		.long	_C_LABEL(Xintr_x2apic_level18), _C_LABEL(Xintr_x2apic_level19)
		.long	_C_LABEL(Xintr_x2apic_level20), _C_LABEL(Xintr_x2apic_level21)
		.long	_C_LABEL(Xintr_x2apic_level22), _C_LABEL(Xintr_x2apic_level23)
		.long	_C_LABEL(Xintr_x2apic_level24), _C_LABEL(Xintr_x2apic_level25)
		.long	_C_LABEL(Xintr_x2apic_level26), _C_LABEL(Xintr_x2apic_level27)
		.long	_C_LABEL(Xintr_x2apic_level28), _C_LABEL(Xintr_x2apic_level29)
		.long	_C_LABEL(Xintr_x2apic_level30), _C_LABEL(Xintr_x2apic_level31)
		/* resume interrupts */
		.long	_C_LABEL(Xresume_x2apic_level0), _C_LABEL(Xresume_x2apic_level1)
		.long	_C_LABEL(Xresume_x2apic_level2), _C_LABEL(Xresume_x2apic_level3)
		.long	_C_LABEL(Xresume_x2apic_level4), _C_LABEL(Xresume_x2apic_level5)
		.long	_C_LABEL(Xresume_x2apic_level6), _C_LABEL(Xresume_x2apic_level7)
		.long	_C_LABEL(Xresume_x2apic_level8), _C_LABEL(Xresume_x2apic_level9)
		.long	_C_LABEL(Xresume_x2apic_level10), _C_LABEL(Xresume_x2apic_level11)
		.long	_C_LABEL(Xresume_x2apic_level12), _C_LABEL(Xresume_x2apic_level13)
		.long	_C_LABEL(Xresume_x2apic_level14), _C_LABEL(Xresume_x2apic_level15)
		.long	_C_LABEL(Xresume_x2apic_level16), _C_LABEL(Xresume_x2apic_level17)
		.long	_C_LABEL(Xresume_x2apic_level18), _C_LABEL(Xresume_x2apic_level19)
		.long	_C_LABEL(Xresume_x2apic_level20), _C_LABEL(Xresume_x2apic_level21)
		.long	_C_LABEL(Xresume_x2apic_level22), _C_LABEL(Xresume_x2apic_level23)
		.long	_C_LABEL(Xresume_x2apic_level24), _C_LABEL(Xresume_x2apic_level25)
		.long	_C_LABEL(Xresume_x2apic_level26), _C_LABEL(Xresume_x2apic_level27)
		.long	_C_LABEL(Xresume_x2apic_level28), _C_LABEL(Xresume_x2apic_level29)
		.long	_C_LABEL(Xresume_x2apic_level30), _C_LABEL(Xresume_x2apic_level31)
		/* recurse interrupts */
		.long	_C_LABEL(Xrecurse_x2apic_level0), _C_LABEL(Xrecurse_x2apic_level1)
		.long	_C_LABEL(Xrecurse_x2apic_level2), _C_LABEL(Xrecurse_x2apic_level3)
		.long	_C_LABEL(Xrecurse_x2apic_level4), _C_LABEL(Xrecurse_x2apic_level5)
		.long	_C_LABEL(Xrecurse_x2apic_level6), _C_LABEL(Xrecurse_x2apic_level7)
		.long	_C_LABEL(Xrecurse_x2apic_level8), _C_LABEL(Xrecurse_x2apic_level9)
		.long	_C_LABEL(Xrecurse_x2apic_level10), _C_LABEL(Xrecurse_x2apic_level11)
		.long	_C_LABEL(Xrecurse_x2apic_level12), _C_LABEL(Xrecurse_x2apic_level13)
		.long	_C_LABEL(Xrecurse_x2apic_level14), _C_LABEL(Xrecurse_x2apic_level15)
		.long	_C_LABEL(Xrecurse_x2apic_level16), _C_LABEL(Xrecurse_x2apic_level17)
		.long	_C_LABEL(Xrecurse_x2apic_level18), _C_LABEL(Xrecurse_x2apic_level19)
		.long	_C_LABEL(Xrecurse_x2apic_level20), _C_LABEL(Xrecurse_x2apic_level21)
		.long	_C_LABEL(Xrecurse_x2apic_level22), _C_LABEL(Xrecurse_x2apic_level23)
		.long	_C_LABEL(Xrecurse_x2apic_level24), _C_LABEL(Xrecurse_x2apic_level25)
		.long	_C_LABEL(Xrecurse_x2apic_level26), _C_LABEL(Xrecurse_x2apic_level27)
		.long	_C_LABEL(Xrecurse_x2apic_level28), _C_LABEL(Xrecurse_x2apic_level29)
		.long	_C_LABEL(Xrecurse_x2apic_level30), _C_LABEL(Xrecurse_x2apic_level31)

		.globl _C_LABEL(x2apic_edge_stubs)
_C_LABEL(x2apic_edge_stubs):
		.long	_C_LABEL(Xintr_x2apic_edge0), _C_LABEL(Xintr_x2apic_edge1)
		.long	_C_LABEL(Xintr_x2apic_edge2), _C_LABEL(Xintr_x2apic_edge3)
		.long	_C_LABEL(Xintr_x2apic_edge4), _C_LABEL(Xintr_x2apic_edge5)
		.long	_C_LABEL(Xintr_x2apic_edge6), _C_LABEL(Xintr_x2apic_edge7)
		.long	_C_LABEL(Xintr_x2apic_edge8), _C_LABEL(Xintr_x2apic_edge9)
		.long	_C_LABEL(Xintr_x2apic_edge10), _C_LABEL(Xintr_x2apic_edge11)
		.long	_C_LABEL(Xintr_x2apic_edge12), _C_LABEL(Xintr_x2apic_edge13)
		.long	_C_LABEL(Xintr_x2apic_edge14), _C_LABEL(Xintr_x2apic_edge15)
		.long	_C_LABEL(Xintr_x2apic_edge16), _C_LABEL(Xintr_x2apic_edge17)
		.long	_C_LABEL(Xintr_x2apic_edge18), _C_LABEL(Xintr_x2apic_edge19)
		.long	_C_LABEL(Xintr_x2apic_edge20), _C_LABEL(Xintr_x2apic_edge21)
		.long	_C_LABEL(Xintr_x2apic_edge22), _C_LABEL(Xintr_x2apic_edge23)
		.long	_C_LABEL(Xintr_x2apic_edge24), _C_LABEL(Xintr_x2apic_edge25)
		.long	_C_LABEL(Xintr_x2apic_edge26), _C_LABEL(Xintr_x2apic_edge27)
		.long	_C_LABEL(Xintr_x2apic_edge28), _C_LABEL(Xintr_x2apic_edge29)
		.long	_C_LABEL(Xintr_x2apic_edge30), _C_LABEL(Xintr_x2apic_edge31)
		/* resume interrupts */
		.long	_C_LABEL(Xresume_x2apic_edge0), _C_LABEL(Xresume_x2apic_edge1)
		.long	_C_LABEL(Xresume_x2apic_edge2), _C_LABEL(Xresume_x2apic_edge3)
		.long	_C_LABEL(Xresume_x2apic_edge4), _C_LABEL(Xresume_x2apic_edge5)
		.long	_C_LABEL(Xresume_x2apic_edge6), _C_LABEL(Xresume_x2apic_edge7)
		.long	_C_LABEL(Xresume_x2apic_edge8), _C_LABEL(Xresume_x2apic_edge9)
		.long	_C_LABEL(Xresume_x2apic_edge10), _C_LABEL(Xresume_x2apic_edge11)
		.long	_C_LABEL(Xresume_x2apic_edge12), _C_LABEL(Xresume_x2apic_edge13)
		.long	_C_LABEL(Xresume_x2apic_edge14), _C_LABEL(Xresume_x2apic_edge15)
		.long	_C_LABEL(Xresume_x2apic_edge16), _C_LABEL(Xresume_x2apic_edge17)
		.long	_C_LABEL(Xresume_x2apic_edge18), _C_LABEL(Xresume_x2apic_edge19)
		.long	_C_LABEL(Xresume_x2apic_edge20), _C_LABEL(Xresume_x2apic_edge21)
		.long	_C_LABEL(Xresume_x2apic_edge22), _C_LABEL(Xresume_x2apic_edge23)
		.long	_C_LABEL(Xresume_x2apic_edge24), _C_LABEL(Xresume_x2apic_edge25)
		.long	_C_LABEL(Xresume_x2apic_edge26), _C_LABEL(Xresume_x2apic_edge27)
		.long	_C_LABEL(Xresume_x2apic_edge28), _C_LABEL(Xresume_x2apic_edge29)
		.long	_C_LABEL(Xresume_x2apic_edge30), _C_LABEL(Xresume_x2apic_edge31)
		/* recurse interrupts */
		.long	_C_LABEL(Xrecurse_x2apic_edge0), _C_LABEL(Xrecurse_x2apic_edge1)
		.long	_C_LABEL(Xrecurse_x2apic_edge2), _C_LABEL(Xrecurse_x2apic_edge3)
		.long	_C_LABEL(Xrecurse_x2apic_edge4), _C_LABEL(Xrecurse_x2apic_edge5)
		.long	_C_LABEL(Xrecurse_x2apic_edge6), _C_LABEL(Xrecurse_x2apic_edge7)
		.long	_C_LABEL(Xrecurse_x2apic_edge8), _C_LABEL(Xrecurse_x2apic_edge9)
		.long	_C_LABEL(Xrecurse_x2apic_edge10), _C_LABEL(Xrecurse_x2apic_edge11)
		.long	_C_LABEL(Xrecurse_x2apic_edge12), _C_LABEL(Xrecurse_x2apic_edge13)
		.long	_C_LABEL(Xrecurse_x2apic_edge14), _C_LABEL(Xrecurse_x2apic_edge15)
		.long	_C_LABEL(Xrecurse_x2apic_edge16), _C_LABEL(Xrecurse_x2apic_edge17)
		.long	_C_LABEL(Xrecurse_x2apic_edge18), _C_LABEL(Xrecurse_x2apic_edge19)
		.long	_C_LABEL(Xrecurse_x2apic_edge20), _C_LABEL(Xrecurse_x2apic_edge21)
		.long	_C_LABEL(Xrecurse_x2apic_edge22), _C_LABEL(Xrecurse_x2apic_edge23)
		.long	_C_LABEL(Xrecurse_x2apic_edge24), _C_LABEL(Xrecurse_x2apic_edge25)
		.long	_C_LABEL(Xrecurse_x2apic_edge26), _C_LABEL(Xrecurse_x2apic_edge27)
		.long	_C_LABEL(Xrecurse_x2apic_edge28), _C_LABEL(Xrecurse_x2apic_edge29)
		.long	_C_LABEL(Xrecurse_x2apic_edge30), _C_LABEL(Xrecurse_x2apic_edge31)
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
