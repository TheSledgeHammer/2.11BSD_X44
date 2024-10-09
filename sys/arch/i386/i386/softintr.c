/*	$NetBSD: softintr.c,v 1.1 2003/02/26 21:26:12 fvdl Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Generic soft interrupt implementation for 211BSD/i386 & x86.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
//#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <machine/pic.h>
#include <machine/intr.h>

#include <vm/include/vm_extern.h>

static void softintr_register_pic(struct pic *, struct apic *);

/*
 * Generic software interrupt support.
 */
struct pic softintr_pic_template = {
		.pic_type = PIC_SOFT,
		.pic_hwmask = NULL,
		.pic_hwunmask = NULL,
		.pic_addroute = NULL,
		.pic_delroute = NULL,
		.pic_register = softintr_register_pic
};

struct apic softintr_apic_template = {
		.apic_pic_type = PIC_SOFT,
		.apic_edge = NULL,
		.apic_level = NULL
};

static void
softintr_register_pic(pic, apic)
	struct pic *pic;
	struct apic *apic;
{
	pic = &softintr_pic_template;
	apic = &softintr_apic_template;
	softpic_register(pic, apic);
}

struct i386_soft_intr i386_soft_intrs[I386_NSOFTINTR];
const int i386_soft_intr_to_ssir[I386_NSOFTINTR] = {
		SIR_CLOCK,
		SIR_NET,
		SIR_SERIAL,
		SIR_BIO,
};

/*
 * softintr_init:
 *
 *	Initialize the software interrupt system.
 */
void
softintr_init(void)
{
	struct i386_soft_intr *si;
	int i;

	for (i = 0; i < I386_NSOFTINTR; i++) {
		si = &i386_soft_intrs[i];
		TAILQ_INIT(&si->softintr_q);
		simple_lock_init(si->softintr_slock, "softintr_slock");
		si->softintr_ssir = i386_soft_intr_to_ssir[i];
	}
}

/*
 * softintr_dispatch:
 *
 *	Process pending software interrupts.
 */
void
softintr_dispatch(which)
	int which;
{
	struct i386_soft_intr 		*si;
	struct i386_soft_intrhand 	*sih;
	int s;

	si = &i386_soft_intrs[which];
	for (;;) {
		i386_softintr_lock(si, s);
		sih = TAILQ_FIRST(&si->softintr_q);
		if (sih == NULL) {
			i386_softintr_unlock(si, s);
			break;
		}
		TAILQ_REMOVE(&si->softintr_q, sih, sih_q);
		sih->sih_pending = 0;
		i386_softintr_unlock(si, s);

		(*sih->sih_fn)(sih->sih_arg);
	}
}

/*
 * softintr_establish:		[interface]
 *
 *	Register a software interrupt handler.
 */
void *
softintr_establish(level, func, arg)
	int level;
	void (*func)(void *);
	void *arg;
{
	struct i386_soft_intr 		*si;
	struct i386_soft_intrhand 	*sih;
	int which;

	which = (int)intrselect(level);
	if (which == I386_NOINTERRUPT) {
		panic("softintr_establish: no interrupt");
	}
	si = &i386_soft_intrs[which];
	sih = malloc(sizeof(*sih), M_DEVBUF, M_NOWAIT);
	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = si;
		sih->sih_fn = func;
		sih->sih_arg = arg;
		sih->sih_pending = 0;
	}
	return (sih);
}

/*
 * softintr_disestablish:	[interface]
 *
 *	Unregister a software interrupt handler.
 */
void
softintr_disestablish(void *arg)
{
	struct i386_soft_intrhand *sih;
	struct i386_soft_intr *si;
	int s;

	sih = arg;
	si = sih->sih_intrhead;
	i386_softintr_lock(si, s);
	if (sih->sih_pending) {
		TAILQ_REMOVE(&si->softintr_q, sih, sih_q);
		sih->sih_pending = 0;
	}
	i386_softintr_unlock(si, s);

	free(sih, M_DEVBUF);
}

/*
 * softintr_set:	[interface]
 *  Software interrupt update.
 */
void
softintr_set(arg, which)
	void *arg;
	int which;
{
	struct i386_soft_intr *si;
	int sir;

	si = (struct i386_soft_intr *)arg;
	if (si == NULL) {
		switch (which) {
		case I386_SOFTINTR_SOFTBIO:
			sir = SIR_BIO;
			break;
		case I386_SOFTINTR_SOFTCLOCK:
			sir = SIR_CLOCK;
			break;
		case I386_SOFTINTR_SOFTSERIAL:
			sir = SIR_SERIAL;
			break;
		case I386_SOFTINTR_SOFTNET:
			sir = SIR_NET;
			break;
		default:
			sir = I386_NOINTERRUPT;
			panic("softintr_set: no software interrupt");
		}
	} else {
		sir = which;
	}
	softintr(sir);
}

/*
void
softintr_schedule(arg)
	void *arg;
{
	struct i386_soft_intrhand *sih;
	struct i386_soft_intr *si;
	int s;

	sih = (struct i386_soft_intrhand *)arg;
	si = sih->sih_intrhead;
	i386_softintr_lock(si, s);
	if (sih->sih_pending == 0) {
		TAILQ_INSERT_TAIL(&si->softintr_q, sih, sih_q);
		sih->sih_pending = 1;
		softintr_set(si, si->softintr_ssir);
	}
	i386_softintr_unlock(si, s);
}
*/
