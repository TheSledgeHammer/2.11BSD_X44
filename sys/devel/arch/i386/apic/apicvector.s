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

#define	IRQ_BIT(irq_num)	    	(1 << ((irq_num) % 8))
#define	IRQ_BYTE(irq_num)	    	((irq_num) >> 3)

#define	RECURSE(name)			\
IDTVEC(recurse_/**/name)		;\
		pushfl					;\
		pushl	%cs				;\
		pushl	%esi			;\
		pushl	$0				;\
		pushl	$T_ASTFLT		;\
		INTRENTRY				;\

#define	RESUME(name)			\
IDTVEC(resume_/**/name)			;\
		cli						;\
		jmp		1f				;\

#ifdef SMP
#if NLAPIC > 0

/* Interrupt from the local APIC IPI */
RECURSE(lapic_ipi)
IDTVEC(lapic_ipi)
		pushl	$0		
		pushl	$T_ASTFLT
		INTRENTRY		
		movl	$0,local_apic+LAPIC_EOI
		movl	_C_LABEL(ilevel),%ebx
		cmpl	$IPL_IPI,%ebx
		jae		2f
1:
		incl	_C_LABEL(idepth)
		movl	$IPL_IPI,_C_LABEL(ilevel)
        sti
		pushl	%ebx
		call	_C_LABEL(i386_ipi_handler)
		jmp		_C_LABEL(Xdoreti)
2:
		orl		$(1 << LIR_IPI),_C_LABEL(ipending)
		sti
		INTRFASTEXIT
		
IDTVEC(x2apic_ipi)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
		xorl	%eax,%eax
		xorl	%edx,%edx
		wrmsr
		movl	_C_LABEL(ilevel),%ebx
		cmpl	$IPL_HIGH,%ebx
		jae		2f
		jmp		1f

RESUME(lapic_ipi)

/* Interrupt from the local APIC TLB */
IDTVEC(lapic_tlb)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		call	invltlb
		movl	_C_LABEL_(local_apic_va),%eax
		movl	$0,LAPIC_EOI(%eax)
		INTRFASTEXIT
		
IDTVEC(x2apic_tlb)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		call	invltlb
		movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
		xorl	%eax,%eax
		xorl	%edx,%edx
		wrmsr
		INTRFASTEXIT

#endif

/* Interrupt from the local APIC timer. */
RECURSE(lapic_ltimer)

IDTVEC(lapic_ltimer)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		movl	_C_LABEL(local_apic_va),%ebx
		movl	$0,LAPIC_EOI(%ebx)
		movl	_C_LABEL(ilevel),%ebx
		cmpl	$IPL_CLOCK,%ebx
		jae		2f
1:
		incl	_C_LABEL(idepth)
		movl	$IPL_CLOCK, _C_LABEL(ilevel)
		sti
		pushl	%ebx
		pushl	$0
		call	_C_LABEL(lapic_clockintr)
		addl	$4,%esp		
		jmp		_C_LABEL(Xdoreti)
2:
		orl	$(1 << LIR_TIMER),_C_LABEL(ipending)
		sti
		INTRFASTEXIT
		
IDTVEC(x2apic_ltimer)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx
		xorl	%eax,%eax
		xorl	%edx,%edx
		wrmsr
		movl	_C_LABEL(ilevel),%ebx
		cmpl	$IPL_CLOCK,%ebx
		jae		2f
		jmp		1f
				
RESUME(lapic_ltimer)

#endif /* NLAPIC > 0 */

IDTVEC(spurious)
		ret


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


		.globl	_C_LABEL(apic_strayintr)

#define MY_COUNT 					_C_LABEL(cnt)
#define INTR_ADDR(intr, irq_num) 	(_C_LABEL(intr)+(irq_num) * 4)

#define	XAPICSTRAY(irq_num)			_Xapic_stray/**/irq_num

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
		movl	_C_LABEL(ilevel),%esi									;\
		cmpl	%ebx,%esi												;\
		jae		10f						/* currently masked; hold it */	;\
		incl	MY_COUNT+V_INTR			/* statistical info */			;\
		sti																;\
1:																		;\
		pushl	%esi													;\
		movl	%ebx,_C_LABEL(ilevel)									;\
		sti																;\
		incl	_C_LABEL(idepth)										;\
		movl	IS_HANDLERS(%ebp),%ebx									;\
