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



struct cpu_info {
	struct cpu_info 	*ci_self;			/* self-pointer */
	struct cpu_info 	*ci_next;			/* next cpu */

	struct device 		*ci_dev;			/* pointer to our device */

	struct cpu_data 	ci_data;			/* MI per-cpu data */

	u_int 				ci_apicid;			/* our APIC ID */

	int					cpu_present:1;
	int					cpu_bsp:1;
	int					cpu_disabled:1;
	int					cpu_hyperthread:1;
};
extern struct cpu_info 	*cpu_info;

struct cpu_ops {
	void 				(*cpu_init)(void);
	void 				(*cpu_resume)(void);
};
extern struct cpu_ops 	cpu_ops;

extern struct percpu 	__percpu[];
#define	PERCPU_MD_FIELDS											\
	struct	percpu 		*pc_prvspace;		/* Self-reference */	\
	struct	i386tss 	*pc_common_tssp;							\
	u_int   			pc_acpi_id;			/* ACPI CPU id */		\
	u_int				pc_apic_id;									\

#ifdef _KERNEL
/*
 * Evaluates to the byte offset of the per-cpu variable name.
 */
#define __percpu_offset(name) 										\
	__offsetof(struct percpu, name)

/*
 * Evaluates to the type of the per-cpu variable name.
 */
#define	__percpu_type(name)											\
	__typeof(((struct percpu *)0)->name)

/*
 * Evaluates to the address of the per-cpu variable name.
 */
#define	__PERCPU_PTR(name) __extension__ ({							\
	__percpu_type(name) *__p;										\
																	\
	__asm __volatile("movl %%fs:%1,%0; addl %2,%0"					\
	    : "=r" (__p)												\
	    : "m" (*(struct percpu *)(__percpu_offset(pc_prvspace))),	\
	      "i" (__percpu_offset(name)));								\
																	\
	__p;															\
})

/*
 * Evaluates to the value of the per-cpu variable name.
 */
#define	__PERCPU_GET(name) __extension__ ({							\
	__percpu_type(name) __res;										\
	struct __s {													\
		u_char	__b[MIN(sizeof(__res), 4)];							\
	} __s;															\
																	\
	if (sizeof(__res) == 1 || sizeof(__res) == 2 ||					\
	    sizeof(__res) == 4) {										\
		__asm __volatile("mov %%fs:%1,%0"							\
		    : "=r" (__s)											\
		    : "m" (*(struct __s *)(__percpu_offset(name))));		\
		*(struct __s *)(void *)&__res = __s;						\
	} else {														\
		__res = *__PERCPU_PTR(name);								\
	}																\
	__res;															\
})

/*
 * Adds a value of the per-cpu counter name.  The implementation
 * must be atomic with respect to interrupts.
 */
#define	__PERCPU_ADD(name, val) do {								\
	__percpu_type(name) __val;										\
	struct __s {													\
		u_char	__b[MIN(sizeof(__val), 4)];							\
	} __s;															\
																	\
	__val = (val);													\
	if (sizeof(__val) == 1 || sizeof(__val) == 2 ||					\
			sizeof(__val) == 4) {									\
		__s = *(struct __s *)(void *)&__val;						\
		__asm __volatile("add %1,%%fs:%0"							\
		    : "=m" (*(struct __s *)(__percpu_offset(name)))			\
		    : "r" (__s));											\
	} else															\
		*__PERCPU_PTR(name) += __val;								\
} while (0)

/*
 * Sets the value of the per-cpu variable name to value val.
 */
#define	__PERCPU_SET(name, val) do {								\
	__percpu_type(name) __val;										\
	struct __s {													\
		u_char	__b[MIN(sizeof(__val), 4)];							\
	} __s;															\
																	\
	__val = (val);													\
	if (sizeof(__val) == 1 || sizeof(__val) == 2 ||					\
	    sizeof(__val) == 4) {										\
		__s = *(struct __s *)(void *)&__val;						\
		__asm __volatile("mov %1,%%fs:%0"							\
		    : "=m" (*(struct __s *)(__percpu_offset(name)))			\
		    : "r" (__s));											\
	} else {														\
		*__PERCPU_PTR(name) = __val;								\
	}																\
} while (0)

#define	get_percpu() __extension__ ({								\
	struct percpu *__pc;											\
																	\
	__asm __volatile("movl %%fs:%1,%0"								\
	    : "=r" (__pc)												\
	    : "m" (*(struct percpu *)(__percpu_offset(pc_prvspace))));	\
	__pc;															\
})

#define	PERCPU_GET(member)		__PERCPU_GET(pc_ ## member)
#define	PERCPU_ADD(member, val)	__PERCPU_ADD(pc_ ## member, val)
#define	PERCPU_PTR(member)		__PERCPU_PTR(pc_ ## member)
#define	PERCPU_SET(member, val)	__PERCPU_SET(pc_ ## member, val)

#define	IS_BSP()				(PERCPU_GET(cpuid) == 0)

struct kthread *
__curkthread(void)
{
	struct kthread *kt;
	__asm("movl %%fs:%1,%0" : "=r" (kt)
			: "m" (*(char *)offsetof(struct percpu, pc_curkthread)));
	return (kt);
}

#endif /* _KERNEL */

#endif /* !_MACHINE_CPU_H_ */
