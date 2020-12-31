/*-
 * Copyright (c) 1990 William Jolitz.
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	from: @(#)npx.c	7.2 (Berkeley) 5/12/91
 */

#include <sys/cdefs.h>

/* __FBSDID("$FreeBSD$");

#include "opt_cpu.h"
#include "opt_isa.h"
#include "opt_npx.h"
*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/proc.h>
//#include <sys/smp.h>
#include <sys/sysctl.h>

#include <sys/map.h>
#ifdef NPX_DEBUG
#include <sys/syslog.h>
#endif
#include <sys/signalvar.h>
#include <vm/include/vm.h>

#include <machine/bus.h>
#include <machine/cputypes.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/specialreg.h>
#include <machine/segments.h>
#include <machine/npx.h>
#include <machine/intr.h>

//#ifdef DEV_ISA
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
//#endif

/*
 * 387 and 287 Numeric Coprocessor Extension (NPX) Driver.
 */

#if defined(__GNUCLIKE_ASM) && !defined(lint)

#define	fldcw(cw)			__asm __volatile("fldcw %0" : : "m" (cw))
#define	fnclex()			__asm __volatile("fnclex")
#define	fninit()			__asm __volatile("fninit")
#define	fnsave(addr)		__asm __volatile("fnsave %0" : "=m" (*(addr)))
#define	fnstcw(addr)		__asm __volatile("fnstcw %0" : "=m" (*(addr)))
#define	fnstsw(addr)		__asm __volatile("fnstsw %0" : "=am" (*(addr)))
#define	fp_divide_by_0()	__asm __volatile("fldz; fld1; fdiv %st,%st(1); fnop")
#define	frstor(addr)		__asm __volatile("frstor %0" : : "m" (*(addr)))
#define	fxrstor(addr)		__asm __volatile("fxrstor %0" : : "m" (*(addr)))
#define	fxsave(addr)		__asm __volatile("fxsave %0" : "=m" (*(addr)))
#define	ldmxcsr(csr)		__asm __volatile("ldmxcsr %0" : : "m" (csr))
#define	stmxcsr(addr)		__asm __volatile("stmxcsr %0" : : "m" (*(addr)))

static __inline void
xrstor(char *addr, uint64_t mask)
{
	uint32_t low, hi;

	low = mask;
	hi = mask >> 32;
	__asm __volatile("xrstor %0" : : "m" (*addr), "a" (low), "d" (hi));
}

static __inline void
xsave(char *addr, uint64_t mask)
{
	uint32_t low, hi;

	low = mask;
	hi = mask >> 32;
	__asm __volatile("xsave %0" : "=m" (*addr) : "a" (low), "d" (hi) : "memory");
}

static __inline void
xsaveopt(char *addr, uint64_t mask)
{
	uint32_t low, hi;

	low = mask;
	hi = mask >> 32;
	__asm __volatile("xsaveopt %0" : "=m" (*addr) : "a" (low), "d" (hi) : "memory");
}
#else	/* !(__GNUCLIKE_ASM && !lint) */

void	fldcw(u_short cw);
void	fnclex(void);
void	fninit(void);
void	fnsave(caddr_t addr);
void	fnstcw(caddr_t addr);
void	fnstsw(caddr_t addr);
void	fp_divide_by_0(void);
void	frstor(caddr_t addr);
void	fxsave(caddr_t addr);
void	fxrstor(caddr_t addr);
void	ldmxcsr(u_int csr);
void	stmxcsr(u_int *csr);
void	xrstor(char *addr, uint64_t mask);
void	xsave(char *addr, uint64_t mask);
void	xsaveopt(char *addr, uint64_t mask);

#endif	/* __GNUCLIKE_ASM && !lint */

//#define	clts()				__asm __volatile("clts")

#define	start_emulating()	lcr0(rcr0() | CR0_TS)
#define	stop_emulating()	clts()

int 		use_xsave;
uint64_t 	xsave_mask;

