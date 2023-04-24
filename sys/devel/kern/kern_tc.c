/* $NetBSD: kern_tc.c,v 1.62 2021/06/02 21:34:58 riastradh Exp $ */

/*-
 * Copyright (c) 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ---------------------------------------------------------------------------
 */

#include <sys/cdefs.h>
/*
__FBSDID("$FreeBSD: src/sys/kern/kern_tc.c,v 1.166 2005/09/19 22:16:31 andre Exp $");
__KERNEL_RCSID(0, "$NetBSD: kern_tc.c,v 1.62 2021/06/02 21:34:58 riastradh Exp $");
*/
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/timepps.h>

#include <devel/sys/timetc.h>

/*
 * A large step happens on boot.  This constant detects such steps.
 * It is relatively small so that ntp_update_second gets called enough
 * in the typical 'missed a couple of seconds' case, but doesn't loop
 * forever when the time step is large.
 */
#define LARGE_STEP	200

/*
 * Implement a dummy timecounter which we can use until we get a real one
 * in the air.  This allows the console and other early stuff to use
 * time services.
 */

static u_int
dummy_get_timecount(struct timecounter *tc)
{
	static u_int now;

	return ++now;
}

static struct timecounter dummy_timecounter = {
		.tc_get_timecount	= dummy_get_timecount,
		.tc_counter_mask	= ~0u,
		.tc_frequency		= 1000000,
		.tc_name			= "dummy",
		.tc_quality			= -1000000,
		.tc_priv			= NULL,
};

struct timehands {
	/* These fields must be initialized by the driver. */
	struct timecounter	*th_counter;     /* active timecounter */
	int64_t				th_adjustment;   /* frequency adjustment */
						 /* (NTP/adjtime) */
	uint64_t			th_scale;        /* scale factor (counter */
						 /* tick->time) */
	uint64_t 			th_offset_count; /* offset at last time */
						 /* update (tc_windup()) */
	struct bintime		th_offset;       /* bin (up)time at windup */
	struct timeval		th_microtime;    /* cached microtime */
	struct timespec		th_nanotime;     /* cached nanotime */
	/* Fields not to be copied in tc_windup start with th_generation. */
	volatile u_int		th_generation;   /* current genration */
	struct timehands	*th_next;        /* next timehand */
};

static struct timehands th0;
static struct timehands th9 = { .th_next = &th0, };
static struct timehands th8 = { .th_next = &th9, };
static struct timehands th7 = { .th_next = &th8, };
static struct timehands th6 = { .th_next = &th7, };
static struct timehands th5 = { .th_next = &th6, };
static struct timehands th4 = { .th_next = &th5, };
static struct timehands th3 = { .th_next = &th4, };
static struct timehands th2 = { .th_next = &th3, };
static struct timehands th1 = { .th_next = &th2, };

static struct timehands th0 = {
	.th_counter = &dummy_timecounter,
	.th_scale = (uint64_t)-1 / 1000000,
	.th_offset = { .sec = 1, .frac = 0 },
	.th_generation = 1,
	.th_next = &th1,
};

static struct timehands *volatile timehands = &th0;
struct timecounter *timecounter = &dummy_timecounter;
static struct timecounter *timecounters = &dummy_timecounter;

volatile time_t time_second __cacheline_aligned = 1;
volatile time_t time_uptime __cacheline_aligned = 1;

static struct bintime timebasebin;

static int timestepwarnings;

struct mtx timecounter_lock;
static u_int timecounter_mods;
static volatile int timecounter_removals = 1;
static u_int timecounter_bad;

