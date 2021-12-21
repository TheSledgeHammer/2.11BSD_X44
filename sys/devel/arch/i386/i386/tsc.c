/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1998-2003 Poul-Henning Kamp
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
/* __FBSDID("$FreeBSD$"); */

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/stdint.h>
#include <sys/types.h>

#include <devel/sys/timetc.h>
#include <arch/i386/include/cpu.h>

#include <arch/i386/include/cpu.h>
#include <arch/i386/include/cpufunc.h>
#include <arch/i386/include/cputypes.h>
#include <arch/i386/include/specialreg.h>

#include <arch/i386/include/ansi.h>
#include "arch/i386/include/types.h"

u_int64_t	tsc_freq;
int			tsc_is_invariant;
int			tsc_perf_stat;

#ifdef SMP
int	smp_tsc;
int	smp_tsc_adjust = 0;
#endif
static int	tsc_shift = 1;
static int	tsc_disabled;
static int	tsc_skip_calibration;

static struct timecounter tsc_timecounter = {
		.tc_get_timecount =			tsc_get_timecount,
		.tc_counter_mask =			~0u,
		.tc_name =					"TSC",
		.tc_quality =				800,	/* adjusted in code */
};

/*
 * Calculate TSC frequency using information from the CPUID leaf 0x15
 * 'Time Stamp Counter and Nominal Core Crystal Clock'.  If leaf 0x15
 * is not functional, as it is on Skylake/Kabylake, try 0x16 'Processor
 * Frequency Information'.  Leaf 0x16 is described in the SDM as
 * informational only, but if 0x15 did not work, and TSC calibration
 * is disabled, it is the best we can get at all.  It should still be
 * an improvement over the parsing of the CPU model name in
 * tsc_freq_intel(), when available.
 */
static bool_t
tsc_freq_cpuid(u_int64_t *res)
{
	u_int regs[4];

	if (cpu_high < 0x15)
		return (FALSE);
	do_cpuid(0x15, regs);
	if (regs[0] != 0 && regs[1] != 0 && regs[2] != 0) {
		*res = (uint64_t)regs[2] * regs[1] / regs[0];
		return (TRUE);
	}

	if (cpu_high < 0x16)
		return (FALSE);
	do_cpuid(0x16, regs);
	if (regs[0] != 0) {
		*res = (uint64_t)regs[0] * 1000000;
		return (TRUE);
	}

	return (FALSE);
}

static void
tsc_freq_intel(void)
{
	char brand[48];
	u_int regs[4];
	u_int64_t freq;
	char *p;
	u_int i;

	/*
	 * Intel Processor Identification and the CPUID Instruction
	 * Application Note 485.
	 * http://www.intel.com/assets/pdf/appnote/241618.pdf
	 */
	if (cpu_exthigh >= 0x80000004) {
		p = brand;
		for (i = 0x80000002; i < 0x80000005; i++) {
			do_cpuid(i, regs);
			memcpy(p, regs, sizeof(regs));
			p += sizeof(regs);
		}
		p = NULL;
		for (i = 0; i < sizeof(brand) - 1; i++)
			if (brand[i] == 'H' && brand[i + 1] == 'z')
				p = brand + i;
		if (p != NULL) {
			p -= 5;
			switch (p[4]) {
			case 'M':
				i = 1;
				break;
			case 'G':
				i = 1000;
				break;
			case 'T':
				i = 1000000;
				break;
			default:
				return;
			}
#define	C2D(c)	((c) - '0')
			if (p[1] == '.') {
				freq = C2D(p[0]) * 1000;
				freq += C2D(p[2]) * 100;
				freq += C2D(p[3]) * 10;
				freq *= i * 1000;
			} else {
				freq = C2D(p[0]) * 1000;
				freq += C2D(p[1]) * 100;
				freq += C2D(p[2]) * 10;
				freq += C2D(p[3]);
				freq *= i * 1000000;
			}
#undef C2D
			tsc_freq = freq;
		}
	}
}

