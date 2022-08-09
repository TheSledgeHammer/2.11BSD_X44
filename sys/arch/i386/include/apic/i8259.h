/*	$NetBSD: i8259.h,v 1.4 2003/08/07 16:30:32 agc Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)icu.h	5.6 (Berkeley) 5/9/91
 */

#ifndef _I386_I8259_H_
#define _I386_I8259_H_

#include <dev/core/isa/isareg.h>

#ifndef	_LOCORE
/*
 * Interrupt "level" mechanism variables, masks, and macros
 */
extern unsigned i8259_imen;		/* interrupt mask enable */
extern unsigned i8259_setmask(unsigned);

#define SET_ICUS()	(outb(IO_ICU1 + 1, imen), outb(IO_ICU2 + 1, imen >> 8))

extern void i8259_default_setup(void);

#endif /* !_LOCORE */
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

#define i8259_late_icu(num)

#ifdef ICU_SPECIAL_MASK_MODE

#define	ENABLE_ICU1(irq_num)
#define	ENABLE_ICU2(irq_num) 											 	 \
		movb	$(0x60|IRQ_SLAVE),%al			/* specific EOI for IRQ2 */	;\
		outb	%al,$IO_ICU1
#define	LEGACY_MASK(irq_num)
#define	LEGACY_UNMASK(irq_num) 												 \
		movb	$(0x60|(irq_num%8)),%al			/* specific EOI */			;\
		outb	%al,$ICUADDR

#else /* ICU_SPECIAL_MASK_MODE */

#ifndef	AUTO_EOI_1
#define	ENABLE_ICU1(irq_num) 												 \
		movb	$(0x60|(irq_num%8)),%al			/* specific EOI */			;\
		outb	%al,$IO_ICU1
#else
#define	ENABLE_ICU1(irq_num)
#endif

#ifndef AUTO_EOI_2
#define	ENABLE_ICU2(irq_num) 											 	 \
		movb	$(0x60|(irq_num%8)),%al		/* specific EOI */				;\
		outb	%al,$IO_ICU2				/* do the second ICU first */	;\
		movb	$(0x60|IRQ_SLAVE),%al		/* specific EOI for IRQ2 */		;\
		outb	%al,$IO_ICU1
#else
#define	ENABLE_ICU2(irq_num)
#endif

#ifdef PIC_MASKDELAY
#define	FASTER_NOP		pushl %eax; inb $0x84,%al; popl %eax
#else
#define FASTER_NOP
#endif

#ifdef ICU_HARDWARE_MASK

#define	LEGACY_MASK(irq_num) 												 \
		movb	CVAROFF(i8259_imen, IRQ_BYTE(irq_num)),%al					;\
		orb		$IRQ_BIT(irq_num),%al										;\
		movb	%al,CVAROFF(i8259_imen, IRQ_BYTE(irq_num))					;\
		FASTER_NOP															;\
		outb	%al,$(ICUADDR+1)

#define	LEGACY_UNMASK(irq_num) 										 		 \
		movb	CVAROFF(i8259_imen, IRQ_BYTE(irq_num)),%al					;\
		andb	$~IRQ_BIT(irq_num),%al										;\
		movb	%al,CVAROFF(i8259_imen, IRQ_BYTE(irq_num))					;\
		FASTER_NOP															;\
		outb	%al,$(ICUADDR+1)

#else /* ICU_HARDWARE_MASK */

#define	LEGACY_MASK(irq_num)
#define	LEGACY_UNMASK(irq_num)

#endif /* ICU_HARDWARE_MASK */
#endif /* ICU_SPECIAL_MASK_MODE */

#endif /* !_I386_I8259_H_ */