6:																		;\
		movl	IH_LEVEL(%ebx),%edi										;\
		cmpl	%esi,%edi												;\
		jle		7f														;\
		pushl	IH_ARG(%ebx)											;\
		movl	IH_FUN(%ebx),%eax										;\
		movl	%edi,_C_LABEL(ilevel)									;\
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
		
IDTVEC(name_/**/stray/**/irq_num)										;\
		pushl	$num													;\
		call	_C_LABEL(apic_stray)									;\
		addl	$4,%esp													;\
		jmp		8b														;\		

#define	APIC_STRAY_INITIALIZE 	                                        \
		xorl	%esi,%esi				/* nobody claimed it yet */
#define	APIC_STRAY_INTEGRATE 	                                        \
		orl		%eax,%esi				/* maybe he claimed it */	
#define	APIC_STRAY_TEST 			                                    \
		testl	%esi,%esi				/* no more handlers */			;\
		jz		XAPICSTRAY(irq_num)		/* nobody claimed it */

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

IDTVEC(apic_intr)
		.long	_C_LABEL(Xapic_intr0), _C_LABEL(Xapic_intr1)
		.long	_C_LABEL(Xapic_intr2), _C_LABEL(Xapic_intr3)
		.long	_C_LABEL(Xapic_intr4), _C_LABEL(Xapic_intr5)
		.long	_C_LABEL(Xapic_intr6), _C_LABEL(Xapic_intr7)
		.long	_C_LABEL(Xapic_intr8), _C_LABEL(Xapic_intr9)
		.long	_C_LABEL(Xapic_intr10), _C_LABEL(Xapic_intr11)
		.long	_C_LABEL(Xapic_intr12), _C_LABEL(Xapic_intr13)
		.long	_C_LABEL(Xapic_intr14), _C_LABEL(Xapic_intr15)
		.long	_C_LABEL(Xapic_intr16), _C_LABEL(Xapic_intr17)
		.long	_C_LABEL(Xapic_intr18), _C_LABEL(Xapic_intr19)
		.long	_C_LABEL(Xapic_intr20), _C_LABEL(Xapic_intr21)
		.long	_C_LABEL(Xapic_intr22), _C_LABEL(Xapic_intr23)
		.long	_C_LABEL(Xapic_intr24), _C_LABEL(Xapic_intr25)
		.long	_C_LABEL(Xapic_intr26), _C_LABEL(Xapic_intr27)
		.long	_C_LABEL(Xapic_intr28), _C_LABEL(Xapic_intr29)
		.long	_C_LABEL(Xapic_intr30), _C_LABEL(Xapic_intr31)

IDTVEC(apic_resume)
		.long	_C_LABEL(Xapic_resume0), _C_LABEL(Xapic_resume1)
		.long	_C_LABEL(Xapic_resume2), _C_LABEL(Xapic_resume3)
		.long	_C_LABEL(Xapic_resume4), _C_LABEL(Xapic_resume5)
		.long	_C_LABEL(Xapic_resume6), _C_LABEL(Xapic_resume7)
		.long	_C_LABEL(Xapic_resume8), _C_LABEL(Xapic_resume9)
		.long	_C_LABEL(Xapic_resume10), _C_LABEL(Xapic_resume11)
		.long	_C_LABEL(Xapic_resume12), _C_LABEL(Xapic_resume13)
		.long	_C_LABEL(Xapic_resume14), _C_LABEL(Xapic_resume15)
		.long	_C_LABEL(Xapic_resume16), _C_LABEL(Xapic_resume17)
		.long	_C_LABEL(Xapic_resume18), _C_LABEL(Xapic_resume19)
		.long	_C_LABEL(Xapic_resume20), _C_LABEL(Xapic_resume21)
		.long	_C_LABEL(Xapic_resume22), _C_LABEL(Xapic_resume23)
		.long	_C_LABEL(Xapic_resume24), _C_LABEL(Xapic_resume25)
		.long	_C_LABEL(Xapic_resume26), _C_LABEL(Xapic_resume27)
		.long	_C_LABEL(Xapic_resume28), _C_LABEL(Xapic_resume29)
		.long	_C_LABEL(Xapic_resume30), _C_LABEL(Xapic_resume31)