static void
probe_tsc_freq(void)
{
	u_int64_t tmp_freq, tsc1, tsc2;
	int no_cpuid_override;

	if (cpu_power_ecx & CPUID_PERF_STAT) {
		/*
		 * XXX Some emulators expose host CPUID without actual support
		 * for these MSRs.  We must test whether they really work.
		 */
		wrmsr(MSR_MPERF, 0);
		wrmsr(MSR_APERF, 0);
		delay(10);
		if (rdmsr(MSR_MPERF) > 0 && rdmsr(MSR_APERF) > 0)
			tsc_perf_stat = 1;
	}
/*
	if (vm_guest == VM_GUEST_VMWARE) {
		tsc_freq_vmware();
		return;
	}
*/
	switch (cpu_vendor_id) {
	case CPUVENDOR_AMD:
	case CPUVENDOR_HYGON:
		if ((amd_pminfo & AMDPM_TSC_INVARIANT) != 0	|| (/*vm_guest == VM_GUEST_NO && */CPUID_TO_FAMILY(cpu_id) >= 0x10)) {
			tsc_is_invariant = 1;
		}
		if (cpu_feature & CPUID_SSE2) {
			tsc_timecounter.tc_get_timecount = tsc_get_timecount_mfence;
		}
		break;
	case CPUVENDOR_INTEL:
		if ((amd_pminfo & AMDPM_TSC_INVARIANT) != 0	|| (/*vm_guest == VM_GUEST_NO && */((CPUID_TO_FAMILY(cpu_id) == 0x6 &&
				CPUID_TO_MODEL(cpu_id) >= 0xe) || (CPUID_TO_FAMILY(cpu_id) == 0xf && CPUID_TO_MODEL(cpu_id) >= 0x3)))) {
			tsc_is_invariant = 1;
		}
		if (cpu_feature & CPUID_SSE2) {
			tsc_timecounter.tc_get_timecount = tsc_get_timecount_lfence;
		}
		break;
	case CPUVENDOR_CENTAUR:
		if (/* vm_guest == VM_GUEST_NO && */CPUID_TO_FAMILY(cpu_id) == 0x6 && CPUID_TO_MODEL(cpu_id) >= 0xf && (rdmsr(0x1203) & 0x100000000ULL) == 0) {
			tsc_is_invariant = 1;
		}
		if (cpu_feature & CPUID_SSE2) {
			tsc_timecounter.tc_get_timecount = tsc_get_timecount_lfence;
		}
		break;
	}

	if (tsc_skip_calibration) {
		if (tsc_freq_cpuid(&tmp_freq)) {
			tsc_freq = tmp_freq;
		} else if (cpu_vendor_id == CPUVENDOR_INTEL) {
			tsc_freq_intel();
		}
		if (tsc_freq == 0) {
			tsc_disabled = 1;
		}
	} else {
		if (bootverbose) {
			printf("Calibrating TSC clock ... ");
		}
		tsc1 = rdtsc();
		delay(1000000);
		tsc2 = rdtsc();
		tsc_freq = tsc2 - tsc1;

		/*
		 * If the difference between calibrated frequency and
		 * the frequency reported by CPUID 0x15/0x16 leafs
		 * differ significantly, this probably means that
		 * calibration is bogus.  It happens on machines
		 * without 8254 timer.  The BIOS rarely properly
		 * reports it in FADT boot flags, so just compare the
		 * frequencies directly.
		 */
		if (tsc_freq_cpuid(&tmp_freq) && qabs(tsc_freq - tmp_freq) > uqmin(tsc_freq, tmp_freq)) {
			no_cpuid_override = 0;
		//	TUNABLE_INT_FETCH("machdep.disable_tsc_cpuid_override", &no_cpuid_override);
			if (!no_cpuid_override) {
				if (bootverbose) {
					printf("TSC clock: calibration freq %ju Hz, CPUID freq %ju Hz%s\n", (uintmax_t) tsc_freq, (uintmax_t) tmp_freq, no_cpuid_override ? "" : ", doing CPUID override");
				}
				tsc_freq = tmp_freq;
			}
		}
	}
	if (bootverbose) {
		printf("TSC clock: %ju Hz\n", (intmax_t) tsc_freq);
	}
}

