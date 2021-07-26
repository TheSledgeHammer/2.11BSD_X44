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

#ifndef _SYS_SMP_H_
#define _SYS_SMP_H_

#include <devel/sys/cpuset.h>
#include <sys/queue.h>

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
	cpuset_t								cpuset;
	topo_node_type							type;
	uintptr_t								subtype;
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

#define	TOPO_FOREACH(i, root)	\
	for (i = root; i != NULL; i = topo_next_node(root, i))


#endif

extern u_int 		mp_maxid;
extern int 			mp_maxcpus;
extern int 			mp_ncores;
extern int 			mp_ncpus;
extern int 			smp_cpus;
extern volatile int smp_started;
extern int 			smp_threads_per_core;

extern cpuset_t 	all_cpus;
extern cpuset_t 	cpuset_domain[MAXMEMDOM]; 	/* CPUs in each NUMA domain. */

struct cpu_group {

};

/*
 * Macro allowing us to determine whether a CPU is absent at any given
 * time, thus permitting us to configure sparse maps of cpuid-dependent
 * (per-CPU) structures.
 */
#define	CPU_ABSENT(x_cpu)	(!CPU_ISSET(x_cpu, &all_cpus))

/*
 * Macros to iterate over non-absent CPUs.  CPU_FOREACH() takes an
 * integer iterator and iterates over the available set of CPUs.
 * CPU_FIRST() returns the id of the first non-absent CPU.  CPU_NEXT()
 * returns the id of the next non-absent CPU.  It will wrap back to
 * CPU_FIRST() once the end of the list is reached.  The iterators are
 * currently implemented via inline functions.
 */
#define	CPU_FOREACH(i)							\
	for ((i) = 0; (i) <= mp_maxid; (i)++) {		\
		if (!CPU_ABSENT((i))) {					\
		}										\
	}

static __inline int
cpu_first(void)
{
	int i;

	for (i = 0;; i++)
		if (!CPU_ABSENT(i))
			return (i);
}

static __inline int
cpu_next(int i)
{

	for (;;) {
		i++;
		if (i > mp_maxid)
			i = 0;
		if (!CPU_ABSENT(i))
			return (i);
	}
}

#define	CPU_FIRST()	cpu_first()
#define	CPU_NEXT(i)	cpu_next((i))
#endif /* _SYS_SMP_H_ */
