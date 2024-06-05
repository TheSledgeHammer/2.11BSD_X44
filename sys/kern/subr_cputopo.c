/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2001, John Baldwin <jhb@FreeBSD.org>.
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
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/user.h>
#include <sys/cputopo.h>

/*
 * Cpu_topo (MI & MD related) is an interface to the underlying bitlist/hash array.
 * With the main goal of transferring information about the smp topology (topo_node)
 * to the cpu (cpu_info).
 *
 */

void
cpu_topo_init(topo)
	cpu_topo_t 	*topo;
{
	bitlist_init();
	topo = (cpu_topo_t *)malloc(sizeof(cpu_topo_t *), M_CPUTOPO, M_WAITOK);
}

void
cpu_topo_set(topo, val)
	cpu_topo_t 	*topo;
	uint32_t val;
{
    topo->ct_mask = val;
    bitlist_insert(val);
}

uint32_t
cpu_topo_get(topo)
	cpu_topo_t *topo;
{
	bitlist_t *bt;

	bt = bitlist_search(topo->ct_mask);
	if (bt != NULL) {
		return (bt->value);
	}
	return (0);
}

void
cpu_topo_remove(topo)
	cpu_topo_t *topo;
{

    bitlist_remove(topo->ct_mask);
}

int
cpu_topo_isset(topo, val)
	cpu_topo_t *topo;
	uint32_t val;
{
	if (topo->ct_mask != val) {
		return (1);
	}
	return (0);
}

int
cpu_topo_empty(topo)
	cpu_topo_t *topo;
{
	if (topo == NULL) {
		return (0);
	}
	return (1);
}

/*
 * This module holds the global variables and machine independent functions
 * used for the kernel SMP support.
 */

#ifdef SMP
void
topo_init_node(struct topo_node *node)
{
	bzero(node, sizeof(*node));
	TAILQ_INIT(&node->children);
	cpu_topo_init(&node->cpuset);
}

void
topo_init_root(struct topo_node *root)
{
	topo_init_node(root);
	root->type = TOPO_TYPE_SYSTEM;
}

/*
 * Add a child node with the given ID under the given parent.
 * Do nothing if there is already a child with that ID.
 */
struct topo_node *
topo_add_node_by_hwid(struct topo_node *parent, int hwid,
    topo_node_type type, uintptr_t subtype)
{
	struct topo_node *node;

	TAILQ_FOREACH_REVERSE(node, &parent->children,
	    topo_children, siblings) {
		if (node->hwid == hwid
		    && node->type == type && node->subtype == subtype) {
			return (node);
		}
	}

	node = malloc(sizeof(*node), M_TOPO, M_WAITOK);
	topo_init_node(node);
	node->parent = parent;
	node->hwid = hwid;
	node->type = type;
	node->subtype = subtype;
	TAILQ_INSERT_TAIL(&parent->children, node, siblings);
	parent->nchildren++;

	return (node);
}

/*
 * Find a child node with the given ID under the given parent.
 */
struct topo_node *
topo_find_node_by_hwid(struct topo_node *parent, int hwid,
    topo_node_type type, uintptr_t subtype)
{

	struct topo_node *node;

	TAILQ_FOREACH(node, &parent->children, siblings) {
		if (node->hwid == hwid && node->type == type && node->subtype == subtype) {
			return (node);
		}
	}
	return (NULL);
}

/*
 * Given a node change the order of its parent's child nodes such
 * that the node becomes the firt child while preserving the cyclic
 * order of the children.  In other words, the given node is promoted
 * by rotation.
 */
void
topo_promote_child(struct topo_node *child)
{
	struct topo_node *next;
	struct topo_node *node;
	struct topo_node *parent;

	parent = child->parent;
	next = TAILQ_NEXT(child, siblings);
	TAILQ_REMOVE(&parent->children, child, siblings);
	TAILQ_INSERT_HEAD(&parent->children, child, siblings);

	while (next != NULL) {
		node = next;
		next = TAILQ_NEXT(node, siblings);
		TAILQ_REMOVE(&parent->children, node, siblings);
		TAILQ_INSERT_AFTER(&parent->children, child, node, siblings);
		child = node;
	}
}

/*
 * Iterate to the next node in the depth-first search (traversal) of
 * the topology tree.
 */