void
init_TSC(void)
{

	if ((cpu_feature & CPUID_TSC) == 0 || tsc_disabled)
		return;

#ifdef __i386__
	/* The TSC is known to be broken on certain CPUs. */
	switch (cpu_vendor_id) {
	case CPUVENDOR_AMD:
		switch (cpu_id & 0xFF0) {
		case 0x500:
			/* K5 Model 0 */
			return;
		}
		break;
	case CPUVENDOR_CENTAUR:
		switch (cpu_id & 0xff0) {
		case 0x540:
			/*
			 * http://www.centtech.com/c6_data_sheet.pdf
			 *
			 * I-12 RDTSC may return incoherent values in EDX:EAX
			 * I-13 RDTSC hangs when certain event counters are used
			 */
			return;
		}
		break;
	case CPUVENDOR_NSC:
		switch (cpu_id & 0xff0) {
		case 0x540:
			if ((cpu_id & CPUID_STEPPING) == 0)
				return;
			break;
		}
		break;
	}
#endif

	probe_tsc_freq();

	/*
	 * Inform CPU accounting about our boot-time clock rate.  This will
	 * be updated if someone loads a cpufreq driver after boot that
	 * discovers a new max frequency.
	 */
	if (tsc_freq != 0) {
		set_cputicker(rdtsc, tsc_freq, !tsc_is_invariant);
	}

	if (tsc_is_invariant) {
		return;
	}

	/* Register to find out about changes in CPU frequency. */
//	tsc_pre_tag = EVENTHANDLER_REGISTER(cpufreq_pre_change, tsc_freq_changing, NULL, EVENTHANDLER_PRI_FIRST);
//	tsc_post_tag = EVENTHANDLER_REGISTER(cpufreq_post_change, tsc_freq_changed, NULL, EVENTHANDLER_PRI_FIRST);
//	tsc_levels_tag = EVENTHANDLER_REGISTER(cpufreq_levels_changed, tsc_levels_changed, NULL, EVENTHANDLER_PRI_ANY);
}

#ifdef SMP

/*
 * RDTSC is not a serializing instruction, and does not drain
 * instruction stream, so we need to drain the stream before executing
 * it.  It could be fixed by use of RDTSCP, except the instruction is
 * not available everywhere.
 *
 * Use CPUID for draining in the boot-time SMP constistency test.  The
 * timecounters use MFENCE for AMD CPUs, and LFENCE for others (Intel
 * and VIA) when SSE2 is present, and nothing on older machines which
 * also do not issue RDTSC prematurely.  There, testing for SSE2 and
 * vendor is too cumbersome, and we learn about TSC presence from CPUID.
 *
 * Do not use do_cpuid(), since we do not need CPUID results, which
 * have to be written into memory with do_cpuid().
 */
#define	TSC_READ(x)												\
static void														\
tsc_read_##x(void *arg)											\
{																\
	u_int64_t *tsc = arg;										\
	u_int cpu = PCPU_GET(cpuid);								\
																\
	__asm __volatile("cpuid" : : : "eax", "ebx", "ecx", "edx");	\
	tsc[cpu * 3 + x] = rdtsc();									\
}
TSC_READ(0)
TSC_READ(1)
TSC_READ(2)
#undef TSC_READ

#define	N	1000

static void
comp_smp_tsc(void *arg)
{
	u_int64_t *tsc;
	int64_t d1, d2;
	u_int cpu = PCPU_GET(cpuid);
	u_int i, j, size;

	size = (mp_maxid + 1) * 3;
	for (i = 0, tsc = arg; i < N; i++, tsc += size)
		CPU_FOREACH(j) {
			if (j == cpu)
				continue;
			d1 = tsc[cpu * 3 + 1] - tsc[j * 3];
			d2 = tsc[cpu * 3 + 2] - tsc[j * 3 + 1];
			if (d1 <= 0 || d2 <= 0) {
				smp_tsc = 0;
				return;
			}
		}
}

