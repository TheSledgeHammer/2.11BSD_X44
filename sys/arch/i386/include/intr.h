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

#ifndef _I386_INTR_H_
#define _I386_INTR_H_

/* Interrupt priority `levels'. */
#define	IPL_NONE			9	/* nothing */
#define	IPL_SOFTCLOCK		8	/* timeouts */
#define	IPL_SOFTNET			7	/* protocol stacks */
#define	IPL_BIO				6	/* block I/O */
#define	IPL_SOFTBIO			IPL_BIO	/* block I/O passdown */
#define	IPL_NET				5	/* network */
#define	IPL_SOFTSERIAL		4	/* serial */
#define	IPL_TTY				3	/* terminal */
#define	IPL_IMP				3	/* memory allocation */
#define	IPL_VM				IPL_IMP	/* memory allocation */
#define	IPL_AUDIO			2	/* audio */
#define	IPL_CLOCK			1	/* clock */
#define	IPL_HIGH			1	/* everything */
#define	IPL_SERIAL			0	/* serial */
#define IPL_IPI				0	/* inter-processor interrupts */
#define	NIPL				10

/* Interrupt sharing types. */
#define	IST_NONE			0	/* none */
#define	IST_PULSE			1	/* pulsed */
#define	IST_EDGE			2	/* edge-triggered */
#define	IST_LEVEL			3	/* level-triggered */

/*
 * Local APIC masks and software interrupt masks, in order
 * of priority.  Must not conflict with SIR_* below.
 */
#define LIR_IPI				31
#define LIR_TIMER			30

/* Soft interrupt masks. */
#define	SIR_CLOCK			29
#define	SIR_NET				28
#define	SIR_SERIAL			27
#define	SIR_BIO				26

#define IREENT_MAGIC		0x18041969

#define I386_NIPI			9

/*
 * Interrupt enable bits -- in order of priority
 */
#define	IRQ0				0x0001		/* highest priority - timer */
#define	IRQ1				0x0002
#define	IRQ_SLAVE			0x0004
#define	IRQ8				0x0100
#define	IRQ9				0x0200
#define	IRQ2				IRQ9
#define	IRQ10				0x0400
#define	IRQ11				0x0800
#define	IRQ12				0x1000
#define	IRQ13				0x2000
#define	IRQ14				0x4000
#define	IRQ15				0x8000
#define	IRQ3				0x0008
#define	IRQ4				0x0010
#define	IRQ5				0x0020
#define	IRQ6				0x0040
#define	IRQ7				0x0080		/* lowest - parallel printer */

/*
 * Interrupt Control offset into Interrupt descriptor table (IDT)
 */
#define	ICU_OFFSET			32			/* 0-31 are processor exceptions */
#define	ICU_LEN				16			/* 32-47 are ISA interrupts */
#define MAX_INTR_SOURCES 	ICU_OFFSET
#define NUM_LEGACY_IRQS		ICU_LEN

#define	LEGAL_IRQ(x)		((x) >= 0 && (x) < ICU_LEN && (x) != 2)

#ifndef _LOCORE

/*
 * Interrupt "level" mechanism variables, masks, and macros
 */
extern volatile int		imen;		/* interrupt mask enable */
extern volatile int		cpl;		/* current priority level mask */
extern volatile int		ipending;
extern volatile int		idepth;
extern int 				imask[];
extern int 				iunmask[];
extern int 				intrtype[];
extern int 				intrmask[];
extern int 				intrlevel[];

/*
 * Convert spl level to local APIC level
 */
#define APIC_LEVEL(l)   	((l) << 4)

/*
 * Low and high boundaries between which interrupt gates will
 * be allocated in the IDT.
 */
#define IDT_INTR_LOW	 	(0x20 + NUM_LEGACY_IRQS)
#define IDT_INTR_HIGH	 	0xef

/*
 * Hardware interrupt masks
 */
void Xspllower(void);

extern int splraise(int);
extern int spllower(int);
extern void splx(int);
extern void softintr(int);

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
#define	splsoftbio()		splraise(imask[IPL_SOFTBIO])

/*
 * Miscellaneous
 */