int npxprobe(struct device *, void *, void *);
void npxattach(struct device *, struct device *, void *);

struct npx_softc {
	struct device 	sc_dev;
	void *			sc_ih;
};

struct cfdriver npx_cd = {
	NULL, "npx", npxprobe, npxattach, NULL, DV_DULL, sizeof(struct npx_softc)
};

static void
fpusave_xsaveopt(union savefpu *addr)
{
	xsaveopt((char *)addr, xsave_mask);
}

static void
fpusave_xsave(union savefpu *addr)
{
	xsave((char *)addr, xsave_mask);
}

static void
fpusave_fxsave(union savefpu *addr)
{
	fxsave((char *)addr);
}

static void
fpusave_fnsave(union savefpu *addr)
{
	fnsave((char *)addr);
}

static void
init_xsave(void)
{
	if (use_xsave)
		return;
	if (!cpu_fxsr || (cpu_feature2 & CPUID2_XSAVE) == 0)
		return;
	use_xsave = 1;
}

/*
 * Enable XSAVE if supported and allowed by user.
 * Calculate the xsave_mask.
 */
static void
npxinit_bsp1(void)
{
	u_int cp[4];
	uint64_t xsave_mask_user;

	TUNABLE_INT_FETCH("hw.lazy_fpu_switch", &lazy_fpu_switch);
	if (!use_xsave)
		return;
	cpuid_count(0xd, 0x0, cp);
	xsave_mask = XFEATURE_ENABLED_X87 | XFEATURE_ENABLED_SSE;
	if ((cp[0] & xsave_mask) != xsave_mask)
		panic("CPU0 does not support X87 or SSE: %x", cp[0]);
	xsave_mask = ((uint64_t) cp[3] << 32) | cp[0];
	xsave_mask_user = xsave_mask;
	TUNABLE_QUAD_FETCH("hw.xsave_mask", &xsave_mask_user);
	xsave_mask_user |= XFEATURE_ENABLED_X87 | XFEATURE_ENABLED_SSE;
	xsave_mask &= xsave_mask_user;
	if ((xsave_mask & XFEATURE_AVX512) != XFEATURE_AVX512)
		xsave_mask &= ~XFEATURE_AVX512;
	if ((xsave_mask & XFEATURE_MPX) != XFEATURE_MPX)
		xsave_mask &= ~XFEATURE_MPX;
}

/*
 * Calculate the fpu save area size.
 */
static void
npxinit_bsp2(void)
{
	u_int cp[4];

	if (use_xsave) {
		cpuid_count(0xd, 0x0, cp);
		cpu_max_ext_state_size = cp[1];

		/*
		 * Reload the cpu_feature2, since we enabled OSXSAVE.
		 */
		do_cpuid(1, cp);
		cpu_feature2 = cp[2];
	} else
		cpu_max_ext_state_size = sizeof(struct savefpu);
}

void
npxinit(bool bsp)
{
	static struct savefpu dummy;
	register_t saveintr;
	u_int mxcsr;
	u_short control;

	if (bsp) {
		if (!npx_probe())
			return;
		npxinit_bsp1();
	}

	if (use_xsave) {
		lcr4(rcr4() | CR4_XSAVE);
		load_xcr(XCR0, xsave_mask);
	}

	/*
	 * XCR0 shall be set up before CPU can report the save area size.
	 */
	if (bsp)
		npxinit_bsp2();

	/*
	 * fninit has the same h/w bugs as fnsave.  Use the detoxified
	 * fnsave to throw away any junk in the fpu.  fpusave() initializes
	 * the fpu.
	 *
	 * It is too early for critical_enter() to work on AP.
	 */
	saveintr = intr_disable();
	stop_emulating();
	if (cpu_fxsr)
		fninit();
	else
		fnsave(&dummy);
	control = ___NPX87___;
	fldcw(control);
	if (cpu_fxsr) {
		mxcsr = ___MXCSR___;
		ldmxcsr(mxcsr);
	}
	start_emulating();
	intr_restore(saveintr);
}