static void
adj_smp_tsc(void *arg)
{
	u_int64_t *tsc;
	int64_t d, min, max;
	u_int cpu = PCPU_GET(cpuid);
	u_int first, i, size;

	first = CPU_FIRST();
	if (cpu == first)
		return;
	min = INT64_MIN;
	max = INT64_MAX;
	size = (mp_maxid + 1) * 3;
	for (i = 0, tsc = arg; i < N; i++, tsc += size) {
		d = tsc[first * 3] - tsc[cpu * 3 + 1];
		if (d > min)
			min = d;
		d = tsc[first * 3 + 1] - tsc[cpu * 3 + 2];
		if (d > min)
			min = d;
		d = tsc[first * 3 + 1] - tsc[cpu * 3];
		if (d < max)
			max = d;
		d = tsc[first * 3 + 2] - tsc[cpu * 3 + 1];
		if (d < max)
			max = d;
	}
	if (min > max)
		return;
	d = min / 2 + max / 2;
	__asm __volatile (
		"movl $0x10, %%ecx\n\t"
		"rdmsr\n\t"
		"addl %%edi, %%eax\n\t"
		"adcl %%esi, %%edx\n\t"
		"wrmsr\n"
		: /* No output */
		: "D" ((u_int32_t)d), "S" ((uint32_t)(d >> 32))
		: "ax", "cx", "dx", "cc"
	);
}

static int
test_tsc(int adj_max_count)
{
	u_int64_t *data, *tsc;
	u_int i, size, adj;

	if ((!smp_tsc && !tsc_is_invariant) || vm_guest)
		return (-100);
	size = (mp_maxid + 1) * 3;
	data = malloc(sizeof(*data) * size * N, M_TEMP, M_WAITOK);
	adj = 0;
retry:
	for (i = 0, tsc = data; i < N; i++, tsc += size)
		smp_rendezvous(tsc_read_0, tsc_read_1, tsc_read_2, tsc);
	smp_tsc = 1;	/* XXX */
	smp_rendezvous(smp_no_rendezvous_barrier, comp_smp_tsc, smp_no_rendezvous_barrier, data);
	if (!smp_tsc && adj < adj_max_count) {
		adj++;
		smp_rendezvous(smp_no_rendezvous_barrier, adj_smp_tsc, smp_no_rendezvous_barrier, data);
		goto retry;
	}
	free(data, M_TEMP);
	if (bootverbose)
		printf("SMP: %sed TSC synchronization test%s\n", smp_tsc ? "pass" : "fail", adj > 0 ? " after adjustment" : "");
	if (smp_tsc && tsc_is_invariant) {
		switch (cpu_vendor_id) {
		case CPUVENDOR_AMD:
		case CPUVENDOR_HYGON:
			/*
			 * Processor Programming Reference (PPR) for AMD
			 * Family 17h states that the TSC uses a common
			 * reference for all sockets, cores and threads.
			 */
			if (CPUID_TO_FAMILY(cpu_id) >= 0x17)
				return (1000);
			/*
			 * Starting with Family 15h processors, TSC clock
			 * source is in the north bridge.  Check whether
			 * we have a single-socket/multi-core platform.
			 * XXX Need more work for complex cases.
			 */
			if (CPUID_TO_FAMILY(cpu_id) < 0x15 || (amd_feature2 & AMDID2_CMP) == 0 || smp_cpus > (cpu_procinfo2 & AMDID_CMP_CORES) + 1)
				break;
			return (1000);
		case CPUVENDOR_INTEL:
			/*
			 * XXX Assume Intel platforms have synchronized TSCs.
			 */
			return (1000);
		}
		return (800);
	}
	return (-100);
}

#undef N

#endif /* SMP */