IDTVEC(apic_recurse)
		.long	_C_LABEL(Xapic_recurse0), _C_LABEL(Xapic_recurse1)
		.long	_C_LABEL(Xapic_recurse2), _C_LABEL(Xapic_recurse3)
		.long	_C_LABEL(Xapic_recurse4), _C_LABEL(Xapic_recurse5)
		.long	_C_LABEL(Xapic_recurse6), _C_LABEL(Xapic_recurse7)
		.long	_C_LABEL(Xapic_recurse8), _C_LABEL(Xapic_recurse9)
		.long	_C_LABEL(Xapic_recurse10), _C_LABEL(Xapic_recurse11)
		.long	_C_LABEL(Xapic_recurse12), _C_LABEL(Xapic_recurse13)
		.long	_C_LABEL(Xapic_recurse14), _C_LABEL(Xapic_recurse15)
		.long	_C_LABEL(Xapic_recurse16), _C_LABEL(Xapic_recurse17)
		.long	_C_LABEL(Xapic_recurse18), _C_LABEL(Xapic_recurse19)
		.long	_C_LABEL(Xapic_recurse20), _C_LABEL(Xapic_recurse21)
		.long	_C_LABEL(Xapic_recurse22), _C_LABEL(Xapic_recurse23)
		.long	_C_LABEL(Xapic_recurse24), _C_LABEL(Xapic_recurse25)
		.long	_C_LABEL(Xapic_recurse26), _C_LABEL(Xapic_recurse27)
		.long	_C_LABEL(Xapic_recurse28), _C_LABEL(Xapic_recurse29)
		.long	_C_LABEL(Xapic_recurse30), _C_LABEL(Xapic_recurse31)

IDTVEC(x2apic_intr)
		.long	_C_LABEL(Xx2apic_intr0), _C_LABEL(Xx2apic_intr1)
		.long	_C_LABEL(Xx2apic_intr2), _C_LABEL(Xx2apic_intr3)
		.long	_C_LABEL(Xx2apic_intr4), _C_LABEL(Xx2apic_intr5)
		.long	_C_LABEL(Xx2apic_intr6), _C_LABEL(Xx2apic_intr7)
		.long	_C_LABEL(Xx2apic_intr8), _C_LABEL(Xx2apic_intr9)
		.long	_C_LABEL(Xx2apic_intr10), _C_LABEL(Xx2apic_intr11)
		.long	_C_LABEL(Xx2apic_intr12), _C_LABEL(Xx2apic_intr13)
		.long	_C_LABEL(Xx2apic_intr14), _C_LABEL(Xx2apic_intr15)
		.long	_C_LABEL(Xx2apic_intr16), _C_LABEL(Xx2apic_intr17)
		.long	_C_LABEL(Xx2apic_intr18), _C_LABEL(Xx2apic_intr19)
		.long	_C_LABEL(Xx2apic_intr20), _C_LABEL(Xx2apic_intr21)
		.long	_C_LABEL(Xx2apic_intr22), _C_LABEL(Xx2apic_intr23)
		.long	_C_LABEL(Xx2apic_intr24), _C_LABEL(Xx2apic_intr25)
		.long	_C_LABEL(Xx2apic_intr26), _C_LABEL(Xx2apic_intr27)
		.long	_C_LABEL(Xx2apic_intr28), _C_LABEL(Xx2apic_intr29)
		.long	_C_LABEL(Xx2apic_intr30), _C_LABEL(Xx2apic_intr31)

