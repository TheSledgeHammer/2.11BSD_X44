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

/*
 * This module holds the global variables and machine independent functions
 * used for the kernel SMP support.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/smp.h>
#include <sys/user.h>

cpuset_t all_cpus;

int mp_ncpus;
/* export this for libkvm consumers. */
int mp_maxcpus = NCPUS;

int smp_disabled = 0;			/* has smp been disabled? */
int smp_cpus = 1;				/* how many cpu's running */
int smp_threads_per_core = 1;	/* how many SMT threads are running per core */
int mp_ncores = -1;				/* how many physical cores running */
int smp_topology = 0;			/* Which topology we're using. */

/*
 * Let the MD SMP code initialize mp_maxid very early if it can.
 */
static void
mp_setmaxid(void *dummy)
{
	cpu_mp_setmaxid();
	KASSERT(mp_ncpus >= 1 ("%s: CPU count < 1", __func__));
	KASSERT(mp_ncpus > 1 || mp_maxid == 0 ("%s: one CPU but mp_maxid is not zero", __func__));
	KASSERT(mp_maxid >= mp_ncpus - 1 ("%s: counters out of sync: max %d, count %d", __func__, mp_maxid, mp_ncpus));
}

/*
 * Call the MD SMP initialization code.
 */
static void
mp_start(pc)
	struct percpu *pc;
{
	if (smp_disabled != 0 || cpu_mp_probe() == 0) {
		mp_ncores = 1;
		mp_ncpus = 1;
		//CPU_SETOF(PERCPU_GET(pc, cpuid), &all_cpus);
		return;
	}

	cpu_mp_start(pc); /* TODO: add percpu here */
	printf("FreeBSD/SMP: Multiprocessor System Detected: %d CPUs\n", mp_ncpus);

	if (mp_ncores < 0) {
		mp_ncores = mp_ncpus;
	}

	cpu_mp_announce();
}

//#ifdef SMP
void
topo_init_node(struct topo_node *node)
{
	bzero(node, sizeof(*node));
	TAILQ_INIT(&node->children);
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

	KASSERT(node->type == TOPO_TYPE_PU ("topo_set_pu_id: wrong node type: %u", node->type));
	KASSERT(CPU_EMPTY(&node->cpuset) && node->cpu_count == 0 ("topo_set_pu_id: cpuset already not empty"));
	node->id = id;
	CPU_SET(id, &node->cpuset);
	node->cpu_count = 1;
	node->subtype = 1;

	while ((node = node->parent) != NULL) {
		KASSERT(!CPU_ISSET(id, &node->cpuset)("logical ID %u is already set in node %p", id, node));
		CPU_SET(id, &node->cpuset);
		node->cpu_count++;
	}
}

#endif /* SMP */