#define	splvm()				splraise(imask[IPL_VM])
#define	splimp()			splraise(imask[IPL_IMP])
#define	splhigh()			splraise(imask[IPL_HIGH])
#define	spl0()				spllower(0)
#define spllock() 			splhigh()

/* set software interrupts: compatibility */
#define setsoftintr(mask)   (softintr_set(NULL, (mask)))

#define	setsoftast(p)		aston(p)
#define	setsoftclock()		setsoftintr(I386_SOFTINTR_SOFTCLOCK) 	/* SIR_CLOCK */
#define	setsoftnet()		setsoftintr(I386_SOFTINTR_SOFTNET)		/* SIR_NET */
#define	setsoftserial()		setsoftintr(I386_SOFTINTR_SOFTSERIAL)	/* SIR_SERIAL */
#define setsoftbio()		setsoftintr(I386_SOFTINTR_SOFTBIO)		/* SIR_BIO */

#define I386_IPI_HALT		0x00000001
#define I386_IPI_MICROSET	0x00000002
#define I386_IPI_FLUSH_FPU	0x00000004
#define I386_IPI_SYNCH_FPU	0x00000008
#define I386_IPI_TLB		0x00000010
#define I386_IPI_MTRR		0x00000020
#define I386_IPI_GDT		0x00000040

extern void Xsoftclock(void);
extern void Xsoftnet(void);
extern void Xsoftserial(void);

struct cpu_info;

extern int (*i386_ipi)(int, int, int);
int 	i386_ipi_init(int);
int 	i386_ipi_startup(int, int);
int 	i386_send_ipi(struct cpu_info *, int);
void	i386_self_ipi(int);
void 	i386_broadcast_ipi(int);
void 	i386_multicast_ipi(int, int);
void 	i386_ipi_handler(void);

extern void (*ipifunc[I386_NIPI])(struct cpu_info *);

#endif /* !_LOCORE */

/*
 * Generic software interrupt support.
 */
#define	I386_SOFTINTR_SOFTBIO	        0
#define	I386_SOFTINTR_SOFTCLOCK		1
#define	I386_SOFTINTR_SOFTNET		2
#define	I386_SOFTINTR_SOFTSERIAL	3
#define	I386_NSOFTINTR			4
#define I386_NOINTERRUPT 			-1

#ifndef _LOCORE
#include <sys/queue.h>

struct i386_soft_intrhand {
	TAILQ_ENTRY(i386_soft_intrhand) 	sih_q;
	struct i386_soft_intr 				*sih_intrhead;
	void								(*sih_fn)(void *);
	void								*sih_arg;
	int									sih_pending;
};

struct i386_soft_intr {
	TAILQ_HEAD(, i386_soft_intrhand) 	softintr_q;
	int 								softintr_ssir;
	struct lock_object					*softintr_slock;
};

#define	i386_softintr_lock(si, s)			\
do {										\
	(s) = splhigh();						\
	simple_lock(si->softintr_slock);		\
} while (/*CONSTCOND*/ 0)

#define	i386_softintr_unlock(si, s)			\
do {										\
	simple_unlock(si->softintr_slock);		\
	splx((s));								\
} while (/*CONSTCOND*/ 0)

void	*softintr_establish(int, void (*)(void *), void *);
void	softintr_disestablish(void *);
void	softintr_init(void);
void	softintr_dispatch(int);
void 	softintr_set(void *, int);

#define	softintr_schedule(arg)								\
do {														\
	struct i386_soft_intrhand *__sih = (arg);				\
	struct i386_soft_intr *__si = __sih->sih_intrhead;		\
	int __s;												\
															\
	i386_softintr_lock(__si, __s);							\
	if (__sih->sih_pending == 0) {							\
		TAILQ_INSERT_TAIL(&__si->softintr_q, __sih, sih_q);	\
		__sih->sih_pending = 1;								\
		softintr(__si->softintr_ssir);						\
	}														\
	i386_softintr_unlock(__si, __s);						\
} while (/*CONSTCOND*/ 0)

#endif /* _LOCORE */
#endif /* !_I386_INTR_H_ */