IDTVEC(x2apic_resume)
		.long	_C_LABEL(Xx2apic_resume0), _C_LABEL(Xx2apic_resume1)
		.long	_C_LABEL(Xx2apic_resume2), _C_LABEL(Xx2apic_resume3)
		.long	_C_LABEL(Xx2apic_resume4), _C_LABEL(Xx2apic_resume5)
		.long	_C_LABEL(Xx2apic_resume6), _C_LABEL(Xx2apic_resume7)
		.long	_C_LABEL(Xx2apic_resume8), _C_LABEL(Xx2apic_resume9)
		.long	_C_LABEL(Xx2apic_resume10), _C_LABEL(Xx2apic_resume11)
		.long	_C_LABEL(Xx2apic_resume12), _C_LABEL(Xx2apic_resume13)
		.long	_C_LABEL(Xx2apic_resume14), _C_LABEL(Xx2apic_resume15)
		.long	_C_LABEL(Xx2apic_resume16), _C_LABEL(Xx2apic_resume17)
		.long	_C_LABEL(Xx2apic_resume18), _C_LABEL(Xx2apic_resume19)
		.long	_C_LABEL(Xx2apic_resume20), _C_LABEL(Xx2apic_resume21)
		.long	_C_LABEL(Xx2apic_resume22), _C_LABEL(Xx2apic_resume23)
		.long	_C_LABEL(Xx2apic_resume24), _C_LABEL(Xx2apic_resume25)
		.long	_C_LABEL(Xx2apic_resume26), _C_LABEL(Xx2apic_resume27)
		.long	_C_LABEL(Xx2apic_resume28), _C_LABEL(Xx2apic_resume29)
		.long	_C_LABEL(Xx2apic_resume30), _C_LABEL(Xx2apic_resume31)

IDTVEC(x2apic_recurse)
		.long	_C_LABEL(Xx2apic_recurse0), _C_LABEL(Xx2apic_recurse1)
		.long	_C_LABEL(Xx2apic_recurse2), _C_LABEL(Xx2apic_recurse3)
		.long	_C_LABEL(Xx2apic_recurse4), _C_LABEL(Xx2apic_recurse5)
		.long	_C_LABEL(Xx2apic_recurse6), _C_LABEL(Xx2apic_recurse7)
		.long	_C_LABEL(Xx2apic_recurse8), _C_LABEL(Xx2apic_recurse9)
		.long	_C_LABEL(Xx2apic_recurse10), _C_LABEL(Xx2apic_recurse11)
		.long	_C_LABEL(Xx2apic_recurse12), _C_LABEL(Xx2apic_recurse13)
		.long	_C_LABEL(Xx2apic_recurse14), _C_LABEL(Xx2apic_recurse15)
		.long	_C_LABEL(Xx2apic_recurse16), _C_LABEL(Xx2apic_recurse17)
		.long	_C_LABEL(Xx2apic_recurse18), _C_LABEL(Xx2apic_recurse19)
		.long	_C_LABEL(Xx2apic_recurse20), _C_LABEL(Xx2apic_recurse21)
		.long	_C_LABEL(Xx2apic_recurse22), _C_LABEL(Xx2apic_recurse23)
		.long	_C_LABEL(Xx2apic_recurse24), _C_LABEL(Xx2apic_recurse25)
		.long	_C_LABEL(Xx2apic_recurse26), _C_LABEL(Xx2apic_recurse27)
		.long	_C_LABEL(Xx2apic_recurse28), _C_LABEL(Xx2apic_recurse29)
		.long	_C_LABEL(Xx2apic_recurse30), _C_LABEL(Xx2apic_recurse31)

/* Some bogus data, to keep vmstat happy, for now. */
		.globl	_C_LABEL(apicintrnames), _C_LABEL(x2apicintrnames)
		.globl	_C_LABEL(apicintrcnt), _C_LABEL(x2apicintrcnt)
		
		/* Names */
_C_LABEL(apicintrnames):
		.asciz	"apic0", "apic1", "apic2", "apic3"
		.asciz	"apic4", "apic5", "apic6", "apic7"
		.asciz	"apic8", "apic9", "apic10", "apic11"
		.asciz	"apic12", "apic13", "apic14", "apic15"
		.asciz	"apic16", "apic17", "apic18", "apic19"
		.asciz	"apic20", "apic21", "apic22", "apic23"
		.asciz	"apic24", "apic25", "apic26", "apic27"
		.asciz	"apic28", "apic29", "apic30", "apic31"
_C_LABEL(apicstrayintrnames):
		.asciz	"apicstray0", "apicstray1", "apicstray2", "apicstray3"
		.asciz	"apicstray4", "apicstray5", "apicstray6", "apicstray7"
		.asciz	"apicstray8", "apicstray9", "apicstray10", "apicstray11"
		.asciz	"apicstray12", "apicstray13", "apicstray14", "apicstray15"
		.asciz	"apicstray16", "apicstray17", "apicstray18", "apicstray19"
		.asciz	"apicstray20", "apicstray21", "apicstray22", "apicstray23"
		.asciz	"apicstray24", "apicstray25", "apicstray26", "apicstray27"
		.asciz	"apicstray28", "apicstray29", "apicstray30", "apicstray31"
