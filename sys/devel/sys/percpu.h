/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2001 Wind River Systems, Inc.
 * All rights reserved.
 * Written by: John Baldwin <jhb@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

#ifndef _SYS_PERCPU_H_
#define	_SYS_PERCPU_H_

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/resource.h>

#define	DPCPU_SETNAME		"set_pcpu"
#define	DPCPU_SYMPREFIX		"pcpu_entry_"

/*
 * Define a set for pcpu data.
 */
extern uintptr_t *__start_set_pcpu;
__GLOBL(__start_set_pcpu);
extern uintptr_t *__stop_set_pcpu;
__GLOBL(__stop_set_pcpu);

/*
 * Array of dynamic percpu base offsets.  Indexed by id.
 */
extern uintptr_t 		dpcpu_off[];

/*
 * Convenience defines.
 */
#define	DPCPU_QUANTUM_SIZE	(ALIGNBYTES + 1)
#define	DPCPU_SIZE			2048

/*
 * Declaration and definition.
 */
#define	DPCPU_NAME(n)	pcpu_entry_##n
#define	DPCPU_DECLARE(t, n)	extern t DPCPU_NAME(n)

struct cpuhead;
LIST_HEAD(cpuhead, percpu);
struct percpu {
	u_int				pc_cpuid;		/* This cpu number */
	LIST_ENTRY(percpu) 	pc_entry;

	struct extent		*pc_extent;		/* Dynamic storage alloctor */
	uintptr_t			pc_dynamic;		/* Dynamic per-cpu data area */
};

extern struct cpuhead 	cpuhead;
extern struct percpu 	*cpuid_to_percpu[];

/*
 * Machine dependent callouts.  cpu_percpu_init() is responsible for
 * initializing machine dependent fields of struct percpu.
 */
void			cpu_percpu_init(struct percpu *pcpu, int cpuid, size_t size);

void			*dpcpu_alloc(int size);
void			dpcpu_copy(void *s, int size);
void			dpcpu_free(void *s, int size);
void			dpcpu_init(void *dpcpu, int cpuid);
void			percpu_destroy(struct percpu *pcpu);
struct percpu 	*percpu_find(u_int cpuid);
void			percpu_init(struct percpu *pcpu, int cpuid, size_t size);

#endif /* _SYS_PERCPU_H_ */
