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
#define	CPU_SETSIZE	NCPUS
#endif

#define	CPU_MAXSIZE	256

#ifndef	CPU_SETSIZE
#define	CPU_SETSIZE	CPU_MAXSIZE
#endif

BITSET_DEFINE(_cpuset, CPU_SETSIZE);
typedef struct _cpuset cpuset_t;

#define	CPU_ISSET(n, p)			BIT_ISSET(CPU_SETSIZE, n, p)
#define	CPU_SET(n, p)			BIT_SET(CPU_SETSIZE, n, p)
#define	CPU_EMPTY(p)			BIT_EMPTY(CPU_SETSIZE, p)
#define	CPU_FFS(p)				BIT_FFS(CPU_SETSIZE, p)

#ifdef _KERNEL
#include <sys/queue.h>

LIST_HEAD(setlist, cpuset);

struct cpuset {
	volatile u_int		cs_ref;			/* (a) Reference count. */
	int					cs_flags;		/* (s) Flags from below. */
	LIST_ENTRY(cpuset)	cs_link;		/* (c) All identified sets. */
	LIST_ENTRY(cpuset)	cs_siblings;	/* (c) Sibling set link. */
	struct setlist		cs_children;	/* (c) List of children. */
};
typedef struct cpuset 	cpuset_t;

#else

#endif
#endif /* _SYS_CPUSET_H_ */
