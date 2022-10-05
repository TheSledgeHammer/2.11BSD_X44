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
#include <sys/malloc.h>
#include <sys/memrange.h>
#include <sys/cputopo.h>
#include <sys/percpu.h>

#include <vm/include/vm_param.h>

#include <machine/bus.h>
#include <machine/cpuinfo.h>
#include <machine/cpufunc.h>
#include <machine/cputypes.h>
#include <machine/cpuvar.h>
#include <machine/apic/lapicvar.h>
#include <machine/param.h>
#include <machine/smp.h>
#include <machine/specialreg.h>
#include <machine/vmparam.h>
//#include <machine/percpu.h>

/* lock region used by kernel profiling */
int	mcount_lock;

int	mp_naps;			/* # of Applications processors */
int	boot_cpu_id = -1;	/* designated BSP */

/* AP uses this during bootstrap.  Do not staticize.  */
char *bootSTK;
int bootAP;


/* Free these after use */
void *bootstacks[NCPUS];

/* Default cpu_ops implementation. */
struct cpu_ops cpu_ops;

/* Set to 1 once we're ready to let the APs out of the pen. */
volatile int aps_ready = 0;

/*
 * Store data from cpu_add() until later in the boot when we actually setup
 * the APs.
 */
int cpu_apic_ids[NCPUS];

static int hyperthreading_allowed = 1;
static int hyperthreading_intr_allowed = 0;
static struct topo_node topo_root;

static int pkg_id_shift;
static int node_id_shift;
static int core_id_shift;
static int disabled_cpus;

struct cache_info {
	int	id_shift;
	int	present;
} static caches[MAX_CACHE_LEVELS];

unsigned int boot_address;
u_int max_apic_id;

void	intr_add_cpu(u_int);

#define MiB(v)	(v ## ULL << 20)

void
mem_range_AP_init(void)
{
	if (mem_range_softc.mr_op && mem_range_softc.mr_op->initAP) {
		mem_range_softc.mr_op->initAP(&mem_range_softc);
	}
}

/*
 * Round up to the next power of two, if necessary, and then
 * take log2.
 * Returns -1 if argument is zero.
 */