static void
init_TSC_tc(void)
{
	u_int64_t max_freq;
	int shift;

	if ((cpu_feature & CPUID_TSC) == 0 || tsc_disabled)
		return;

	/*
	 * Limit timecounter frequency to fit in an int and prevent it from
	 * overflowing too fast.
	 */
	max_freq = UINT_MAX;

	/*
	 * We can not use the TSC if we support APM.  Precise timekeeping
	 * on an APM'ed machine is at best a fools pursuit, since
	 * any and all of the time spent in various SMM code can't
	 * be reliably accounted for.  Reading the RTC is your only
	 * source of reliable time info.  The i8254 loses too, of course,
	 * but we need to have some kind of time...
	 * We don't know at this point whether APM is going to be used
	 * or not, nor when it might be activated.  Play it safe.
	 */
	if (power_pm_get_type() == POWER_PM_TYPE_APM) {
		tsc_timecounter.tc_quality = -1000;
		if (bootverbose)
			printf("TSC timecounter disabled: APM enabled.\n");
		goto init;
	}

	/*
	 * Intel CPUs without a C-state invariant TSC can stop the TSC
	 * in either C2 or C3.  Disable use of C2 and C3 while using
	 * the TSC as the timecounter.  The timecounter can be changed
	 * to enable C2 and C3.
	 *
	 * Note that the TSC is used as the cputicker for computing
	 * thread runtime regardless of the timecounter setting, so
	 * using an alternate timecounter and enabling C2 or C3 can
	 * result incorrect runtimes for kernel idle threads (but not
	 * for any non-idle threads).
	 */
	if (cpu_vendor_id == CPUVENDOR_INTEL &&
	    (amd_pminfo & AMDPM_TSC_INVARIANT) == 0) {
		tsc_timecounter.tc_flags |= TC_FLAGS_C2STOP;
		if (bootverbose)
			printf("TSC timecounter disables C2 and C3.\n");
	}

	/*
	 * We can not use the TSC in SMP mode unless the TSCs on all CPUs
	 * are synchronized.  If the user is sure that the system has
	 * synchronized TSCs, set kern.timecounter.smp_tsc tunable to a
	 * non-zero value.  The TSC seems unreliable in virtualized SMP
	 * environments, so it is set to a negative quality in those cases.
	 */
#ifdef SMP
	if (mp_ncpus > 1)
		tsc_timecounter.tc_quality = test_tsc(smp_tsc_adjust);
	else
#endif /* SMP */
	if (tsc_is_invariant)
		tsc_timecounter.tc_quality = 1000;
	max_freq >>= tsc_shift;

init:
	for (shift = 0; shift <= 31 && (tsc_freq >> shift) > max_freq; shift++)
		;
	if ((cpu_feature & CPUID_SSE2) != 0 && mp_ncpus > 1) {
		if (cpu_vendor_id == CPUVENDOR_AMD ||
		    cpu_vendor_id == CPUVENDOR_HYGON) {
			tsc_timecounter.tc_get_timecount = shift > 0 ?
			    tsc_get_timecount_low_mfence :
			    tsc_get_timecount_mfence;
		} else {
			tsc_timecounter.tc_get_timecount = shift > 0 ?
			    tsc_get_timecount_low_lfence :
			    tsc_get_timecount_lfence;
		}
	} else {
		tsc_timecounter.tc_get_timecount = shift > 0 ?
		    tsc_get_timecount_low : tsc_get_timecount;
	}
	if (shift > 0) {
		tsc_timecounter.tc_name = "TSC-low";
		if (bootverbose)
			printf("TSC timecounter discards lower %d bit(s)\n",
			    shift);
	}
	if (tsc_freq != 0) {
		tsc_timecounter.tc_frequency = tsc_freq >> shift;
		tsc_timecounter.tc_priv = (void *)(intptr_t)shift;
		tc_init(&tsc_timecounter);
	}
}

void
resume_TSC(void)
{
#ifdef SMP
	int quality;

	/* If TSC was not good on boot, it is unlikely to become good now. */
	if (tsc_timecounter.tc_quality < 0)
		return;
	/* Nothing to do with UP. */
	if (mp_ncpus < 2)
		return;

	/*
	 * If TSC was good, a single synchronization should be enough,
	 * but honour smp_tsc_adjust if it's set.
	 */
	quality = test_tsc(MAX(smp_tsc_adjust, 1));
	if (quality != tsc_timecounter.tc_quality) {
		printf("TSC timecounter quality changed: %d -> %d\n",
		    tsc_timecounter.tc_quality, quality);
		tsc_timecounter.tc_quality = quality;
	}
#endif /* SMP */
}

