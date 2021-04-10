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
#include <machine/cpu.h>
#include <machine/param.h>


#include <devel/arch/i386/include/cpu.h>
#include <devel/arch/i386/include/percpu.h>

struct cpu_info;

struct cpuhead;
LIST_HEAD(cpuhead, percpu);
struct percpu {
	u_int					pc_cpuid;			/* This cpu number */
	u_int					pc_cpumask;			/* This cpu mask */
	LIST_ENTRY(percpu) 		pc_entry;
	struct kthread			*pc_curkthread;

	size_t					pc_dynamic;			/* Dynamic per-cpu data area */
	size_t					pc_size;			/* Static per-cpu allocation */

	struct extent			*pc_extent;			/* Dynamic storage alloctor */
	u_long 					pc_start;			/* start of per-cpu extent region */
	u_long 					pc_end;				/* end of per-cpu extent region */


	u_int					pc_offset;
	PERCPU_MD_FIELDS;
};

extern struct cpuhead 			cpuhead;
extern struct percpu 			*cpuid_to_percpu[];

/* percpu definitions */
#define PERCPU_START 			(ALIGNBYTES + 1)
#define PERCPU_END 				2048
#define PERCPU_SIZE				2048
#define PERCPU_ALIGN			PAGE_SIZE
#define PERCPU_BOUNDARY			EX_NOBOUNDARY
#define percpu_offset_cpu(cpu)  (PERCPU_SIZE * cpu)

/*
 * Machine dependent callouts.  cpu_percpu_init() is responsible for
 * initializing machine dependent fields of struct percpu.
 */
void					percpu_init(struct percpu *, struct cpu_info *, size_t);
struct percpu 			*percpu_start(struct cpu_info *, size_t);
void					percpu_remove(struct cpu_info *);
struct percpu 			*percpu_lookup(struct cpu_info *);
struct percpu 			*percpu_create(struct cpu_info *, size_t, int, int);
void					percpu_malloc(struct percpu *, size_t);
void					percpu_free(struct percpu *);
void					percpu_destroy(struct percpu *);
struct percpu 			*percpu_find(int);
void					percpu_extent(struct percpu *, u_long, u_long);
void					percpu_extent_region(struct percpu *);
void					percpu_extent_subregion(struct percpu *, size_t);
void					percpu_extent_free(struct percpu *, u_long, u_long, int);

#endif /* _SYS_PERCPU_H_ */
