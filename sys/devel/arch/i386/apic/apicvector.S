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
#include <devel/arch/i386/apic/ioapicreg.h>
#include <devel/arch/i386/apic/ioaaicvar.h>
#include <devel/arch/i386/apic/lapicreg.h>
#include <devel/arch/i386/apic/lapicvar.h>
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

/* belongs in icu.s when ready */
		.globl _C_LABEL(idepth)
_C_LABEL(idepth):
		.long	0xffff
		
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
		call	invltlb
		movl	_C_LABEL_(local_apic_va),%eax
		movl	$0,LAPIC_EOI(%eax)
		INTRFASTEXIT
		
IDTVEC(x2apic_intr_tlb)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		call	invltlb
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
		
#define IOAPIC_ICU(irq_num)												 \
		movl	_C_LABEL(local_apic_va),%eax							;\
		movl	$0,LAPIC_EOI(%eax)
		
#define X2APIC_ICU(irq_num)												 \
		movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx 				;\
		xorl	%eax,%eax												;\
		xorl	%edx,%edx												;\
		wrmsr

#define IOAPIC_MASK(irq_num)											 \
		movl	IS_PIC(%ebp),%edi										;\
		movl	IS_PIN(%ebp),%esi										;\
		leal	0x10(%esi,%esi,1),%esi									;\
		movl	PIC_IOAPIC(%edi),%edi									;\
		movl	IOAPIC_SC_REG(%edi),%ebx								;\
		movl	%esi, (%ebx)											;\
		movl	IOAPIC_SC_DATA(%edi),%ebx								;\
		movl	(%ebx),%esi												;\
		orl		$IOAPIC_REDLO_MASK,%esi									;\
		andl	$~IOAPIC_REDLO_RIRR,%esi								;\
		movl	%esi,(%ebx)												;\
		movl	IS_PIC(%ebp),%edi										;\

#define IOAPIC_UNMASK(irq_num)											 \
		movl    (%esp),%eax												;\
		cmpl    $IREENT_MAGIC,(TF_ERR+4)(%eax)							;\
		jne     79f														;\
		movl	IS_PIC(%ebp),%edi										;\
		movl	IS_PIN(%ebp),%esi										;\
		leal	0x10(%esi,%esi,1),%esi									;\
		movl	PIC_IOAPIC(%edi),%edi									;\
		movl	IOAPIC_SC_REG(%edi),%ebx								;\
		movl	IOAPIC_SC_DATA(%edi),%eax								;\
		movl	%esi, (%ebx)											;\
		movl	(%eax),%edx												;\
		andl	$~(IOAPIC_REDLO_MASK|IOAPIC_REDLO_RIRR),%edx			;\
		movl	%esi, (%ebx)											;\
		movl	%edx,(%eax)												;\
		movl	IS_PIC(%ebp),%edi										;\
79:

	/*
	 * I/O APIC interrupt.
	 * We sort out which one is which based on the value of
	 * the processor priority register.
	 *
	 * XXX use cmove when appropriate.
	 */
#define	APICINTR(name, irq_num, late_icu)                        		\
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
		IOAPIC_MASK(irq_num)											;\
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
		IOAPIC_UNMASK(irq_num)			/* unmask it in hardware */		;\
		late_icu(irq_num)												;\
		sti																;\
		jmp		_C_LABEL(Xdoreti)		/* lower spl and do ASTs */		;\
7:																		;\
		cli																;\
		orl     $(1 << irq_num),_C_LABEL(ipending)						;\
		IOAPIC_MASK(irq_num)											;\
		late_icu(irq_num)												;\
		sti																;\
		jmp		_C_LABEL(Xdoreti)		/* lower spl and do ASTs */		;\
10:																		;\
		cli																;\
		orl     $(1 << irq_num),_C_LABEL(ipending)						;\
		IOAPIC_MASK(irq_num)											;\
		late_icu(irq_num)												;\
		sti																;\
		INTRFASTEXIT													;\
9:																		;\
		IOAPIC_UNMASK(irq_num)											;\
		late_icu(irq_num)												;\

