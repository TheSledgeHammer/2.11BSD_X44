/*
 * The 3-Clause BSD License:
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
/*-
 * SPDX-License-Identifier: Beerware
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.org> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * $FreeBSD$
 */

#ifndef _SYS_CPU_H_
#define _SYS_CPU_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/bitlist.h>

#include <machine/cpu.h>

union cpu_top {
    uint32_t 				ct_mask;
};
typedef union cpu_top 		ctop_t;

void		ctop_set(ctop_t *, uint32_t);
uint32_t	ctop_get(ctop_t *);
int			ctop_isset(ctop_t *, uint32_t);

/* ctop macros */
#define CPU_SET(top, val) 	(ctop_set(top, val))
#define CPU_GET(top) 		(ctop_get(top))
#define CPU_ISSET(top, val)	(ctop_isset(top, val))
#define CPU_EMPTY(top)		((top) == NULL)

/*
 * Types of nodes in the topological tree.
 */
enum topo_node_type {
	/* Processing unit aka computing unit aka logical CPU. */
	TOPO_TYPE_PU,
	/* Physical subdivision of a package. */
	TOPO_TYPE_CORE,
	/* CPU L1/L2/L3 cache. */
	TOPO_TYPE_CACHE,
	/* Package aka chip, equivalent to socket. */
	TOPO_TYPE_PKG,
	/* Other logical or physical grouping of PUs. */
	/* E.g. PUs on the same dye, or PUs sharing an FPU. */
	TOPO_TYPE_GROUP,
	/* The whole system. */
	TOPO_TYPE_SYSTEM
};
typedef enum topo_node_type topo_node_type;

/* Hardware indenitifier of a topology component. */
typedef	unsigned int hwid_t;
/* Logical CPU idenitifier. */
typedef	int cpuid_t;

struct topo_node {
	struct topo_node						*parent;
	TAILQ_HEAD(topo_children, topo_node)	children;
	TAILQ_ENTRY(topo_node)					siblings;
	union cpu_top							cpuset;
	topo_node_type							type;
	__uintptr_t								subtype;
	hwid_t									hwid;
	cpuid_t									id;
	int										nchildren;
	int										cpu_count;
};

/*
 * Defines common resources for CPUs in the group.  The highest level
 * resource should be used when multiple are shared.
 */
#define	CG_SHARE_NONE		0
#define	CG_SHARE_L1			1
#define	CG_SHARE_L2			2
#define	CG_SHARE_L3			3

#define MAX_CACHE_LEVELS	CG_SHARE_L3

/*
 * Convenience routines for building and traversing topologies.
 */
#ifdef SMP
void 				topo_init_node(struct topo_node *node);
void 				topo_init_root(struct topo_node *root);
struct topo_node 	*topo_add_node_by_hwid(struct topo_node *parent, int hwid, topo_node_type type, uintptr_t subtype);
struct topo_node 	*topo_find_node_by_hwid(struct topo_node *parent, int hwid, topo_node_type type, uintptr_t subtype);
void 				topo_promote_child(struct topo_node *child);
struct topo_node 	*topo_next_node(struct topo_node *top, struct topo_node *node);
struct topo_node 	*topo_next_nonchild_node(struct topo_node *top, struct topo_node *node);
void 				topo_set_pu_id(struct topo_node *node, cpuid_t id);

enum topo_level {
	TOPO_LEVEL_PKG = 0,
	/*
	 * Some systems have useful sub-package core organizations.  On these,
	 * a package has one or more subgroups.  Each subgroup contains one or
	 * more cache groups (cores that share a last level cache).
	 */
	TOPO_LEVEL_GROUP,
	TOPO_LEVEL_CACHEGROUP,
	TOPO_LEVEL_CORE,
	TOPO_LEVEL_THREAD,
	TOPO_LEVEL_COUNT	/* Must be last */
};

struct topo_analysis {
	int entities[TOPO_LEVEL_COUNT];
};

int			topo_analyze(struct topo_node *topo_root, int all, struct topo_analysis *results);

#define	TOPO_FOREACH(i, root)	\
	for (i = root; i != NULL; i = topo_next_node(root, i))

#endif /* SMP */
#endif /* _SYS_CPU_H_ */
