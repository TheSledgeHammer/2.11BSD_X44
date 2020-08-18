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
 *	@(#)cpu.h	8.5 (Berkeley) 5/17/95
 */

#ifndef _I386_CPU_H_
#define _I386_CPU_H_

/*
 * Definitions unique to i386 cpu support.
 */
#include <machine/psl.h>
#include <machine/frame.h>
#include <machine/segments.h>

struct pmap;

struct cpu_info {
	u_int32_t		ci_kern_cr3;			/* U+K page table */
	u_int32_t		ci_scratch;				/* for U<-->K transition */
	struct device 	*ci_dev;				/* our device */
	struct cpu_info *ci_self;				/* pointer to this structure */
	struct cpu_info *ci_next;				/* next cpu */

	/*
	 * Public members.
	 */
	struct proc 	*ci_curproc; 			/* current owner of the processor */
	struct user		*ci_user;
	cpuid_t 		ci_cpuid; 				/* our CPU ID */
	u_int 			ci_apicid;				/* our APIC ID */
	u_int 			ci_acpi_proc_id;
	u_int32_t		ci_randseed;

	u_int32_t 		ci_kern_esp;			/* kernel-only stack */
	u_int32_t 		ci_intr_esp;			/* U<-->K trampoline stack */
	u_int32_t 		ci_user_cr3;			/* U-K page table */

	/*
	 * Private members.
	 */
	struct proc		*ci_fpcurproc;
	struct proc 	*ci_fpsaveproc;
	int 			ci_fpsaving;			/* save in progress */

	struct pcb 		*ci_curpcb;				/* VA of current HW PCB */
	struct pcb 		*ci_idle_pcb;			/* VA of current PCB */
	struct pmap 	*ci_pmap;				/* current pmap */

	u_int32_t 		ci_level;
	u_int32_t 		ci_vendor[4];
	u_int32_t 		ci_signature; 			/* X86 cpuid type */
	u_int32_t 		ci_family; 				/* extended cpuid family */
	u_int32_t 		ci_model; 				/* extended cpuid model */
	u_int32_t 		ci_feature_flags; 		/* X86 CPUID feature bits */
	u_int32_t		ci_feature_sefflags_ebx;/* more CPUID feature bits */
	u_int32_t		ci_feature_sefflags_ecx;/* more CPUID feature bits */
	u_int32_t		ci_feature_sefflags_edx;/* more CPUID feature bits */
	u_int32_t		ci_feature_tpmflags;	/* thermal & power bits */
	u_int32_t		cpu_class;				/* CPU class */
	u_int32_t		ci_cflushsz;			/* clflush cache-line size */
	u_int32_t		ci_amdcacheinfo[4];		/* AMD cache info */
	u_int32_t		ci_extcacheinfo[4];		/* Intel cache info */

	void (*cpu_setup)(struct cpu_info *);	/* proc-dependant init */
};

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#undef	COPY_SIGCODE				/* don't copy sigcode above user stack in exec */

#define	cpu_exec(p)					/* nothing */
#define	cpu_swapin(p)				/* nothing */
#define cpu_setstack(p, ap)			(p)->p_md.md_regs[SP] = ap
#define cpu_set_init_frame(p, fp)	(p)->p_md.md_regs = fp

#define	BACKTRACE(p)				/* not implemented */

/*
 * Arguments to hardclock, softclock and gatherstats
 * encapsulate the previous machine state in an opaque
 * clockframe; for now, use generic intrframe.
 */
struct clockframe {
	struct intrframe	cf_if;
};

#define	CLKF_USERMODE(framep)	(ISPL((framep)->cf_if.if_cs) == SEL_UPL)
#define	CLKF_BASEPRI(framep)	((framep)->cf_if.if_ppl == 0)
#define	CLKF_PC(framep)			((framep)->cf_if.if_eip)

#define	resettodr()	/* no todr to set */

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	need_resched()	{ want_resched++; aston(); }

/*
 * Give a profiling tick to the current process from the softclock
 * interrupt.  On tahoe, request an ast to send us through trap(),
 * marking the proc as needing a profiling tick.
 */
#define	profile_tick(p, framep)	{ (p)->p_flag |= P_OWEUPC; aston(); }

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)	aston()

#define aston() 		(astpending++)

int	astpending;			/* need to trap before returning to user mode */
int	want_resched;		/* resched() was called */

/*
 * pull in #defines for kinds of processors
 */
#include <machine/cputypes.h>

struct cpu_nocpuid_nameclass {
	int 		cpu_vendor;
	const char 	*cpu_vendorname;
	const char 	*cpu_name;
	int 		cpu_class;
	void 		(*cpu_setup) (void);
};

struct cpu_cpuid_nameclass {
	const char 		*cpu_id;
	int 			cpu_vendor;
	const char 		*cpu_vendorname;
	struct cpu_cpuid_family {
		int 		cpu_class;
		const char 	*cpu_models[CPU_MAXMODEL+2];
		void 		(*cpu_setup) (void);
	} cpu_family[CPU_MAXFAMILY - CPU_MINFAMILY + 1];
};

#ifdef _KERNEL
extern int cpu;
extern int cpu_class;
extern int cpu_feature;
extern int cpuid_level;
extern struct cpu_nocpuid_nameclass i386_nocpuid_cpus[];
extern struct cpu_cpuid_nameclass i386_cpuid_cpus[];

/* locore.s */
struct pcb;
void	savectx (struct pcb *);

/* clock.c */
void	startrtclock (void);

/* autoconf.c */
void	configure (void);

/* vm_machdep.c */
int		cpu_fork (struct proc *, struct proc *);

#ifdef USER_LDT
/* sys_machdep.h */
int		i386_get_ldt (struct proc *, char *, register_t *);
int		i386_set_ldt (struct proc *, char *, register_t *);
#endif

/* isa_machdep.c */
void	isa_defaultirq (void);
int		isa_nmi (void);

/* bus_machdep.c */
void 	i386_bus_space_init	(void);
void 	i386_bus_space_mallocok	(void);
void	i386_bus_space_check (vm_offset_t, int, int);

#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_BIOSBASEMEM	2	/* int: bios-reported base mem (K) */
#define	CPU_BIOSEXTMEM	3	/* int: bios-reported ext. mem (K) */

#define	CPU_MAXID		4	/* number of valid machdep ids */

#define CTL_MACHDEP_NAMES { 				\
	{ 0, 0 }, 								\
	{ "console_device", CTLTYPE_STRUCT }, 	\
	{ "biosbasemem", CTLTYPE_INT }, 		\
	{ "biosextmem", CTLTYPE_INT }, 			\
}

#ifndef LOCORE
extern int	cpu;
extern int	cpu_class;
#endif

#endif /* !_I386_CPU_H_ */
