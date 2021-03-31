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

#ifndef _CPU_H_
#define _CPU_H_

struct cpu_info {
	struct cpu_info 	*ci_self;			/* self-pointer */
	struct cpu_info 	*ci_next;			/* next cpu */

	struct device 		*ci_dev;			/* pointer to our device */

	struct cpu_data 	ci_data;			/* MI per-cpu data */

	u_int 				ci_cpu_vendor_id;	/* CPU vendor ID */
	int 				ci_cpu_class;		/* CPU class */

	u_int 				ci_apicid;			/* our APIC ID */

	struct percpu		*ci_percpu;

	int					cpu_present:1;
	int					cpu_bsp:1;
	int					cpu_disabled:1;
	int					cpu_hyperthread:1;
};
extern struct cpu_info *cpu_info;

struct cpu_ops {
	void 				(*cpu_init)(void);
	void 				(*cpu_resume)(void);
};
extern struct cpu_ops cpu_ops;

#define	PERCPU_MD_FIELDS									\
	struct	percpu 	*pc_prvspace;	/* Self-reference */	\
	u_int   		pc_acpi_id;		/* ACPI CPU id */		\


#define percpu_offset(pcpc, name) 		offsetof(struct percpu, name)

#define	percpu_type(name)				__typeof(((struct percpu *)0)->name)

static inline void
cpu_info_cpu(ci)
	struct cpu_info *ci;
{
	ci->ci_cpu_vendor_id = find_cpu_vendor_id();
	ci->ci_cpu_class = find_cpu_class();
}

#endif /* _CPU_H_ */