#if NIOAPIC > 0

APICINTR(apic, 0, IOAPIC_ICU)
APICINTR(apic, 1, IOAPIC_ICU)
APICINTR(apic, 2, IOAPIC_ICU)
APICINTR(apic, 3, IOAPIC_ICU)
APICINTR(apic, 4, IOAPIC_ICU)
APICINTR(apic, 5, IOAPIC_ICU)
APICINTR(apic, 6, IOAPIC_ICU)
APICINTR(apic, 7, IOAPIC_ICU)
APICINTR(apic, 8, IOAPIC_ICU)
APICINTR(apic, 9, IOAPIC_ICU)
APICINTR(apic, 10, IOAPIC_ICU)
APICINTR(apic, 11, IOAPIC_ICU)
APICINTR(apic, 12, IOAPIC_ICU)
APICINTR(apic, 13, IOAPIC_ICU)
APICINTR(apic, 14, IOAPIC_ICU)
APICINTR(apic, 15, IOAPIC_ICU)
APICINTR(apic, 16, IOAPIC_ICU)
APICINTR(apic, 17, IOAPIC_ICU)
APICINTR(apic, 18, IOAPIC_ICU)
APICINTR(apic, 19, IOAPIC_ICU)
APICINTR(apic, 20, IOAPIC_ICU)
APICINTR(apic, 21, IOAPIC_ICU)
APICINTR(apic, 22, IOAPIC_ICU)
APICINTR(apic, 23, IOAPIC_ICU)
APICINTR(apic, 24, IOAPIC_ICU)
APICINTR(apic, 25, IOAPIC_ICU)
APICINTR(apic, 26, IOAPIC_ICU)
APICINTR(apic, 27, IOAPIC_ICU)
APICINTR(apic, 28, IOAPIC_ICU)
APICINTR(apic, 29, IOAPIC_ICU)
APICINTR(apic, 30, IOAPIC_ICU)
APICINTR(apic, 31, IOAPIC_ICU)

APICINTR(x2apic, 0, X2APIC_ICU)
APICINTR(x2apic, 1, X2APIC_ICU)
APICINTR(x2apic, 2, X2APIC_ICU)
APICINTR(x2apic, 3, X2APIC_ICU)
APICINTR(x2apic, 4, X2APIC_ICU)
APICINTR(x2apic, 5, X2APIC_ICU)
APICINTR(x2apic, 6, X2APIC_ICU)
APICINTR(x2apic, 7, X2APIC_ICU)
APICINTR(x2apic, 8, X2APIC_ICU)
APICINTR(x2apic, 9, X2APIC_ICU)
APICINTR(x2apic, 10, X2APIC_ICU)
APICINTR(x2apic, 11, X2APIC_ICU)
APICINTR(x2apic, 12, X2APIC_ICU)
APICINTR(x2apic, 13, X2APIC_ICU)
APICINTR(x2apic, 14, X2APIC_ICU)
APICINTR(x2apic, 15, X2APIC_ICU)
APICINTR(x2apic, 16, X2APIC_ICU)
APICINTR(x2apic, 17, X2APIC_ICU)
APICINTR(x2apic, 18, X2APIC_ICU)
APICINTR(x2apic, 19, X2APIC_ICU)
APICINTR(x2apic, 20, X2APIC_ICU)
APICINTR(x2apic, 21, X2APIC_ICU)
APICINTR(x2apic, 22, X2APIC_ICU)
APICINTR(x2apic, 23, X2APIC_ICU)
APICINTR(x2apic, 24, X2APIC_ICU)
APICINTR(x2apic, 25, X2APIC_ICU)
APICINTR(x2apic, 26, X2APIC_ICU)
APICINTR(x2apic, 27, X2APIC_ICU)
APICINTR(x2apic, 28, X2APIC_ICU)
APICINTR(x2apic, 29, X2APIC_ICU)
APICINTR(x2apic, 30, X2APIC_ICU)
APICINTR(x2apic, 31, X2APIC_ICU)

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