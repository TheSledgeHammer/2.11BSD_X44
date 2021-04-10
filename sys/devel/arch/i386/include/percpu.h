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

#ifndef _MACHINE_PERCPU_H_
#define _MACHINE_PERCPU_H_

extern struct percpu 				__percpu[];

#define	PERCPU_MD_FIELDS 														\
	struct	cpu_info				*pc_cpuinfo;		/* Self-reference */	\
	struct	segment_descriptor 		pc_common_tssd;								\
	struct	segment_descriptor 		*pc_tss_gdt;								\
	struct	segment_descriptor 		*pc_fsgs_gdt;								\
	struct	i386tss 				*pc_common_tssp;							\
	u_int							pc_trampstk;								\
	int								pc_currentldt;								\
	u_int   						pc_acpi_id;			/* ACPI CPU id */		\
	u_int							pc_apic_id;			/* APIC CPU id */		\
	char							*pc_copyout_buf;							\

#ifdef _KERNEL
#define	PERCPU_PTR(pc, name)		((pc)->pc_##name)
#define PERCPU_SET(pc, name, val)	(PERCPU_PTR(pc, name) = (val))
#define PERCPU_GET(pc, name)        (PERCPU_PTR(pc, name))
#define PERCPU_ADD(pc, name, val)   (PERCPU_PTR(pc, name) += (val))

#define	IS_BSP(pc)					(PCPU_GET(pc, cpuid) == 0)

struct kthread *
__curkthread(void)
{
	struct kthread *kt;
	__asm("movl %%fs:%1,%0" : "=r" (kt)
			: "m" (*(char *)offsetof(struct percpu, pc_curkthread)));
	return (kt);
}

#endif /* _KERNEL */

#endif /* _MACHINE_PERCPU_H_ */
