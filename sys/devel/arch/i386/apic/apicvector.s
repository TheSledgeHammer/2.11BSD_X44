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

#define MY_COUNT 					_C_LABEL(cnt)
#define INTR_ADDR(intr, irq_num) 	((intr)+(irq_num) * 4)

#define	APICINTR(name, irq_num, icu, enable_icus)                        \
IDTVEC(recurse_/**/name/**/num)											;\
		pushfl															;\
		pushl	%cs														;\
		pushl	%esi													;\
		subl	$4,%esp													;\
		pushl	$T_ASTFLT				/* trap # for doing ASTs */		;\
		INTRENTRY														;\
IDTVEC(resume_/**/name/**/num)											;\
		movl	$IREENT_MAGIC,TF_ERR(%esp)								;\
		movl	%ebx,%esi												;\
		movl	INTR_ADDR(intrsource, irq_num), %ebp					;\
		movl	IS_MAXLEVEL(%ebp),%ebx									;\
		jmp		1f	
IDTVEC(intr_/**/name/**/num)                                        	;\


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
APICINTR(apic, 16, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 17, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 18, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 19, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 20, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 21, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 22, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 23, IO_ICU1, ENABLE_ICU1)
APICINTR(apic, 24, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 25, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 26, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 27, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 28, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 29, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 30, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(apic, 31, IO_ICU2, ENABLE_ICU1_AND_2)

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