struct topo_node *
topo_next_node(struct topo_node *top, struct topo_node *node)
{
	struct topo_node *next;

	if ((next = TAILQ_FIRST(&node->children)) != NULL) {
		return (next);
	}

	if ((next = TAILQ_NEXT(node, siblings)) != NULL) {
		return (next);
	}

	while (node != top && (node = node->parent) != top) {
		if ((next = TAILQ_NEXT(node, siblings)) != NULL) {
			return (next);
		}
	}

	return (NULL);
}

/*
 * Iterate to the next node in the depth-first search of the topology tree,
 * but without descending below the current node.
 */
struct topo_node *
topo_next_nonchild_node(struct topo_node *top, struct topo_node *node)
{
	struct topo_node *next;

	if ((next = TAILQ_NEXT(node, siblings)) != NULL) {
		return (next);
	}

	while (node != top && (node = node->parent) != top) {
		if ((next = TAILQ_NEXT(node, siblings)) != NULL) {
			return (next);
		}
	}
	return (NULL);
}

/*
 * Assign the given ID to the given topology node that represents a logical
 * processor.
 */
void
topo_set_pu_id(struct topo_node *node, cpuid_t id)
{
	KASSERT(node->type == TOPO_TYPE_PU);
	KASSERT(CPU_EMPTY(&node->cpuset) && node->cpu_count == 0);
	node->id = id;
	CPU_SET(&node->cpuset, id);
	node->cpu_count = 1;
	node->subtype = 1;

	while ((node = node->parent) != NULL) {
		KASSERT(!CPU_ISSET(&node->cpuset, id));
		CPU_SET(&node->cpuset, id);
		node->cpu_count++;
	}
}

static struct topology_spec {
	topo_node_type	type;
	bool		match_subtype;
	uintptr_t	subtype;
} topology_level_table[TOPO_LEVEL_COUNT] = {
	[TOPO_LEVEL_PKG] = { .type = TOPO_TYPE_PKG, },
	[TOPO_LEVEL_GROUP] = { .type = TOPO_TYPE_GROUP, },
	[TOPO_LEVEL_CACHEGROUP] = {
		.type = TOPO_TYPE_CACHE,
		.match_subtype = true,
		.subtype = CG_SHARE_L3,
	},
	[TOPO_LEVEL_CORE] = { .type = TOPO_TYPE_CORE, },
	[TOPO_LEVEL_THREAD] = { .type = TOPO_TYPE_PU, },
};

static bool_t
topo_analyze_table(struct topo_node *root, int all, enum topo_level level, struct topo_analysis *results)
{
	struct topology_spec *spec;
	struct topo_node *node;
	int count;

	if (level >= TOPO_LEVEL_COUNT)
		return (true);

	spec = &topology_level_table[level];
	count = 0;
	node = topo_next_node(root, root);

	while (node != NULL) {
		if (node->type != spec->type ||
		    (spec->match_subtype && node->subtype != spec->subtype)) {
			node = topo_next_node(root, node);
			continue;
		}
		if (!all && CPU_EMPTY(&node->cpuset)) {
			node = topo_next_nonchild_node(root, node);
			continue;
		}

		count++;

		if (!topo_analyze_table(node, all, level + 1, results))
			return (FALSE);

		node = topo_next_nonchild_node(root, node);
	}

	/* No explicit subgroups is essentially one subgroup. */
	if (count == 0) {
		count = 1;

		if (!topo_analyze_table(root, all, level + 1, results))
			return (FALSE);
	}

	if (results->entities[level] == -1)
		results->entities[level] = count;
	else if (results->entities[level] != count)
		return (FALSE);

	return (TRUE);
}

/*
 * Check if the topology is uniform, that is, each package has the same number
 * of cores in it and each core has the same number of threads (logical
 * processors) in it.  If so, calculate the number of packages, the number of
 * groups per package, the number of cachegroups per group, and the number of
 * logical processors per cachegroup.  'all' parameter tells whether to include
 * administratively disabled logical processors into the analysis.
 */
int
topo_analyze(struct topo_node *topo_root, int all, struct topo_analysis *results)
{
	results->entities[TOPO_LEVEL_PKG] = -1;
	results->entities[TOPO_LEVEL_CORE] = -1;
	results->entities[TOPO_LEVEL_THREAD] = -1;
	results->entities[TOPO_LEVEL_GROUP] = -1;
	results->entities[TOPO_LEVEL_CACHEGROUP] = -1;

	if (!topo_analyze_table(topo_root, all, TOPO_LEVEL_PKG, results)) {
		return (0);
	}

	KASSERT(results->entities[TOPO_LEVEL_PKG] > 0/*, ("bug in topology or analysis")*/);

	return (1);
}
#endif /* SMP */
