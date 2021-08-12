/*
 * smp_machdep.c
 *
 *  Created on: 11 Aug 2021
 *      Author: marti
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/memrange.h>
#include <sys/proc.h>
#include <sys/sysctl.h>

#include <vm/include/vm.h>
#include <vm/include/vm_param.h>
#include <vm/include/pmap.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_extern.h>

#include <devel/sys/smp.h>
#include <arch/i386/include/cpu.h>
#include <arch/i386/include/cputypes.h>
#include <arch/i386/include/pcb.h>
#include <arch/i386/include/psl.h>
#include <arch/i386/include/pte.h>
#include <arch/i386/include/param.h>
#include <arch/i386/include/specialreg.h>
#include <arch/i386/include/vmparam.h>
#include <arch/i386/include/vm86.h>

#include <devel/arch/i386/include/smp.h>
#include <devel/arch/i386/include/percpu.h>
#include "../../../../sys/percpu.h"

/* FreeBSD 5.x SMP (Possibility) */
struct cpu_group {
	u_int					cg_mask;
	int						cg_count;
	int						cg_children;
	struct cpu_group 		*cg_child;
};

struct cpu_top {
	int						ct_count;		/* Count of groups. */
	struct cpu_group 		*ct_group;		/* Array of pointers to cpu groups. */
};

extern struct cpu_top 	*smp_topology;
extern void 			(*cpustop_restartfunc)(void);
extern int 				smp_active;
extern int 				smp_cpus;
extern volatile u_int  	started_cpus;
extern volatile u_int  	stopped_cpus;
extern u_int  			idle_cpus_mask;
extern u_int  			hlt_cpus_mask;
extern u_int  			logical_cpus_mask;

/*
 * CPU topology map datastructures for HTT.
 */
static struct cpu_group mp_groups[NCPUS];
static struct cpu_top 	mp_top;

static u_int 	logical_cpus;
static int		hlt_logical_cpus;
static u_int	hyperthreading_cpus;
static u_int	hyperthreading_cpus_mask;

void
mp_topology(void)
{
	struct cpu_group *group;
	int logical_cpus;
	int apic_id;
	int groups;
	int cpu;

	/* Build the smp_topology map. */
	/* Nothing to do if there is no HTT support. */
	if ((cpu_feature & CPUID_HTT) == 0)
		return;
	logical_cpus = (cpu_procinfo & CPUID_HTT_CORES) >> 16;
	if (logical_cpus <= 1)
		return;
	group = &mp_groups[0];
	groups = 1;
	for (cpu = 0, apic_id = 0; apic_id < NCPUS; apic_id++) {
		if (!cpu_info[apic_id].cpu_present)
			continue;
		/*
		 * If the current group has members and we're not a logical
		 * cpu, create a new group.
		 */
		if (group->cg_count != 0 && (apic_id % logical_cpus) == 0) {
			group++;
			groups++;
		}
		group->cg_count++;
		group->cg_mask |= 1 << cpu;
		cpu++;
	}

	mp_top.ct_count = groups;
	mp_top.ct_group = mp_groups;
	smp_topology = &mp_top;
}

void
logical_cpu_probe()
{
	u_int threads_per_cache, p[4];

	/* Setup the initial logical CPUs info. */
	logical_cpus = logical_cpus_mask = 0;
	if (cpu_feature & CPUID_HTT)
		logical_cpus = (cpu_procinfo & CPUID_HTT_CORES) >> 16;

	/*
	 * Work out if hyperthreading is *really* enabled.  This
	 * is made really ugly by the fact that processors lie: Dual
	 * core processors claim to be hyperthreaded even when they're
	 * not, presumably because they want to be treated the same
	 * way as HTT with respect to per-cpu software licensing.
	 * At the time of writing (May 12, 2005) the only hyperthreaded
	 * cpus are from Intel, and Intel's dual-core processors can be
	 * identified via the "deterministic cache parameters" cpuid
	 * calls.
	 */
	/*
	 * First determine if this is an Intel processor which claims
	 * to have hyperthreading support.
	 */
	if ((cpu_feature & CPUID_HTT)
			&& (strcmp(cpu_vendor, "GenuineIntel") == 0)) {
		/*
		 * If the "deterministic cache parameters" cpuid calls
		 * are available, use them.
		 */
		if (cpu_high >= 4) {
			/* Ask the processor about up to 32 caches. */
			for (i = 0; i < 32; i++) {
				cpuid_count(4, i, p);
				threads_per_cache = ((p[0] & 0x3ffc000) >> 14) + 1;
				if (hyperthreading_cpus < threads_per_cache)
					hyperthreading_cpus = threads_per_cache;
				if ((p[0] & 0x1f) == 0)
					break;
			}
		}

		/*
		 * If the deterministic cache parameters are not
		 * available, or if no caches were reported to exist,
		 * just accept what the HTT flag indicated.
		 */
		if (hyperthreading_cpus == 0)
			hyperthreading_cpus = logical_cpus;
	}
}

/*
 * AP CPU's call this to initialize themselves.
 */
void
init_secondary(void)
{
	/* Determine if we are a logical CPU. */
	if (logical_cpus > 1 && PERCPU_GET(pc, apic_id) % logical_cpus != 0)
		logical_cpus_mask |= PERCPU_GET(pc, cpumask);

	/* Determine if we are a hyperthread. */
	if (hyperthreading_cpus > 1 && PERCPU_GET(pc, apic_id) % hyperthreading_cpus != 0) {
		hyperthreading_cpus_mask |= PERCPU_GET(pc, cpumask);
	}
}
