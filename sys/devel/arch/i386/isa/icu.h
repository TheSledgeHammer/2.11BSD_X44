/*-
 * Copyright (c) 1990, 1993
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
 *	@(#)icu.h	8.1 (Berkeley) 6/11/93
 */

#ifndef	__I386_ICU__
#define	__I386_ICU__

#ifndef	LOCORE

/*
 * Interrupt "level" mechanism variables, masks, and macros
 */
extern unsigned short			imen;			/* interrupt mask enable */
extern unsigned short			cpl;			/* current priority level mask */
extern unsigned short			ipending;
extern unsigned short			ilevel;
extern unsigned short			idepth;

extern int 						intrtype[];
extern int 						intrmask[];
extern int 						intrlevel[];

#define	INTREN(s)				imen &= ~(s)
#define	INTRDIS(s)				imen |= (s)
#define	INTRMASK(msk,s)			msk |= (s)
#define INTRUNMASK(msk,s)		(msk &= ~(s))

#endif

#define SOFTINTR_MASK(ipl, sir)	((ipl) = (1 << (sir)))

/*
 * Interrupt enable bits -- in order of priority
 */
#define	IRQ0					0x0001		/* highest priority - timer */
#define	IRQ1					0x0002
#define	IRQ_SLAVE				0x0004
#define	IRQ8					0x0100
#define	IRQ9					0x0200
#define	IRQ2					IRQ9
#define	IRQ10					0x0400
#define	IRQ11					0x0800
#define	IRQ12					0x1000
#define	IRQ13					0x2000
#define	IRQ14					0x4000
#define	IRQ15					0x8000
#define	IRQ3					0x0008
#define	IRQ4					0x0010
#define	IRQ5					0x0020
#define	IRQ6					0x0040
#define	IRQ7					0x0080		/* lowest - parallel printer */

/*
 * Interrupt Control offset into Interrupt descriptor table (IDT)
 */
#define	ICU_OFFSET				32			/* 0-31 are processor exceptions */
#define	ICU_LEN					16			/* 32-47 are ISA interrupts */

#define MAX_INTR_SOURCES 		ICU_OFFSET
#define NUM_LEGACY_IRQS			ICU_LEN

#endif	__I386_ICU__