_C_LABEL(x2apicintrnames):
		.asciz	"x2apic0", "x2apic1", "x2apic2", "x2apic3"
		.asciz	"x2apic4", "x2apic5", "x2apic6", "x2apic7"
		.asciz	"x2apic8", "x2apic9", "x2apic10", "x2apic11"
		.asciz	"x2apic12", "x2apic13", "x2apic14", "x2apic15"
		.asciz	"x2apic16", "x2apic17", "x2apic18", "x2apic19"
		.asciz	"x2apic20", "x2apic21", "x2apic22", "x2apic23"
		.asciz	"x2apic24", "x2apic25", "x2apic26", "x2apic27"
		.asciz	"x2apic28", "x2apic29", "x2apic30", "x2apic31"
_C_LABEL(x2apicstrayintrnames):
		.asciz	"x2apicstray0", "x2apicstray1", "x2apicstray2", "x2apicstray3"
		.asciz	"x2apicstray4", "x2apicstray5", "x2apicstray6", "x2apicstray7"
		.asciz	"x2apicstray8", "x2apicstray9", "x2apicstray10", "x2apicstray11"
		.asciz	"x2apicstray12", "x2apicstray13", "x2apicstray14", "x2apicstray15"
		.asciz	"x2apicstray16", "x2apicstray17", "x2apicstray18", "x2apicstray19"
		.asciz	"x2apicstray20", "x2apicstray21", "x2apicstray22", "x2apicstray23"
		.asciz	"x2apicstray24", "x2apicstray25", "x2apicstray26", "x2apicstray27"
		.asciz	"x2apicstray28", "x2apicstray29", "x2apicstray30", "x2apicstray31"
		
		/* And counters */
		.data
		.align 4
_C_LABEL(apicintrcnt):
		.long	0, 0, 0, 0, 0, 0, 0, 0
		.long	0, 0, 0, 0, 0, 0, 0, 0
		.long	0, 0, 0, 0, 0, 0, 0, 0
		.long	0, 0, 0, 0, 0, 0, 0, 0
_C_LABEL(x2apicintrcnt):
		.long	0, 0, 0, 0, 0, 0, 0, 0
		.long	0, 0, 0, 0, 0, 0, 0, 0
		.long	0, 0, 0, 0, 0, 0, 0, 0
		.long	0, 0, 0, 0, 0, 0, 0, 0
#endif

ENTRY(lapic_eoi)
		cmpl	$0,_C_LABEL(x2apic_mode)											
		jne		1f														
		movl	_C_LABEL(local_apic_va),%eax							
		movl	$0,LAPIC_EOI(%eax)										
1:																		
		movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx 				
		xorl	%eax,%eax												
		xorl	%edx,%edx												
		wrmsr
		ret
		
#ifdef SMP

/*
 * Global address space TLB shootdown.
 */
		.text
invltlb_ret:
		call	lapic_eoi
		jmp		_C_LABEL(Xdoreti)
		
IDTVEC(invltlb)
		PUSH_FRAME
		SET_KERNEL_SREGS
		cld
		KENTER
		movl	$invltlb_handler, %eax
		call	*%eax
		jmp		invltlb_ret
		
/*
 * Single page TLB shootdown
 */
		.text
		SUPERALIGN_TEXT
IDTVEC(invlpg)
		PUSH_FRAME
		SET_KERNEL_SREGS
		cld
		KENTER
		movl	$invlpg_handler, %eax
		call	*%eax
		jmp		invltlb_ret

/*
 * Page range TLB shootdown.
 */
		.text
		SUPERALIGN_TEXT
IDTVEC(invlrng)
		PUSH_FRAME
		SET_KERNEL_SREGS
		cld
		KENTER
		movl	$invlrng_handler, %eax
		call	*%eax
		jmp		invltlb_ret

/*
 * Invalidate cache.
 */
		.text
		SUPERALIGN_TEXT
IDTVEC(invlcache)
		PUSH_FRAME
		SET_KERNEL_SREGS
		cld
		KENTER
		movl	$invlcache_handler, %eax
		call	*%eax
		jmp		invltlb_ret
		
#endif /* SMP */