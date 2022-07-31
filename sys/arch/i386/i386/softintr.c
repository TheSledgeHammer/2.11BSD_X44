/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/user.h>

#include <machine/pic.h>
#include <machine/intr.h>

#include <vm/include/vm_extern.h>

/*
 * Generic software interrupt support.
 */
static void	softintr_hwmask(struct softpic *, int);
static void	softintr_hwunmask(struct softpic *, int);
static void	softintr_addroute(struct softpic *, struct cpu_info *, int, int, int);
static void	softintr_delroute(struct softpic *, struct cpu_info *, int, int, int);
static void	softintr_register_pic(struct pic *, struct apic *);

struct pic softintr_template = {
		.pic_type = PIC_SOFT,
		.pic_hwmask = softintr_hwmask,
		.pic_hwunmask = softintr_hwunmask,
		.pic_addroute = softintr_addroute,
		.pic_delroute = softintr_delroute,
		.pic_register = softintr_register_pic
};

static void
softintr_hwmask(spic, pin)
	struct softpic *spic;
	int pin;
{
	/* Do Nothing */
}

static void
softintr_hwunmask(spic, pin)
	struct softpic *spic;
	int pin;
{
	/* Do Nothing */
}

static void
softintr_addroute(spic, ci, pin, idtvec, type)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, idtvec, type;
{
	/* Do Nothing */
}


static void
softintr_delroute(spic, ci, pin, idtvec, type)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, idtvec, type;
{
	/* Do Nothing */
}

static void
softintr_register_pic(pic, apic)
	struct pic *pic;
	struct apic *apic;
{
	pic = &softintr_template;
	apic = NULL;
	softpic_register(pic, apic);
}

/* XXX: incomplete */
#if defined(__HAVE_FAST_SOFTINTS)
struct intrhand fake_softclock_intrhand;
struct intrhand fake_softnet_intrhand;
struct intrhand fake_softserial_intrhand;
struct intrhand fake_softbio_intrhand;

void
softint_init_md(struct proc *p, u_int level, uintptr_t *machdep)
{
	struct softpic *spic;
	struct intrsource *isp;
	struct cpu_info *ci;
	u_int sir;

	//ci = p->p_cpuinfo;

	spic = &intrspic;
	//isp->is_recurse = Xsoftintr;
	//isp->is_resume = Xsoftintr;
	isp = &spic->sp_intsrc;
	isp->is_pic = &softintr_template;

	switch (level) {
	case SOFTINT_BIO:
		sir = SIR_BIO;
		fake_softbio_intrhand->ih_pic = &softintr_template;
		fake_softbio_intrhand->ih_level = IPL_SOFTBIO;
		isp->is_handlers = &fake_softbio_intrhand;
		break;

	case SOFTINT_NET:
		sir = SIR_NET;
		fake_softnet_intrhand->ih_pic = &softintr_template;
		fake_softnet_intrhand->ih_level = IPL_SOFTNET;
		isp->is_handlers = &fake_softnet_intrhand;
		break;

	case SOFTINT_SERIAL:
		sir = SIR_SERIAL;
		fake_softserial_intrhand->ih_pic = &softintr_template;
		fake_softserial_intrhand->ih_level = IPL_SOFTSERIAL;
		isp->is_handlers = &fake_softserial_intrhand;
		break;

	case SOFTINT_CLOCK:
		sir = SIR_CLOCK;
		fake_softclock_intrhand->ih_pic = &softintr_template;
		fake_softclock_intrhand->ih_level = IPL_SOFTCLOCK;
		isp->is_handlers = &fake_softclock_intrhand;
		break;

	default:
		panic("softint_init_md");
	}

	KASSERT(spic->sp_intsrc[sir] == NULL);

	*machdep = (1 << sir);
	spic->sp_intsrc[sir] = isp;
	spic->sp_cpu = ci;

	intr_calculatemasks();
}
#endif /* __HAVE_FAST_SOFTINTS */