/*
 * When cpufreq levels change, find out about the (new) max frequency.  We
 * use this to update CPU accounting in case it got a lower estimate at boot.
 */
static void
tsc_levels_changed(void *arg, int unit)
{
	struct device *cf_dev;
	struct cf_level *levels;
	int count, error;
	uint64_t max_freq;

	/* Only use values from the first CPU, assuming all are equal. */
	if (unit != 0)
		return;

	/* Find the appropriate cpufreq device instance. */
	cf_dev = devclass_get_device(devclass_find("cpufreq"), unit);
	if (cf_dev == NULL) {
		printf("tsc_levels_changed() called but no cpufreq device?\n");
		return;
	}

	/* Get settings from the device and find the max frequency. */
	count = 64;
	levels = malloc(count * sizeof(*levels), M_TEMP, M_NOWAIT);
	if (levels == NULL)
		return;
	error = CPUFREQ_LEVELS(cf_dev, levels, &count);
	if (error == 0 && count != 0) {
		max_freq = (uint64_t)levels[0].total_set.freq * 1000000;
		set_cputicker(rdtsc, max_freq, 1);
	} else
		printf("tsc_levels_changed: no max freq found\n");
	free(levels, M_TEMP);
}

/*
 * If the TSC timecounter is in use, veto the pending change.  It may be
 * possible in the future to handle a dynamically-changing timecounter rate.
 */
static void
tsc_freq_changing(void *arg, const struct cf_level *level, int *status)
{
	if (*status != 0 || timecounter != &tsc_timecounter)
		return;

	printf("timecounter TSC must not be in use when changing frequencies; change denied\n");
	*status = EBUSY;
}

/* Update TSC freq with the value indicated by the caller. */
static void
tsc_freq_changed(void *arg, const struct cf_level *level, int status)
{
	u_int64_t freq;

	/* If there was an error during the transition, don't do anything. */
	if (tsc_disabled || status != 0)
		return;

	/* Total setting for this level gives the new frequency in MHz. */
	freq = (uint64_t)level->total_set.freq * 1000000;
	atomic_store_rel_64(&tsc_freq, freq);
	tsc_timecounter.tc_frequency =  freq >> (int)(intptr_t)tsc_timecounter.tc_priv;
}

static int
sysctl_machdep_tsc_freq(SYSCTL_HANDLER_ARGS)
{
	int error;
	u_int64_t freq;

	freq = atomic_load_acq_64(&tsc_freq);
	if (freq == 0)
		return (EOPNOTSUPP);
	error = sysctl_handle_64(oidp, &freq, 0, req);
	if (error == 0 && req->newptr != NULL) {
		atomic_store_rel_64(&tsc_freq, freq);
		atomic_store_rel_64(&tsc_timecounter.tc_frequency, freq >> (int)(intptr_t)tsc_timecounter.tc_priv);
	}
	return (error);
}

static u_int
tsc_get_timecount(struct timecounter *tc /*__unused */)
{
	return (rdtsc32());
}

static inline u_int
tsc_get_timecount_low(struct timecounter *tc)
{
	u_int32_t rv;

	__asm __volatile("rdtsc; shrd %%cl, %%edx, %0"
	    : "=a" (rv) : "c" ((int)(intptr_t)tc->tc_priv) : "edx");
	return (rv);
}

static u_int
tsc_get_timecount_lfence(struct timecounter *tc /*__unused */)
{

	lfence();
	return (rdtsc32());
}

static u_int
tsc_get_timecount_low_lfence(struct timecounter *tc)
{

	lfence();
	return (tsc_get_timecount_low(tc));
}

static u_int
tsc_get_timecount_mfence(struct timecounter *tc /*__unused */)
{
	mfence();
	return (rdtsc32());
}

static u_int
tsc_get_timecount_low_mfence(struct timecounter *tc)
{
	mfence();
	return (tsc_get_timecount_low(tc));
}
