/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2008,	Jeffrey Roberson <jeff@freebsd.org>
 * All rights reserved.
 *
 * Copyright (c) 2008 Nokia Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *
 * $FreeBSD$
 */

#ifndef _SYS_CPUSET_H_
#define _SYS_CPUSET_H_

#include <devel/sys/bitset.h>

#ifdef _KERNEL
#define	CPU_SETSIZE		NCPUS
#endif

#define	CPU_MAXSIZE		256

#ifndef	CPU_SETSIZE
#define	CPU_SETSIZE		CPU_MAXSIZE
#endif

BITSET_DEFINE(_cpuset, CPU_SETSIZE);
typedef struct _cpuset cpuset_t;

#define	_NCPUBITS		_BITSET_BITS
#define	_NCPUWORDS		__bitset_words(CPU_SETSIZE)

#define	CPUSETBUFSIZ	((2 + sizeof(long) * 2) * _NCPUWORDS)

#define	CPU_CLR(n, p)				BIT_CLR(CPU_SETSIZE, n, p)
#define	CPU_COPY(f, t)				BIT_COPY(CPU_SETSIZE, f, t)
#define	CPU_ISSET(n, p)				BIT_ISSET(CPU_SETSIZE, n, p)
#define	CPU_SET(n, p)				BIT_SET(CPU_SETSIZE, n, p)
#define	CPU_ZERO(p) 				BIT_ZERO(CPU_SETSIZE, p)
#define	CPU_FILL(p) 				BIT_FILL(CPU_SETSIZE, p)
#define	CPU_SETOF(n, p)				BIT_SETOF(CPU_SETSIZE, n, p)
#define	CPU_EMPTY(p)				BIT_EMPTY(CPU_SETSIZE, p)
#define	CPU_ISFULLSET(p)			BIT_ISFULLSET(CPU_SETSIZE, p)
#define	CPU_SUBSET(p, c)			BIT_SUBSET(CPU_SETSIZE, p, c)
#define	CPU_OVERLAP(p, c)			BIT_OVERLAP(CPU_SETSIZE, p, c)
#define	CPU_CMP(p, c)				BIT_CMP(CPU_SETSIZE, p, c)
#define	CPU_OR(d, s)				BIT_OR(CPU_SETSIZE, d, s)
#define	CPU_AND(d, s)				BIT_AND(CPU_SETSIZE, d, s)
#define	CPU_ANDNOT(d, s)			BIT_ANDNOT(CPU_SETSIZE, d, s)
#define	CPU_CLR_ATOMIC(n, p)		BIT_CLR_ATOMIC(CPU_SETSIZE, n, p)
#define	CPU_SET_ATOMIC(n, p)		BIT_SET_ATOMIC(CPU_SETSIZE, n, p)
#define	CPU_SET_ATOMIC_ACQ(n, p)	BIT_SET_ATOMIC_ACQ(CPU_SETSIZE, n, p)
#define	CPU_AND_ATOMIC(n, p)		BIT_AND_ATOMIC(CPU_SETSIZE, n, p)
#define	CPU_OR_ATOMIC(d, s)			BIT_OR_ATOMIC(CPU_SETSIZE, d, s)
#define	CPU_COPY_STORE_REL(f, t)	BIT_COPY_STORE_REL(CPU_SETSIZE, f, t)
#define	CPU_FFS(p)					BIT_FFS(CPU_SETSIZE, p)
#define	CPU_FLS(p)					BIT_FLS(CPU_SETSIZE, p)
#define	CPU_COUNT(p)				((int)BIT_COUNT(CPU_SETSIZE, p))
#define	CPUSET_FSET					BITSET_FSET(_NCPUWORDS)
#define	CPUSET_T_INITIALIZER		BITSET_T_INITIALIZER

/*
 * Reserved cpuset identifiers.
 */
#define	CPUSET_INVALID	-1
#define	CPUSET_DEFAULT	0

#ifdef _KERNEL
#include <sys/queue.h>

LIST_HEAD(setlist, cpuset);

struct cpuset {
	volatile u_int		cs_ref;			/* (a) Reference count. */
	int					cs_flags;		/* (s) Flags from below. */
	LIST_ENTRY(cpuset)	cs_link;		/* (c) All identified sets. */
	LIST_ENTRY(cpuset)	cs_siblings;	/* (c) Sibling set link. */
	struct setlist		cs_children;	/* (c) List of children. */
	cpusetid_t			cs_id;			/* (s) Id or INVALID. */
	struct cpuset		*cs_parent;		/* (s) Pointer to our parent. */
	cpuset_t			cs_mask;		/* bitmask of valid cpus. */
};
typedef struct cpuset 	cpuset_t;
typedef	int				cpusetid_t;

#define CPU_SET_ROOT    0x0001  		/* Set is a root set. */
#define CPU_SET_RDONLY  0x0002  		/* No modification allowed. */

extern cpuset_t 		*cpuset_root;

#else

#endif
#endif /* _SYS_CPUSET_H_ */