static __inline int
mask_width(u_int x)
{
	return (fls(x << (1 - powerof2(x))) - 1);
}

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
 * Determine topology of processing units and caches for AMD CPUs.
 * See:
 *  - AMD CPUID Specification (Publication # 25481)
 *  - BKDG for AMD NPT Family 0Fh Processors (Publication # 32559)
 *  - BKDG For AMD Family 10h Processors (Publication # 31116)
 *  - BKDG For AMD Family 15h Models 00h-0Fh Processors (Publication # 42301)
 *  - BKDG For AMD Family 16h Models 00h-0Fh Processors (Publication # 48751)
 *  - PPR For AMD Family 17h Models 00h-0Fh Processors (Publication # 54945)
 */
static void
topo_probe_amd(void)
{
	u_int p[4];
	uint64_t v;
	int level;
	int nodes_per_socket;
	int share_count;
	int type;
	int i;

	/* No multi-core capability. */
	if ((amd_feature2 & AMDID2_CMP) == 0) {
		return;
	}

	/* For families 10h and newer. */
	pkg_id_shift = (cpu_procinfo2 & AMDID_COREID_SIZE) >>
	    AMDID_COREID_SIZE_SHIFT;

	/* For 0Fh family. */
	if (pkg_id_shift == 0) {
		pkg_id_shift = mask_width((cpu_procinfo2 & AMDID_CMP_CORES) + 1);
	}

	/*
	 * Families prior to 16h define the following value as
	 * cores per compute unit and we don't really care about the AMD
	 * compute units at the moment.  Perhaps we should treat them as
	 * cores and cores within the compute units as hardware threads,
	 * but that's up for debate.
	 * Later families define the value as threads per compute unit,
	 * so we are following AMD's nomenclature here.
	 */
	if ((amd_feature2 & AMDID2_TOPOLOGY) != 0 &&
	    CPUID_TO_FAMILY(cpu_id) >= 0x16) {
		cpuid_count(0x8000001e, 0, p);
		share_count = ((p[1] >> 8) & 0xff) + 1;
		core_id_shift = mask_width(share_count);

		/*
		 * For Zen (17h), gather Nodes per Processor.  Each node is a
		 * Zeppelin die; TR and EPYC CPUs will have multiple dies per
		 * package.  Communication latency between dies is higher than
		 * within them.
		 */
		nodes_per_socket = ((p[2] >> 8) & 0x7) + 1;
		node_id_shift = pkg_id_shift - mask_width(nodes_per_socket);
	}

	if ((amd_feature2 & AMDID2_TOPOLOGY) != 0) {
		for (i = 0; ; i++) {
			cpuid_count(0x8000001d, i, p);
			type = p[0] & 0x1f;
			level = (p[0] >> 5) & 0x7;
			share_count = 1 + ((p[0] >> 14) & 0xfff);

			if (!add_deterministic_cache(type, level, share_count)) {
				break;
			}
		}
	} else {
		if (cpu_exthigh >= 0x80000005) {
			cpuid_count(0x80000005, 0, p);
			if (((p[2] >> 24) & 0xff) != 0) {
				caches[0].id_shift = 0;
				caches[0].present = 1;
			}
		}
		if (cpu_exthigh >= 0x80000006) {
			cpuid_count(0x80000006, 0, p);
			if (((p[2] >> 16) & 0xffff) != 0) {
				caches[1].id_shift = 0;
				caches[1].present = 1;
			}
			if (((p[3] >> 18) & 0x3fff) != 0) {
				nodes_per_socket = 1;
				if ((amd_feature2 & AMDID2_NODE_ID) != 0) {
					/*
					 * Handle multi-node processors that
					 * have multiple chips, each with its
					 * own L3 cache, on the same die.
					 */
					v = rdmsr(0xc001100c);
					nodes_per_socket = 1 + ((v >> 3) & 0x7);
				}
				caches[2].id_shift =
				    pkg_id_shift - mask_width(nodes_per_socket);
				caches[2].present = 1;
			}
		}
	}
}

/*
 * Determine topology of processing units for Intel CPUs
 * using CPUID Leaf 1 and Leaf 4, if supported.
 * See:
 *  - Intel 64 Architecture Processor Topology Enumeration
 *  - Intel 64 and IA-32 ArchitecturesSoftware Developer’s Manual,
 *    Volume 3A: System Programming Guide, PROGRAMMING CONSIDERATIONS
 *    FOR HARDWARE MULTI-THREADING CAPABLE PROCESSORS
 */
static void
topo_probe_intel_0x4(void)
{
	u_int p[4];
	int max_cores;
	int max_logical;

	/* Both zero and one here mean one logical processor per package. */
	max_logical = (cpu_feature & CPUID_HTT) != 0 ? (cpu_procinfo & CPUID_HTT_CORES) >> 16 : 1;
	if (max_logical <= 1) {
		return;
	}

	if (cpu_high >= 0x4) {
		cpuid_count(0x04, 0, p);
		max_cores = ((p[0] >> 26) & 0x3f) + 1;
	} else {
		max_cores = 1;
	}

	core_id_shift = mask_width(max_logical/max_cores);
	KASSERT(core_id_shift >= 0 /*("intel topo: max_cores > max_logical\n")*/);
	pkg_id_shift = core_id_shift + mask_width(max_cores);
}

/*
 * Determine topology of processing units for Intel CPUs
 * using CPUID Leaf 11, if supported.
 * See:
 *  - Intel 64 Architecture Processor Topology Enumeration
 *  - Intel 64 and IA-32 ArchitecturesSoftware Developer’s Manual,
 *    Volume 3A: System Programming Guide, PROGRAMMING CONSIDERATIONS
 *    FOR HARDWARE MULTI-THREADING CAPABLE PROCESSORS
 */
static void
topo_probe_intel_0xb(void)
{
	u_int p[4];
	int bits;
	int type;
	int i;

	/* Fall back if CPU leaf 11 doesn't really exist. */
	cpuid_count(0x0b, 0, p);
	if (p[1] == 0) {
		topo_probe_intel_0x4();
		return;
	}
	/* We only support three levels for now. */
	for (i = 0;; i++) {
		cpuid_count(0x0b, i, p);

		bits = p[0] & 0x1f;
		type = (p[2] >> 8) & 0xff;

		if (type == 0) {
			break;
		}

		/* TODO: check for duplicate (re-)assignment */
		if (type == CPUID_TYPE_SMT) {
			core_id_shift = bits;
		} else if (type == CPUID_TYPE_CORE) {
			pkg_id_shift = bits;
		} else {
			printf("unknown CPU level type %d\n", type);
		}
	}

	if (pkg_id_shift < core_id_shift) {
		printf("WARNING: core covers more APIC IDs than a package\n");
		core_id_shift = pkg_id_shift;
	}
}

/*
 * Determine topology of caches for Intel CPUs.
 * See:
 *  - Intel 64 Architecture Processor Topology Enumeration
 *  - Intel 64 and IA-32 Architectures Software Developer’s Manual
 *    Volume 2A: Instruction Set Reference, A-M,
 *    CPUID instruction
 */
static void
topo_probe_intel_caches(void)
{
	u_int p[4];
	int level;
	int share_count;
	int type;
	int i;

	if (cpu_high < 0x4) {
		/*
		 * Available cache level and sizes can be determined
		 * via CPUID leaf 2, but that requires a huge table of hardcoded
		 * values, so for now just assume L1 and L2 caches potentially
		 * shared only by HTT processing units, if HTT is present.
		 */
		caches[0].id_shift = pkg_id_shift;
		caches[0].present = 1;
		caches[1].id_shift = pkg_id_shift;
		caches[1].present = 1;
		return;
	}

	for (i = 0;; i++) {
		cpuid_count(0x4, i, p);
		type = p[0] & 0x1f;
		level = (p[0] >> 5) & 0x7;
		share_count = 1 + ((p[0] >> 14) & 0xfff);

		if (!add_deterministic_cache(type, level, share_count)) {
			break;
		}
	}
}

/*
 * Determine topology of processing units and caches for Intel CPUs.
 * See:
 *  - Intel 64 Architecture Processor Topology Enumeration
 */
static void
topo_probe_intel(void)
{

	/*
	 * Note that 0x1 <= cpu_high < 4 case should be
	 * compatible with topo_probe_intel_0x4() logic when
	 * CPUID.1:EBX[23:16] > 0 (cpu_cores will be 1)
	 * or it should trigger the fallback otherwise.
	 */
	if (cpu_high >= 0xb) {
		topo_probe_intel_0xb();
	} else if (cpu_high >= 0x1) {
		topo_probe_intel_0x4();
	}

	topo_probe_intel_caches();
}

/*
 * Topology information is queried only on BSP, on which this
 * code runs and for which it can query CPUID information.
 * Then topology is extrapolated on all packages using an
 * assumption that APIC ID to hardware component ID mapping is
 * homogenious.
 * That doesn't necesserily imply that the topology is uniform.
 */
void
topo_probe(void)
{
	static int cpu_topo_probed = 0;
	struct i386_topo_layer {
		int type;
		int subtype;
		int id_shift;
	} topo_layers[MAX_CACHE_LEVELS + 4];

	struct topo_node *parent;
	struct topo_node *node;
	int layer;
	int nlayers;
	int node_id;
	int i;

	if (cpu_topo_probed) {
		return;
	}
	if (mp_ncpus <= 1) {
		; /* nothing */
	} else if (cpu_vendor_id == CPUVENDOR_AMD
			|| cpu_vendor_id == CPUVENDOR_HYGON) {
		topo_probe_amd();
	} else if (cpu_vendor_id == CPUVENDOR_INTEL) {
		topo_probe_intel();
	}

	KASSERT(pkg_id_shift >= core_id_shift /*("bug in APIC topology discovery")*/);

	nlayers = 0;
	bzero(topo_layers, sizeof(topo_layers));

	topo_layers[nlayers].type = TOPO_TYPE_PKG;
	topo_layers[nlayers].id_shift = pkg_id_shift;
	if (bootverbose)
		printf("Package ID shift: %u\n", topo_layers[nlayers].id_shift);
	nlayers++;

	if (pkg_id_shift > node_id_shift && node_id_shift != 0) {
		topo_layers[nlayers].type = TOPO_TYPE_GROUP;
		topo_layers[nlayers].id_shift = node_id_shift;
		if (bootverbose)
			printf("Node ID shift: %u\n", topo_layers[nlayers].id_shift);
		nlayers++;
	}

	/*
	 * Consider all caches to be within a package/chip
	 * and "in front" of all sub-components like
	 * cores and hardware threads.
	 */
	for (i = MAX_CACHE_LEVELS - 1; i >= 0; --i) {
		if (caches[i].present) {
			if (node_id_shift != 0) {
				KASSERT(caches[i].id_shift <= node_id_shift /*("bug in APIC topology discovery")*/);
			}
			KASSERT(caches[i].id_shift <= pkg_id_shift /*("bug in APIC topology discovery")*/);
			KASSERT(caches[i].id_shift >= core_id_shift /*("bug in APIC topology discovery")*/);

			topo_layers[nlayers].type = TOPO_TYPE_CACHE;
			topo_layers[nlayers].subtype = i + 1;
			topo_layers[nlayers].id_shift = caches[i].id_shift;
			if (bootverbose) {
				printf("L%u cache ID shift: %u\n", topo_layers[nlayers].subtype, topo_layers[nlayers].id_shift);
			}
			nlayers++;
		}
	}

	if (pkg_id_shift > core_id_shift) {
		topo_layers[nlayers].type = TOPO_TYPE_CORE;
		topo_layers[nlayers].id_shift = core_id_shift;
		if (bootverbose) {
			printf("Core ID shift: %u\n", topo_layers[nlayers].id_shift);
		}
		nlayers++;
	}

	topo_layers[nlayers].type = TOPO_TYPE_PU;
	topo_layers[nlayers].id_shift = 0;
	nlayers++;

	topo_init_root(&topo_root);
	for (i = 0; i <= max_apic_id; ++i) {
		if (!cpu_info[i].cpu_present) {
			continue;
		}

		parent = &topo_root;
		for (layer = 0; layer < nlayers; ++layer) {
			node_id = i >> topo_layers[layer].id_shift;
			parent = topo_add_node_by_hwid(parent, node_id, topo_layers[layer].type, topo_layers[layer].subtype);
		}
	}

	parent = &topo_root;
	for (layer = 0; layer < nlayers; ++layer) {
		node_id = boot_cpu_id >> topo_layers[layer].id_shift;
		node = topo_find_node_by_hwid(parent, node_id, topo_layers[layer].type, topo_layers[layer].subtype);
		topo_promote_child(node);
		parent = node;
	}

	cpu_topo_probed = 1;
}

/*
 * Assign logical CPU IDs to local APICs.
 */
void
assign_cpu_ids(void)
{
	struct topo_node *node;
	u_int smt_mask;
	int nhyper;

	smt_mask = (1u << core_id_shift) - 1;

	/*
	 * Assign CPU IDs to local APIC IDs and disable any CPUs
	 * beyond MAXCPU.  CPU 0 is always assigned to the BSP.
	 */
	mp_ncpus = 0;
	nhyper = 0;
	TOPO_FOREACH(node, &topo_root) {
		if (node->type != TOPO_TYPE_PU)
			continue;

		if ((node->hwid & smt_mask) != (boot_cpu_id & smt_mask)) {
			cpu_info[node->hwid].cpu_hyperthread = 1;
		}

		if (node->hwid != boot_cpu_id) {
			cpu_info[node->hwid].cpu_disabled = 1;
		} else {
			printf("Cannot disable BSP, APIC ID = %d\n", node->hwid);
		}

		if (!hyperthreading_allowed && cpu_info[node->hwid].cpu_hyperthread) {
			cpu_info[node->hwid].cpu_disabled = 1;
		}

		if (mp_ncpus >= NCPUS) {
			cpu_info[node->hwid].cpu_disabled = 1;
		}

		if (cpu_info[node->hwid].cpu_disabled) {
			disabled_cpus++;
			continue;
		}

		if (cpu_info[node->hwid].cpu_hyperthread) {
			nhyper++;
		}

		cpu_apic_ids[mp_ncpus] = node->hwid;
		apic_cpuids[node->hwid] = mp_ncpus;
		topo_set_pu_id(node, mp_ncpus);
		mp_ncpus++;
	}

	KASSERT(mp_maxid >= mp_ncpus - 1 /*("%s: counters out of sync: max %d, count %d", __func__, mp_maxid, mp_ncpus)*/);

	mp_ncores = mp_ncpus - nhyper;
	smp_threads_per_core = mp_ncpus / mp_ncores;
}

/*
 * Print various information about the SMP system hardware and setup.
 */
void
cpu_mp_announce(void)
{
	struct topo_node *node;
	const char *hyperthread;
	struct topo_analysis topology;

	printf("211BSD/SMP: ");
	if (topo_analyze(&topo_root, 1, &topology)) {
		printf("%d package(s)", topology.entities[TOPO_LEVEL_PKG]);
		if (topology.entities[TOPO_LEVEL_GROUP] > 1)
			printf(" x %d groups", topology.entities[TOPO_LEVEL_GROUP]);
		if (topology.entities[TOPO_LEVEL_CACHEGROUP] > 1)
			printf(" x %d cache groups",
					topology.entities[TOPO_LEVEL_CACHEGROUP]);
		if (topology.entities[TOPO_LEVEL_CORE] > 0)
			printf(" x %d core(s)", topology.entities[TOPO_LEVEL_CORE]);
		if (topology.entities[TOPO_LEVEL_THREAD] > 1)
			printf(" x %d hardware threads",
					topology.entities[TOPO_LEVEL_THREAD]);
	} else {
		printf("Non-uniform topology");
	}
	printf("\n");

	if (disabled_cpus) {
		printf("211BSD/SMP Online: ");
		if (topo_analyze(&topo_root, 0, &topology)) {
			printf("%d package(s)", topology.entities[TOPO_LEVEL_PKG]);
			if (topology.entities[TOPO_LEVEL_GROUP] > 1)
				printf(" x %d groups", topology.entities[TOPO_LEVEL_GROUP]);
			if (topology.entities[TOPO_LEVEL_CACHEGROUP] > 1)
				printf(" x %d cache groups",
						topology.entities[TOPO_LEVEL_CACHEGROUP]);
			if (topology.entities[TOPO_LEVEL_CORE] > 0)
				printf(" x %d core(s)", topology.entities[TOPO_LEVEL_CORE]);
			if (topology.entities[TOPO_LEVEL_THREAD] > 1)
				printf(" x %d hardware threads",
						topology.entities[TOPO_LEVEL_THREAD]);
		} else {
			printf("Non-uniform topology");
		}
		printf("\n");
	}

	if (!bootverbose)
		return;

	TOPO_FOREACH(node, &topo_root) {
		switch (node->type) {
		case TOPO_TYPE_PKG:
			printf("Package HW ID = %u\n", node->hwid);
			break;
		case TOPO_TYPE_CORE:
			printf("\tCore HW ID = %u\n", node->hwid);
			break;
		case TOPO_TYPE_PU:
			if (cpu_info[node->hwid].cpu_hyperthread)
				hyperthread = "/HT";
			else
				hyperthread = "";

			if (node->subtype == 0)
				printf("\t\tCPU (AP%s): APIC ID: %u"
						"(disabled)\n", hyperthread, node->hwid);
			else if (node->id == 0)
				printf("\t\tCPU0 (BSP): APIC ID: %u\n", node->hwid);
			else
				printf("\t\tCPU%u (AP%s): APIC ID: %u\n", node->id, hyperthread,
						node->hwid);
			break;
		default:
			/* ignored */
			break;
		}
	}
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
	KASSERT(cpu_info[apic_id].cpu_present == 0 /*("CPU %u added twice", apic_id)*/);
	cpu_info[apic_id].cpu_present = 1;
	if (boot_cpu) {
		KASSERT(boot_cpu_id == -1 /*("CPU %u claims to be BSP, but CPU %u already is", apic_id, boot_cpu_id)*/);
		boot_cpu_id = apic_id;
		cpu_info[apic_id].cpu_bsp = 1;
	}
	mp_ncpus++;
	if (bootverbose) {
		printf("SMP: Added CPU %u (%s)\n", apic_id, boot_cpu ? "BSP" : "AP");
	}
}

void
cpu_mp_setmaxid(void)
{
	/*
	 * mp_ncpus and mp_maxid should be already set by calls to cpu_add().
	 * If there were no calls to cpu_add() assume this is a UP system.
	 */
	mp_maxid = NCPUS - 1;
}

int
cpu_mp_probe(void)
{
	/*
	 * Always record BSP in CPU map so that the mbuf init code works
	 * correctly.
	 */
	all_cpus = 1;
	if (mp_ncpus == 0) {
		/*
		 * No CPUs were found, so this must be a UP system.  Setup
		 * the variables to represent a system with a single CPU
		 * with an id of 0.
		 */
		mp_ncpus = 1;
		return (0);
	}

	/* At least one CPU was found. */
	if (mp_ncpus == 1) {
		/*
		 * One CPU was found, so this must be a UP system with
		 * an I/O APIC.
		 */
		return (0);
	}

	/* At least two CPUs were found. */
	return (1);
}

/* Allocate memory for the AP trampoline. */
void
alloc_ap_trampoline(basemem, seg_start, seg_end)
	int *basemem;
	u_int64_t seg_start, seg_end;
{
	int error;
	bool allocated;

	allocated = TRUE;
	if((seg_end >= MiB(1)) || (trunc_page(seg_end) - round_page(seg_start) < round_page(bootMP_size))) {
		allocated = TRUE;
		if(seg_end < MiB(1)) {
			boot_address = trunc_page(seg_end);
			if ((seg_end - boot_address) < bootMP_size) {
				boot_address -= round_page(bootMP_size);
			}
			seg_end = boot_address;
		} else {
			boot_address = round_page(seg_start);
			seg_start = boot_address + round_page(bootMP_size);
		}
	}

	error = add_mem_cluster(seg_start, seg_end);
	if(error) {
		allocated = FALSE;
	}

	if (!allocated) {
		boot_address = (int)basemem * 1024 - bootMP_size;
		if (bootverbose) {
			printf("Cannot find enough space for the boot trampoline, placing it at %#x", boot_address);
		}
	}
}

/*
 * AP CPU's call this to initialize themselves.
 */
void
init_secondary_tail(pc)
	struct percpu *pc;
{
	u_int cpuid;

	/* Initialize the PAT MSR. */
	pmap_init_pat();

	/* set up CPU registers and state */
	i386_proc0_tss_ldt_init();

	/* set up SSE/NX */
	initializecpu();

	if (cpu_ops.cpu_init) {
		cpu_ops.cpu_init();
	}

	/* A quick check from sanity claus */
	cpuid = PERCPU_GET(cpuid);
	if (PERCPU_GET(apic_id) != lapic_cpu_number()) {
		printf("SMP: cpuid = %d\n", cpuid);
		printf("SMP: actual apic_id = %d\n", lapic_cpu_number());
		printf("SMP: correct apic_id = %d\n", PERCPU_GET(apic_id));
		panic("cpuid mismatch! boom!!");
	}

	/* Set memory range attributes for this CPU to match the BSP */
	mem_range_AP_init();

	smp_cpus++;

	if (bootverbose) {
		printf("SMP: AP CPU #%d Launched!\n", cpuid);
	} else {
		printf("%s%d%s", smp_cpus == 2 ? "Launching APs: " : "", cpuid, smp_cpus == mp_ncpus ? "\n" : " ");
	}

	/* Determine if we are a hyperthread. */
	if (cpu_info[PERCPU_GET(apic_id)].cpu_hyperthread) {
		CPU_SET(cpu_info[PERCPU_GET(apic_id)].cpu_topo, cpuid);
	}

	if (bootverbose)
		lapic_dump();

	if (smp_cpus == mp_ncpus) {
		/* enable IPI's, tlb shootdown, freezes etc */
		smp_started = 1;
	}

#ifdef __amd64__
	/*
	 * Enable global pages TLB extension
	 * This also implicitly flushes the TLB
	 */
	/*
	load_cr4(rcr4() | CR4_PGE);
	if (pmap_pcid_enabled) {
		lcr4(rcr4() | CR4_PCIDE);
	}
	load_ds(_udatasel);
	load_es(_udatasel);
	load_fs(_ufssel);
	*/
#endif

	/* Wait until all the AP's are up. */
	while (smp_started == 0) {
		ia32_pause();
	}
}

/*
 * We tell the I/O APIC code about all the CPUs we want to receive
 * interrupts.  If we don't want certain CPUs to receive IRQs we
 * can simply not tell the I/O APIC code about them in this function.
 * We also do not tell it about the BSP since it tells itself about
 * the BSP internally to work with UP kernels and on UP machines.
 */
void
set_interrupt_apic_ids(void)
{
	u_int i, apic_id;

	for (i = 0; i < NCPUS; i++) {
		apic_id = cpu_apic_ids[i];
		if (apic_id == -1) {
			continue;
		}
		if (cpu_info[apic_id].cpu_bsp) {
			continue;
		}
		if (cpu_info[apic_id].cpu_disabled) {
			continue;
		}
		/* Don't let hyperthreads service interrupts. */
		if (cpu_info[apic_id].cpu_hyperthread && !hyperthreading_intr_allowed) {
			continue;
		}
		intr_add_cpu(i);
	}
}

/*
 * Add a CPU to our mask of valid CPUs that can be destinations of
 * interrupts.
 */
void
intr_add_cpu(u_int cpu)
{
	if (cpu >= NCPUS) {
		panic("%s: Invalid CPU ID", __func__);
	}
	if (bootverbose) {
		printf("INTR: Adding local APIC %d as a target\n", cpu_apic_ids[cpu]);
	}
	CPU_SET(cpu_info[cpu].cpu_topo, cpu);
}
