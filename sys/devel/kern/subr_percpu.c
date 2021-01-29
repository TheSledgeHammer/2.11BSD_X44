/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2001 Wind River Systems, Inc.
 * All rights reserved.
 * Written by: John Baldwin <jhb@FreeBSD.org>
 *
 * Copyright (c) 2009 Jeffrey Roberson <jeff@freebsd.org>
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
 */

/*
 * This module provides MI support for per-cpu data.
 *
 * Each architecture determines the mapping of logical CPU IDs to physical
 * CPUs.  The requirements of this mapping are as follows:
 *  - Logical CPU IDs must reside in the range 0 ... MAXCPU - 1.
 *  - The mapping is not required to be dense.  That is, there may be
 *    gaps in the mappings.
 *  - The platform sets the value of MAXCPU in <machine/param.h>.
 *  - It is suggested, but not required, that in the non-SMP case, the
 *    platform define MAXCPU to be 1 and define the logical ID of the
 *    sole CPU as 0.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD$"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <devel/sys/percpu.h>

struct dpcpuhead;
TAILQ_HEAD(dpcpuhead, dpcpu_free);
struct dpcpu_free {
	uintptr_t				df_start;
	int						df_len;
	TAILQ_ENTRY(dpcpu_free) df_link;
};

static struct dpcpuhead dpcpu_head = TAILQ_HEAD_INITIALIZER(dpcpu_head);
static struct lock_object dpcpu_lock;
uintptr_t dpcpu_off[NCPUS];
struct percpu *cpuid_to_percpu[NCPUS];
struct cpuhead cpuhead = LIST_HEAD_INITIALIZER(cpuhead);

/*
 * Initialize the MI portions of a struct pcpu.
 * (NOTE: runs in mp_machdep.c (init_secondary))
 */
void
percpu_init(pcpu, cpuid, size)
	struct percpu *pcpu;
	int cpuid;
	size_t size;
{
	bzero(pcpu, size);
	KASSERT(cpuid >= 0 && cpuid < NCPUS ("percpu_init: invalid cpuid %d", cpuid));
	pcpu->pc_cpuid = cpuid;
	cpuid_to_percpu[cpuid] = pcpu;
	LIST_INSERT_HEAD(&cpuhead, pcpu, pc_entry);
	cpu_percpu_init(pcpu, cpuid, size);
}

/* runs in mp_machdep.c (init_secondary) */
void
dpcpu_init(dpcpu, cpuid)
	void *dpcpu;
	int cpuid;
{
	struct percpu *pcpu;

	pcpu = percpu_find(cpuid);
	pcpu->pc_dynamic = (uintptr_t)dpcpu - DPCPU_START;

	/*
	 * Initialize defaults from our linker section.
	 */
	memcpy(dpcpu, (void *)DPCPU_START, DPCPU_BYTES);

	/*
	 * Place it in the global percpu offset array.
	 */
	dpcpu_off[cpuid] = pcpu->pc_dynamic;
}

/* runs via sysinit */
static void
dpcpu_startup(void *dummy)
{
	struct dpcpu_free *df;

	df = malloc(sizeof(*df), M_PERCPU, M_WAITOK | M_ZERO);
	//df->df_start = (uintptr_t)&DPCPU_NAME(modspace);
	//df->df_len = DPCPU_MODMIN;
	TAILQ_INSERT_HEAD(&dpcpu_head, df, df_link);
	simple_lock_init(&dpcpu_lock, "dpcpu_lock");
}

/*
 * First-fit extent based allocator for allocating space in the per-cpu
 * region reserved for modules.  This is only intended for use by the
 * kernel linkers to place module linker sets.
 */
void *
dpcpu_alloc(int size)
{
	struct dpcpu_free 	*df;
	void 				*s;

	s = NULL;
	size = roundup2(size, sizeof(void*));
	simple_lock(&dpcpu_lock);
	TAILQ_FOREACH(df, &dpcpu_head, df_link)	{
		if (df->df_len < size)
			continue;
		if (df->df_len == size) {
			s = (void*) df->df_start;
			TAILQ_REMOVE(&dpcpu_head, df, df_link);
			free(df, M_PERCPU);
			break;
		}
		s = (void*) df->df_start;
		df->df_len -= size;
		df->df_start = df->df_start + size;
		break;
	}
	simple_unlock(&dpcpu_lock);

	return (s);
}

/*
 * Free dynamic per-cpu space at module unload time.
 */
void
dpcpu_free(void *s, int size)
{
	struct dpcpu_free *df;
	struct dpcpu_free *dn;
	uintptr_t start;
	uintptr_t end;

	size = roundup2(size, sizeof(void *));
	start = (uintptr_t)s;
	end = start + size;
	/*
	 * Free a region of space and merge it with as many neighbors as
	 * possible.  Keeping the list sorted simplifies this operation.
	 */
	simple_lock(&dpcpu_lock);
	TAILQ_FOREACH(df, &dpcpu_head, df_link)	{
		if (df->df_start > end)
			break;
		/*
		 * If we expand at the end of an entry we may have to
		 * merge it with the one following it as well.
		 */
		if (df->df_start + df->df_len == start) {
			df->df_len += size;
			dn = TAILQ_NEXT(df, df_link);
			if (df->df_start + df->df_len == dn->df_start) {
				df->df_len += dn->df_len;
				TAILQ_REMOVE(&dpcpu_head, dn, df_link);
				free(dn, M_PERCPU);
			}
			simple_unlock(&dpcpu_lock);
			return;
		}
		if (df->df_start == end) {
			df->df_start = start;
			df->df_len += size;
			simple_unlock(&dpcpu_lock);
			return;
		}
	}
	dn = malloc(sizeof(*df), M_PERCPU, M_WAITOK | M_ZERO);
	dn->df_start = start;
	dn->df_len = size;
	if (df) {
		TAILQ_INSERT_BEFORE(df, dn, df_link);
	} else {
		TAILQ_INSERT_TAIL(&dpcpu_head, dn, df_link);
	}
	simple_unlock(&dpcpu_lock);
}

/*
 * Initialize the per-cpu storage from an updated linker-set region.
 */
void
dpcpu_copy(void *s, int size)
{
#ifdef SMP
	uintptr_t dpcpu;
	int i;

	CPU_FOREACH(i) {
		dpcpu = dpcpu_off[i];
		if (dpcpu == 0)
			continue;
		memcpy((void *)(dpcpu + (uintptr_t)s), s, size);
	}
#else
	memcpy((void *)(dpcpu_off[0] + (uintptr_t)s), s, size);
#endif
}

/*
 * Destroy a struct percpu.
 */
void
percpu_destroy(pcpu)
	struct percpu *pcpu;
{
	LIST_REMOVE(pcpu, pc_entry);
	cpuid_to_percpu[pcpu->pc_cpuid] = NULL;
	dpcpu_off[pcpu->pc_cpuid] = 0;
}

/*
 * Locate a struct percpu by cpu id.
 */
struct percpu *
percpu_find(cpuid)
	int cpuid;
{
	return (cpuid_to_percpu[cpuid]);
}

/* i386 machdep.c */
void
cpu_percpu_init(pcpu, cpuid, size)
	struct percpu *pcpu;
	int cpuid;
	size_t size;
{
	pcpu->pc_acpi_id = 0xffffffff;
}
