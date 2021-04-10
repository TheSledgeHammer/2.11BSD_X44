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

#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

struct percpu;

struct cpu_info {
	struct cpu_info 	*cpu_self;			/* self-pointer */
	struct device 		*cpu_dev;			/* pointer to our device */
	struct percpu		cpu_percpu;		/* pointer to percpu info, when NCPUS > 1 */

	u_int				cpu_cpuid;			/* This cpu number */
	u_int				cpu_cpumask;		/* This cpu mask */
	size_t				cpu_size;			/* This cpu's size */

	u_int				cpu_acpi_id;		/* This cpu's ACPI id */
	u_int				cpu_apic_id;		/* This cpu's APIC id */

	int					cpu_present:1;
	int					cpu_bsp:1;
	int					cpu_disabled:1;
	int					cpu_hyperthread:1;
};
extern struct cpu_info 	*cpu_info;			/* static allocation of cpu_info */

struct cpu_ops {
	void 				(*cpu_init)(void);
	void 				(*cpu_resume)(void);
};
extern struct cpu_ops 	cpu_ops;

/* cpu_info macros */
#define cpu_cpuid(ci) 	((ci)->cpu_cpuid)
#define cpu_cpumask(ci)	((ci)->cpu_cpumask)
#define cpu_cpusize(ci)	((ci)->cpu_size)
#define cpu_acpi_id(ci)	((ci)->cpu_acpi_id)
#define cpu_apic_id(ci)	((ci)->cpu_apic_id)
#define cpu_percpu(ci)	((ci)->cpu_percpu)

#define cpu_number() 	(curcpu()->cpu_cpuid)

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

#endif /* !_MACHINE_CPU_H_ */
