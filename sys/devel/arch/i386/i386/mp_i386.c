/*-
 * Copyright (c) 1996, by Steve Passe
 * Copyright (c) 2003, by Peter Wemm
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <devel/arch/i386/include/cpu.h>

/* lock region used by kernel profiling */
int	mcount_lock;

int	mp_naps;		/* # of Applications processors */
int	boot_cpu_id = -1;	/* designated BSP */

/* AP uses this during bootstrap.  Do not staticize.  */
char *bootSTK;
int bootAP;

/* Free these after use */
void *bootstacks[NCPUS];

/* Default cpu_ops implementation. */
struct cpu_ops cpu_ops;

/*
 * Store data from cpu_add() until later in the boot when we actually setup
 * the APs.
 */
struct cpu_info *cpu_info;
int *apic_cpuids;
int cpu_apic_ids[NCPUS];

static int pkg_id_shift;
static int node_id_shift;
static int core_id_shift;
static int disabled_cpus;

struct cache_info {
	int	id_shift;
	int	present;
} static caches[MAX_CACHE_LEVELS];

/*
 * Add a cache level to the cache topology description.
 */
static int
add_deterministic_cache(int type, int level, int share_count)
{
	if (type == 0) {
		return (0);
	}
	if (type > 3) {
		printf("unexpected cache type %d\n", type);
		return (1);
	}
	if (type == 2) { /* ignore instruction cache */
		return (1);
	}
	if (level == 0 || level > MAX_CACHE_LEVELS) {
		printf("unexpected cache level %d\n", type);
		return (1);
	}

	if (caches[level - 1].present) {
		printf("WARNING: multiple entries for L%u data cache\n", level);
		printf("%u => %u\n", caches[level - 1].id_shift, mask_width(share_count));
	}
	caches[level - 1].id_shift = mask_width(share_count);
	caches[level - 1].present = 1;

	if (caches[level - 1].id_shift > pkg_id_shift) {
		printf("WARNING: L%u data cache covers more "
				"APIC IDs than a package (%u > %u)\n", level,
				caches[level - 1].id_shift, pkg_id_shift);
		caches[level - 1].id_shift = pkg_id_shift;
	}
	if (caches[level - 1].id_shift < core_id_shift) {
		printf("WARNING: L%u data cache covers fewer "
				"APIC IDs than a core (%u < %u)\n", level, caches[level - 1].id_shift, core_id_shift);
		caches[level - 1].id_shift = core_id_shift;
	}

	return (1);
}

/*
 * Add a logical CPU to the topology.
 */
void
cpu_add(u_int apic_id, char boot_cpu)
{
	if (apic_id > max_apic_id) {
		panic("SMP: APIC ID %d too high", apic_id);
		return;
	}
	KASSERT(cpu_info[apic_id].cpu_present == 0, ("CPU %u added twice", apic_id));
	cpu_info[apic_id].cpu_present = 1;
	if (boot_cpu) {
		KASSERT(boot_cpu_id == -1, ("CPU %u claims to be BSP, but CPU %u already is", apic_id, boot_cpu_id));
		boot_cpu_id = apic_id;
		cpu_info[apic_id].cpu_bsp = 1;
	}
	if (bootverbose) {
		printf("SMP: Added CPU %u (%s)\n", apic_id, boot_cpu ? "BSP" : "AP");
	}
}
