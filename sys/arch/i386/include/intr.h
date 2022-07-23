/*	$NetBSD: intr.h,v 1.7 1997/03/21 04:34:18 mycroft Exp $	*/

/*
 * Copyright (c) 1996, 1997 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _I386_INTR_H_
#define _I386_INTR_H_

/* Interrupt priority `levels'. */
#define	IPL_NONE		9	/* nothing */
#define	IPL_SOFTCLOCK	8	/* timeouts */
#define	IPL_SOFTNET		7	/* protocol stacks */
#define	IPL_BIO			6	/* block I/O */
#define	IPL_SOFTBIO		IPL_BIO	/* block I/O passdown */
#define	IPL_NET			5	/* network */
#define	IPL_SOFTSERIAL	4	/* serial */
#define	IPL_TTY			3	/* terminal */
#define	IPL_IMP			3	/* memory allocation */
#define	IPL_AUDIO		2	/* audio */
#define	IPL_CLOCK		1	/* clock */
#define	IPL_HIGH		1	/* everything */
#define	IPL_SERIAL		0	/* serial */
#define IPL_IPI			0	/* inter-processor interrupts */
#define	NIPL			10

//#define	IPL_VM			6	/* low I/O, memory allocation */
//#define IPL_SCHED		7	/* medium I/O, scheduler, clock */

/* Interrupt sharing types. */
#define	IST_NONE		0	/* none */
#define	IST_PULSE		1	/* pulsed */
#define	IST_EDGE		2	/* edge-triggered */
#define	IST_LEVEL		3	/* level-triggered */

/*
 * Local APIC masks and software interrupt masks, in order
 * of priority.  Must not conflict with SIR_* below.
 */
#define LIR_IPI			31
#define LIR_TIMER		30

/* Soft interrupt masks. */
#define	SIR_CLOCK		29
#define	SIR_NET			28
#define	SIR_SERIAL		27
#define	SIR_BIO			26

#define IREENT_MAGIC	0x18041969

#define I386_NIPI		9

#ifndef _LOCORE

#include <i386/isa/icu.h>
/*
int 				imask[NIPL];
int 				iunmask[NIPL];
int 				intrtype[MAX_INTR_SOURCES];
int 				intrmask[MAX_INTR_SOURCES];
int 				intrlevel[MAX_INTR_SOURCES];
*/
extern void Xspllower(void);
extern int splraise(int);
extern int spllower(int);
extern void splx(int);
extern void softintr(int);

/*
 * Hardware interrupt masks
 */
#define	splbio()			splraise(imask[IPL_BIO])
#define	splnet()			splraise(imask[IPL_NET])
#define	spltty()			splraise(imask[IPL_TTY])
#define	splaudio()			splraise(imask[IPL_AUDIO])
#define	splclock()			splraise(imask[IPL_CLOCK])
#define	splstatclock()		splclock()
#define	splserial()			splraise(imask[IPL_SERIAL])
#define splipi()			splraise(IPL_IPI)

/*
 * Software interrupt masks
 *
 * NOTE: splsoftclock() is used by hardclock() to lower the priority from
 * clock to softclock before it calls softclock().
 */
#define	splsoftclock()		spllower(imask[IPL_SOFTCLOCK])
#define	splsoftnet()		splraise(imask[IPL_SOFTNET])
#define	splsoftserial()		splraise(imask[IPL_SOFTSERIAL])

/*
 * Miscellaneous
 */
#define	splimp()			splraise(imask[IPL_IMP])
#define	splhigh()			splraise(imask[IPL_HIGH])
#define	spl0()				spllower(0)

#define	setsoftast(p)		aston(p)
#define	setsoftclock()		softintr(SIR_CLOCK)
#define	setsoftnet()		softintr(SIR_NET)
#define	setsoftserial()		softintr(SIR_SERIAL)

#define I386_IPI_HALT		0x00000001
#define I386_IPI_MICROSET	0x00000002
#define I386_IPI_FLUSH_FPU	0x00000004
#define I386_IPI_SYNCH_FPU	0x00000008
#define I386_IPI_TLB		0x00000010
#define I386_IPI_MTRR		0x00000020
#define I386_IPI_GDT		0x00000040

struct cpu_info;

int 	i386_send_ipi(struct cpu_info *, int);
void	i386_self_ipi(int);
void 	i386_broadcast_ipi(int);
void 	i386_multicast_ipi(int, int);
void 	i386_ipi_handler(void);

#endif /* !_LOCORE */
#endif /* !_I386_INTR_H_ */
