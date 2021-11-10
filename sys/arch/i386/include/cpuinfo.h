/*
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

#ifndef _I386_CPUINFO_H_
#define _I386_CPUINFO_H_

#include <sys/cputopo.h>
#include <machine/intr.h>

struct percpu;

struct cpu_info {
	struct cpu_info 		*cpu_self;			/* self-pointer */
	struct device 			*cpu_dev;			/* pointer to our device */
	struct percpu			cpu_percpu;			/* pointer to percpu info, when NCPUS > 1 */
	struct proc				*cpu_curproc;		/* current owner of the processor */

	u_int					cpu_cpuid;			/* This cpu number */
	u_int					cpu_cpumask;		/* This cpu mask */
	size_t					cpu_size;			/* This cpu's size */

	u_int					cpu_acpi_id;		/* This cpu's ACPI id */
	u_int					cpu_apic_id;		/* This cpu's APIC id */

	u_int32_t 				cpu_flags;			/* flags; see below */

	int						cpu_present:1;
	int						cpu_bsp:1;
	int						cpu_disabled:1;
	int						cpu_hyperthread:1;

	union cpu_top 			*cpu_topo;

	/* npx */
	struct proc				*cpu_npxproc;		/* current owner of the FPU */
	int						cpu_fpsaving;		/* save in progress */

	/* ipi's & tlb */
	u_int32_t				cpu_ipis; 			/* interprocessor interrupts pending */
	struct evcnt			cpu_ipi_events[I386_NIPI];
	volatile u_int32_t 		cpu_tlb_ipi_mask;
};
extern struct cpu_info 		*cpu_info;			/* static allocation of cpu_info */

struct cpu_attach_args {
	const char 				*caa_name;
	u_int 					cpu_apic_id;
	u_int					cpu_acpi_id;
	int 					cpu_role;
	struct cpu_ops			*cpu_ops;
};

#define CPU_ROLE_SP			0
#define CPU_ROLE_BP			1
#define CPU_ROLE_AP			2

struct cpu_ops {
	void 					(*cpu_init)(void);
	void 					(*cpu_resume)(void);
};
extern struct cpu_ops 		cpu_ops;

/* cpu_info macros */
#define cpu_is_primary(ci)	((ci)->cpu_flags & CPUF_PRIMARY)
#define cpu_cpuid(ci) 		((ci)->cpu_cpuid)
#define cpu_cpumask(ci)		((ci)->cpu_cpumask)
#define cpu_cpusize(ci)		((ci)->cpu_size)
#define cpu_acpi_id(ci)		((ci)->cpu_acpi_id)
#define cpu_apic_id(ci)		((ci)->cpu_apic_id)
#define cpu_percpu(ci)		((ci)->cpu_percpu)

#define cpu_number() 		(curcpu()->cpu_cpuid) /* number of cpus available */

/*
 * Processor flag notes: The "primary" CPU has certain MI-defined
 * roles (mostly relating to hardclock handling); we distinguish
 * betwen the processor which booted us, and the processor currently
 * holding the "primary" role just to give us the flexibility later to
 * change primaries should we be sufficiently twisted.
 */
#define	CPUF_BSP			0x0001		/* CPU is the original BSP */
#define	CPUF_AP				0x0002		/* CPU is an AP */
#define	CPUF_SP				0x0004		/* CPU is only processor */
#define	CPUF_PRIMARY		0x0008		/* CPU is active primary processor */

#define CPUF_APIC_CD    	0x0010		/* CPU has apic configured */

#define	CPUF_PRESENT		0x1000		/* CPU is present */
#define	CPUF_RUNNING		0x2000		/* CPU is running */
#define	CPUF_PAUSE			0x4000		/* CPU is paused in DDB */
#define	CPUF_GO				0x8000		/* CPU should start running */

extern void (*delay_func)(int);
extern void (*initclock_func)(void);

struct cpu_info;

#ifndef CPU_INFO_ITERATOR
#define	CPU_INFO_ITERATOR			int
#define	CPU_INFO_FOREACH(cii, ci)	\
	(void)cii, ci = curcpu(); ci != NULL; ci = NULL
#endif
#ifndef CPU_IS_PRIMARY
#define	CPU_IS_PRIMARY(ci)	((void)ci, 1)
#endif

__inline static struct cpu_info *
curcpu(void)
{
	struct cpu_info *ci;

	__asm volatile(
			"movl %%fs:%1, %0" : "=r" (ci) : "m"
			(*(struct cpu_info * const *)&((struct cpu_info *)0)->cpu_self)
	);
	return (ci);
}
#endif /* _I386_CPUINFO_H_ */
