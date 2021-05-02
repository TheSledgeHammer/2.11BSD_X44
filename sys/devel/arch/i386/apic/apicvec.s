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
		movl	CPUVAR(ILEVEL),%ebx
		cmpl	$IPL_IPI,%ebx
		jae		2f
1:
		incl	CPUVAR(IDEPTH)
		movl	$IPL_IPI,CPUVAR(ILEVEL)
        sti
		pushl	%ebx
		call	i386_ipi_handler
		jmp		doreti
2:
		orl		$(1 << LIR_IPI),CPUVAR(IPENDING)
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
		movl	CPUVAR(ILEVEL),%ebx
		cmpl	$IPL_HIGH,%ebx
		jae		2f
		jmp		1f

RESUME(lapic_ipi)

/* Interrupt from the local APIC TLB */
IDTVEC(lapic_tlb)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		call	pmap_tlb_intr
		movl	local_apic_va,%eax
		movl	$0,LAPIC_EOI(%eax)
		INTRFASTEXIT
		
IDTVEC(x2apic_tlb)
		pushl	$0
		pushl	$T_ASTFLT
		INTRENTRY
		call	pmap_tlb_intr
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
		movl	local_apic_va,%ebx
		movl	$0,LAPIC_EOI(%ebx)
		movl	CPUVAR(ILEVEL),%ebx
		cmpl	$IPL_CLOCK,%ebx
		jae		2f
1:
		incl	CPUVAR(IDEPTH)
		movl	$IPL_CLOCK, CPUVAR(ILEVEL)
		sti
		pushl	%ebx
		pushl	$0
		call	lapic_clockintr
		addl	$4,%esp		
		jmp		doreti
2:
		orl	$(1 << LIR_TIMER),CPUVAR(IPENDING)
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
		movl	CPUVAR(ILEVEL),%ebx
		cmpl	$IPL_CLOCK,%ebx
		jae		2f
		jmp		1f
				
RESUME(lapic_ltimer)

#endif /* NLAPIC > 0 */

IDTVEC(spurious)


#define	IRQ_BIT(irq_num)	    	(1 << ((irq_num) % 8))
#define	IRQ_BYTE(irq_num)	    	((irq_num) >> 3)

#define MY_COUNT 					_C_LABEL(cnt)
#define INTR_ADDR(intr, irq_num) 	((intr)+(irq_num) * 4)


#define voidop(num)

	/*
	 * I/O APIC interrupt.
	 * We sort out which one is which based on the value of
	 * the processor priority register.
	 *
	 * XXX use cmove when appropriate.
	 */
#define APICINTR(name, num, early_ack, late_ack, mask, unmask, level_mask) 	\
IDTVEC(recurse_/**/name/**/num)												;\
		pushfl																;\
		pushl	%cs															;\
		pushl	%esi														;\
		subl	$4,%esp														;\
		pushl	$T_ASTFLT			/* trap # for doing ASTs */				;\
		INTRENTRY															;\
IDTVEC(resume_/**/name/**/num)												\
		movl	$IREENT_MAGIC,TF_ERR(%esp)									;\
		movl	%ebx,%esi													;\
		movl	INTR_ADDR(intrsource, irq_num), %ebp						;\
		movl	IS_MAXLEVEL(%ebp),%ebx										;\
		jmp		1f															;\
IDTVEC(intr_/**/name/**/num)												\
		pushl	$0					/* dummy error code */					;\
		pushl	$T_ASTFLT			/* trap # for doing ASTs */				;\
		INTRENTRY															;\
		movl	INTR_ADDR(intrsource, irq_num), %ebp						;\
		mask(num)					/* mask it in hardware */				;\
		early_ack(num)				/* and allow other intrs */				;\
		testl	%ebp,%ebp													;\
		jz		9f					/* stray */								;\
		movl	IS_MAXLEVEL(%ebp),%ebx										;\
		movl	CPUVAR(ILEVEL),%esi											;\
		cmpl	%ebx,%esi													;\
		jae		10f					/* currently masked; hold it */			;\
		incl	MY_COUNT+V_INTR		/* statistical info */					;\
		sti																	;\
1:																			\
		pushl	%esi														;\
		movl	%ebx,CPUVAR(ILEVEL)											;\
		sti																	;\
		incl	CPUVAR(IDEPTH)												;\
		movl	IS_HANDLERS(%ebp),%ebx										;\
6:																			\
		movl	IH_LEVEL(%ebx),%edi											;\
		cmpl	%esi,%edi													;\
		jle		7f															;\
		pushl	IH_ARG(%ebx)												;\
		movl	IH_FUN(%ebx),%eax											;\
		movl	%edi,CPUVAR(ILEVEL)											;\
		movl	IH_NEXT(%ebx),%ebx	/* next handler in chain */				;\
		call	*%eax				/* call it */							;\
		addl	$4,%esp				/* toss the arg */						;\
		testl	%ebx,%ebx													;\
		jnz		6b															;\
5:																			\
		cli																	;\
		unmask(num)					/* unmask it in hardware */				;\
		late_ack(num)														;\
		sti																	;\
		jmp		Xdoreti				/* lower spl and do ASTs */				;\
7:																			\
		cli																	;\
		orl     $(1 << num),CPUVAR(IPENDING)								;\
		level_mask(num)														;\
		late_ack(num)														;\
		sti																	;\
		jmp		doreti				/* lower spl and do ASTs */				;\
10:																			\
		cli																	;\
		orl     $(1 << num),CPUVAR(IPENDING)								;\
		level_mask(num)														;\
		late_ack(num)														;\
		sti																	;\
		INTRFASTEXIT														;\
9:																			\
		unmask(num)															;\
		late_ack(num)														;\
		
#define	APIC_STRAY_INITIALIZE 	                                        	\
		xorl	%esi,%esi				/* nobody claimed it yet */
#define	APIC_STRAY_INTEGRATE 	                                       	 	\
		orl		%eax,%esi				/* maybe he claimed it */	
#define	APIC_STRAY_TEST 			                                    	\
		testl	%esi,%esi				/* no more handlers */				;\
		jz		APIC_STRAY(irq_num)		/* nobody claimed it */
		
APICINTR(apic, 0, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 1, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 2, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 3, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 4, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 5, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 6, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 7, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 8, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 9, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 10, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 11, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 12, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 13, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 14, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 15, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 16, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 17, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 18, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 19, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 20, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 21, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 22, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 23, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 24, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 25, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 26, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 27, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 28, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 29, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 30, voidop, ioapic_asm_ack, voidop, voidop, voidop)
APICINTR(apic, 31, voidop, ioapic_asm_ack, voidop, voidop, voidop)

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
APICINTR(x2apic, 16, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 17, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 18, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 19, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 20, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 21, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 22, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 23, IO_ICU1, ENABLE_ICU1)
APICINTR(x2apic, 24, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 25, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 26, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 27, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 28, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 29, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 30, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(x2apic, 31, IO_ICU2, ENABLE_ICU1_AND_2)
