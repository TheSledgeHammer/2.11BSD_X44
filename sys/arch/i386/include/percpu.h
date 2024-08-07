/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) Peter Wemm
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _I386_PERCPU_H_
#define _I386_PERCPU_H_

#include <sys/percpu.h>
#include <sys/stddef.h>

#define	PERCPU_MD_FIELDS 																\
	struct	percpu 					*pc_prvspace;		/* Self-reference */			\
	struct	cpu_info				*pc_cpuinfo;										\
	struct	segment_descriptor 		pc_common_tssd;										\
	struct	segment_descriptor 		*pc_tss_gdt;										\
	struct	segment_descriptor 		*pc_fsgs_gdt;										\
	struct	i386tss 				*pc_common_tssp;									\
	u_int							pc_trampstk;										\
	int								pc_currentldt;										\
	u_int   						pc_acpi_id;			/* ACPI CPU id */				\
	u_int							pc_apic_id;			/* APIC CPU id */				\
	uint32_t 						pc_smp_tlb_done;	/* TLB op acknowledgement */	\

#ifdef _KERNEL
#define	__percpu_offset(name)		offsetof(struct percpu, name)
#define	__percpu_type(name)			__typeof(((struct percpu *)0)->name)

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
	    sizeof(__val) == 4) {										\
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
#ifdef notyet
#define	get_percpu() __extension__ ({								\
	struct percpu *__pc;											\
																	\
	__asm __volatile("movl %%fs:%1,%0"								\
	    : "=r" (__pc)												\
	    : "m" (*(struct percpu *)(__percpu_offset(pc_cpuinfo))));	\
	__pc;															\
})
#endif
#define	get_percpu()			curcpu()->cpu_percpu

#define	PERCPU_GET(member)		__PERCPU_GET(pc_ ## member)
#define	PERCPU_ADD(member, val)	__PERCPU_ADD(pc_ ## member, val)
#define	PERCPU_PTR(member)		__PERCPU_PTR(pc_ ## member)
#define	PERCPU_SET(member, val)	__PERCPU_SET(pc_ ## member, val)

#define	IS_BSP()				(__PERCPU_GET(cpuid) == 0)

#endif /* _KERNEL */

#endif /* _I386_PERCPU_H_ */