#ifdef TC_COUNTERS
#define	TC_STATS(name)												\
static struct evcnt n##name =										\
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "timecounter", #name);	\
EVCNT_ATTACH_STATIC(n##name)
TC_STATS(binuptime);    TC_STATS(nanouptime);    TC_STATS(microuptime);
TC_STATS(bintime);      TC_STATS(nanotime);      TC_STATS(microtime);
TC_STATS(getbinuptime); TC_STATS(getnanouptime); TC_STATS(getmicrouptime);
TC_STATS(getbintime);   TC_STATS(getnanotime);   TC_STATS(getmicrotime);
TC_STATS(setclock);
#define	TC_COUNT(var)	var.ev_count++
#undef TC_STATS
#else
#define	TC_COUNT(var)	/* nothing */
#endif	/* TC_COUNTERS */

static void tc_windup(void);

/*
 * Return the difference between the timehands' counter value now and what
 * was when we copied it to the timehands' offset_count.
 */
static inline u_int
tc_delta(struct timehands *th)
{
	struct timecounter *tc;

	tc = th->th_counter;
	return (tc->tc_get_timecount(tc) - th->th_offset_count) & tc->tc_counter_mask;
}

/*
 * Functions for reading the time.  We have to loop until we are sure that
 * the timehands that we operated on was not updated under our feet.  See
 * the comment in <sys/timevar.h> for a description of these 12 functions.
 */

void
binuptime(struct bintime *bt)
{
	struct timehands *th;
	struct proc *p;
	u_int lgen, gen;

	TC_COUNT(nbinuptime);

	/*
	 * Provide exclusion against tc_detach().
	 *
	 * We record the number of timecounter removals before accessing
	 * timecounter state.  Note that the LWP can be using multiple
	 * "generations" at once, due to interrupts (interrupted while in
	 * this function).  Hardware interrupts will borrow the interrupted
	 * LWP's l_tcgen value for this purpose, and can themselves be
	 * interrupted by higher priority interrupts.  In this case we need
	 * to ensure that the oldest generation in use is recorded.
	 *
	 * splsched() is too expensive to use, so we take care to structure
	 * this code in such a way that it is not required.  Likewise, we
	 * do not disable preemption.
	 *
	 * Memory barriers are also too expensive to use for such a
	 * performance critical function.  The good news is that we do not
	 * need memory barriers for this type of exclusion, as the thread
	 * updating timecounter_removals will issue a broadcast cross call
	 * before inspecting our l_tcgen value (this elides memory ordering
	 * issues).
	 */
	p = curproc;
	lgen = p->p_tcgen;
	if (__predict_true(lgen == 0)) {
		p->p_tcgen = timecounter_removals;
	}
	__insn_barrier();

	do {
		th = timehands;
		gen = th->th_generation;
		*bt = th->th_offset;
		bintime_addx(bt, th->th_scale * tc_delta(th));
	} while (gen == 0 || gen != th->th_generation);

	__insn_barrier();
	p->p_tcgen = lgen;
}

void
nanouptime(struct timespec *tsp)
{
	struct bintime bt;

	TC_COUNT(nnanouptime);
	binuptime(&bt);
	bintime2timespec(&bt, tsp);
}

void
microuptime(struct timeval *tvp)
{
	struct bintime bt;

	TC_COUNT(nmicrouptime);
	binuptime(&bt);
	bintime2timeval(&bt, tvp);
}

void
bintime(struct bintime *bt)
{

	TC_COUNT(nbintime);
	binuptime(bt);
	bintime_add(bt, &timebasebin);
}

void
nanotime(struct timespec *tsp)
{
	struct bintime bt;

	TC_COUNT(nnanotime);
	bintime(&bt);
	bintime2timespec(&bt, tsp);
}

void
microtime(struct timeval *tvp)
{
	struct bintime bt;

	TC_COUNT(nmicrotime);
	bintime(&bt);
	bintime2timeval(&bt, tvp);
}

void
getbinuptime(struct bintime *bt)
{
	struct timehands *th;
	u_int gen;

	TC_COUNT(ngetbinuptime);
	do {
		th = timehands;
		gen = th->th_generation;
		*bt = th->th_offset;
	} while (gen == 0 || gen != th->th_generation);
}

void
getnanouptime(struct timespec *tsp)
{
	struct timehands *th;
	u_int gen;

	TC_COUNT(ngetnanouptime);
	do {
		th = timehands;
		gen = th->th_generation;
		bintime2timespec(&th->th_offset, tsp);
	} while (gen == 0 || gen != th->th_generation);
}

void
getmicrouptime(struct timeval *tvp)
{
	struct timehands *th;
	u_int gen;

	TC_COUNT(ngetmicrouptime);
	do {
		th = timehands;
		gen = th->th_generation;
		bintime2timeval(&th->th_offset, tvp);
	} while (gen == 0 || gen != th->th_generation);
}

void
getbintime(struct bintime *bt)
{
	struct timehands *th;
	u_int gen;

	TC_COUNT(ngetbintime);
	do {
		th = timehands;
		gen = th->th_generation;
		*bt = th->th_offset;
	} while (gen == 0 || gen != th->th_generation);
	bintime_add(bt, &timebasebin);
}

static inline void
dogetnanotime(struct timespec *tsp)
{
	struct timehands *th;
	u_int gen;

	TC_COUNT(ngetnanotime);
	do {
		th = timehands;
		gen = th->th_generation;
		*tsp = th->th_nanotime;
	} while (gen == 0 || gen != th->th_generation);
}

void
getnanotime(struct timespec *tsp)
{

	dogetnanotime(tsp);
}

void dtrace_getnanotime(struct timespec *tsp);

void
dtrace_getnanotime(struct timespec *tsp)
{

	dogetnanotime(tsp);
}

void
getmicrotime(struct timeval *tvp)
{
	struct timehands *th;
	u_int gen;

	TC_COUNT(ngetmicrotime);
	do {
		th = timehands;
		gen = th->th_generation;
		*tvp = th->th_microtime;
	} while (gen == 0 || gen != th->th_generation);
}

void
getnanoboottime(struct timespec *tsp)
{
	struct bintime bt;

	getbinboottime(&bt);
	bintime2timespec(&bt, tsp);
}

void
getmicroboottime(struct timeval *tvp)
{
	struct bintime bt;

	getbinboottime(&bt);
	bintime2timeval(&bt, tvp);
}

void
getbinboottime(struct bintime *bt)
{

	/*
	 * XXX Need lockless read synchronization around timebasebin
	 * (and not just here).
	 */
	*bt = timebasebin;
}

/*
 * Initialize a new timecounter and possibly use it.
 */
void
tc_init(struct timecounter *tc)
{
	u_int u;

	KASSERTMSG(tc->tc_next == NULL, "timecounter %s already initialised", tc->tc_name);

	u = tc->tc_frequency / tc->tc_counter_mask;
	/* XXX: We need some margin here, 10% is a guess */
	u *= 11;
	u /= 10;
	if (u > hz && tc->tc_quality >= 0) {
		tc->tc_quality = -2000;
		printf("timecounter: Timecounter \"%s\" frequency %ju Hz", tc->tc_name,
				(uintmax_t) tc->tc_frequency);
		printf(" -- Insufficient hz, needs at least %u\n", u);
	} else if (tc->tc_quality >= 0 || bootverbose) {
		printf("timecounter: Timecounter \"%s\" frequency %ju Hz "
				"quality %d\n", tc->tc_name, (uintmax_t) tc->tc_frequency,
				tc->tc_quality);
	}

	mutex_spin_enter(&timecounter_lock);
	tc->tc_next = timecounters;
	timecounters = tc;
	timecounter_mods++;
	/*
	 * Never automatically use a timecounter with negative quality.
	 * Even though we run on the dummy counter, switching here may be
	 * worse since this timecounter may not be monotonous.
	 */
	if (tc->tc_quality >= 0
			&& (tc->tc_quality > timecounter->tc_quality
					|| (tc->tc_quality == timecounter->tc_quality
							&& tc->tc_frequency > timecounter->tc_frequency))) {
		(void) tc->tc_get_timecount(tc);
		(void) tc->tc_get_timecount(tc);
		timecounter = tc;
		tc_windup();
	}
	mutex_spin_exit(&timecounter_lock);
}

/*
 * Pick a new timecounter due to the existing counter going bad.
 */
static void
tc_pick(void)
{
	struct timecounter *best, *tc;

	KASSERT(mutex_owned(&timecounter_lock));

	for (best = tc = timecounters; tc != NULL; tc = tc->tc_next) {
		if (tc->tc_quality > best->tc_quality)
			best = tc;
		else if (tc->tc_quality < best->tc_quality)
			continue;
		else if (tc->tc_frequency > best->tc_frequency)
			best = tc;
	}
	(void)best->tc_get_timecount(best);
	(void)best->tc_get_timecount(best);
	timecounter = best;
}

/*
 * A timecounter has gone bad, arrange to pick a new one at the next
 * clock tick.
 */
void
tc_gonebad(struct timecounter *tc)
{

	tc->tc_quality = -100;
	membar_producer();
	atomic_inc_uint(&timecounter_bad);
}

/*
 * Stop using a timecounter and remove it from the timecounters list.
 */
int
tc_detach(struct timecounter *target)
{
	struct timecounter *tc;
	struct timecounter **tcp = NULL;
	int removals;
	struct proc *p;

	/* First, find the timecounter. */
	mutex_spin_enter(&timecounter_lock);

	for (tcp = &timecounters, tc = timecounters;
	     tc != NULL;
	     tcp = &tc->tc_next, tc = tc->tc_next) {
		if (tc == target)
			break;
	}
	if (tc == NULL) {
		mutex_spin_exit(&timecounter_lock);
		return ESRCH;
	}

	/* And now, remove it. */
	*tcp = tc->tc_next;
	if (timecounter == target) {
		tc_pick();
		tc_windup();
	}
	timecounter_mods++;
	removals = timecounter_removals++;
	mutex_spin_exit(&timecounter_lock);

	/*
	 * We now have to determine if any threads in the system are still
	 * making use of this timecounter.
	 *
	 * We issue a broadcast cross call to elide memory ordering issues,
	 * then scan all LWPs in the system looking at each's timecounter
	 * generation number.  We need to see a value of zero (not actively
	 * using a timecounter) or a value greater than our removal value.
	 *
	 * We may race with threads that read `timecounter_removals' and
	 * and then get preempted before updating `l_tcgen'.  This is not
	 * a problem, since it means that these threads have not yet started
	 * accessing timecounter state.  All we do need is one clean
	 * snapshot of the system where every thread appears not to be using
	 * old timecounter state.
	 */
	for (;;) {
		xc_barrier(0);

		PROC_LOCK(p);
		LIST_FOREACH(p, &allproc, p_list) {
			if (p->p_tcgen == 0 || p->p_tcgen > removals) {
				/*
				 * Not using timecounter or old timecounter
				 * state at time of our xcall or later.
				 */
				continue;
			}
			break;
		}
		PROC_UNLOCK(p);

		/*
		 * If the timecounter is still in use, wait at least 10ms
		 * before retrying.
		 */
		if (p == NULL) {
			break;
		}
		(void)kpause("tcdetach", FALSE, mstohz(10), NULL);
	}

	tc->tc_next = NULL;
	return 0;
}

/* Report the frequency of the current timecounter. */
uint64_t
tc_getfrequency(void)
{
	return timehands->th_counter->tc_frequency;
}
