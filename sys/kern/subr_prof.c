/*-
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)subr_prof.c	8.4 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <sys/mount.h>

#include <machine/cpu.h>

#ifdef GPROF
#include <sys/malloc.h>
#include <sys/gmon.h>

/*
 * Froms is actually a bunch of unsigned shorts indexing tos
 */
struct gmonparam _gmonparam = { GMON_PROF_OFF };

extern char etext[];

void
kmstartup(void)
{
	char *cp;
	struct gmonparam *p = &_gmonparam;
	/*
	 * Round lowpc and highpc to multiples of the density we're using
	 * so the rest of the scaling (here and in gprof) stays in ints.
	 */
	p->lowpc = ROUNDDOWN(KERNBASE, HISTFRACTION * sizeof(HISTCOUNTER));
	p->highpc = ROUNDUP((u_long)etext, HISTFRACTION * sizeof(HISTCOUNTER));
	p->textsize = p->highpc - p->lowpc;
	printf("Profiling kernel, textsize=%d [%x..%x]\n", p->textsize, p->lowpc, p->highpc);
	p->kcountsize = p->textsize / HISTFRACTION;
	p->hashfraction = HASHFRACTION;
	p->fromssize = p->textsize / HASHFRACTION;
	p->tolimit = p->textsize * ARCDENSITY / 100;
	if (p->tolimit < MINARCS) {
		p->tolimit = MINARCS;
	} else if (p->tolimit > MAXARCS) {
		p->tolimit = MAXARCS;
	}
	p->tossize = p->tolimit * sizeof(struct tostruct);
	cp = (char *)malloc(p->kcountsize + p->fromssize + p->tossize, M_GPROF, M_NOWAIT);
	if (cp == 0) {
		printf("No memory for profiling.\n");
		return;
	}
	bzero(cp, p->kcountsize + p->tossize + p->fromssize);
	p->tos = (struct tostruct *)cp;
	cp += p->tossize;
	p->kcount = (u_short *)cp;
	cp += p->kcountsize;
	p->froms = (u_short *)cp;
}

/*
 * Return kernel profiling information.
 */
int
sysctl_doprof(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	struct gmonparam *gp = &_gmonparam;
	int error;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case GPROF_STATE:
		error = sysctl_int(oldp, oldlenp, newp, newlen, &gp->state);
		if (error)
			return (error);
		if (gp->state == GMON_PROF_OFF)
			stopprofclock(&proc0);
		else
			startprofclock(&proc0);
		return (0);
	case GPROF_COUNT:
		return (sysctl_struct(oldp, oldlenp, newp, newlen,
		    gp->kcount, gp->kcountsize));
	case GPROF_FROMS:
		return (sysctl_struct(oldp, oldlenp, newp, newlen,
		    gp->froms, gp->fromssize));
	case GPROF_TOS:
		return (sysctl_struct(oldp, oldlenp, newp, newlen,
		    gp->tos, gp->tossize));
	case GPROF_GMONPARAM:
		return (sysctl_rdstruct(oldp, oldlenp, newp, gp, sizeof *gp));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
#endif /* GPROF */

/*
 * Scale is a fixed-point number with the binary point 16 bits
 * into the value, and is <= 1.0.  pc is at most 32 bits, so the
 * intermediate result is at most 48 bits.
 */
#define	PC_TO_INDEX(pc, prof) 							\
	((int)(((u_quad_t)((pc) - (prof)->pr_off) * 		\
			(u_quad_t)((prof)->pr_scale)) >> 16) & ~1)

/*
 * Collect user-level profiling statistics; called on a profiling tick,
 * when a process is running in user-mode.  This routine may be called
 * from an interrupt context.  We schedule an AST that will vector us
 * to trap() with a context in which copyin and copyout will work.
 * Trap will then call addupc_task().
 *
 * XXX We could use ufetch/ustore here if the profile buffers were
 * wired.
 *
 * Note that we may (rarely) not get around to the AST soon enough, and
 * lose profile ticks when the next tick overwrites this one, but in this
 * case the system is overloaded and the profile is probably already
 * inaccurate.
 */
void
addupc_intr(p, pc, ticks)
	register struct proc *p;
	register u_long pc;
	u_int ticks;
{
	register struct uprof *prof;
	register u_int i;

	if (ticks == 0)
		return;
	prof = &p->p_stats->p_prof;
	if (pc < prof->pr_off || (i = PC_TO_INDEX(pc, prof)) >= prof->pr_size) {
		return; /* out of range; ignore */
	}

	prof->pr_addr = pc;
	prof->pr_ticks = ticks;
	cpu_need_proftick(p);
}

void
addupc_intru(pc, up, ticks)
	u_long pc;
	struct uprof *up;
	u_int ticks;
{
	if (ticks == 0) {
		return;
	}
	register struct proc *p;
	register u_int i;

	if (ticks == 0)
		return;
	p = u.u_procp;

	if (pc < up->pr_off || (i = PC_TO_INDEX(pc, up)) >= up->pr_size) {
		return; /* out of range; ignore */
	}

	up->pr_addr = pc;
	up->pr_ticks = ticks;
	cpu_need_proftick(p);
}

/*
 * Much like before, but we can afford to take faults here.  If the
 * update fails, we simply turn off profiling.
 */
void
addupc_task(p, pc, ticks)
	register struct proc *p;
	register u_long pc;
	u_int ticks;
{
	register struct uprof *prof;
	register void *addr;
	register int error;
	register u_int i;
	u_short v;

	/* Testing P_PROFIL may be unnecessary, but is certainly safe. */
	if ((p->p_flag & P_PROFIL) == 0 || ticks == 0)
		return;

	prof = &p->p_stats->p_prof;
	if (pc < prof->pr_off || (i = PC_TO_INDEX(pc, prof)) >= prof->pr_size) {
		return;
	}

	addr = prof->pr_base + i;
	if (copyin(addr, (caddr_t) &v, sizeof(v)) == 0) {
		v += ticks;
		if (copyout((caddr_t) &v, addr, sizeof(v)) == 0) {
			return;
		}
	}
	stopprofclock(p);
}

void
addupc_tasku(pc, up, ticks)
	u_long pc;
	struct uprof *up;
	u_int ticks;
{
	register struct proc *p;
	register void *addr;
	register int error;
	register u_int i;
	u_short v;

	p = u.u_procp;
	/* Testing P_PROFIL may be unnecessary, but is certainly safe. */
	if ((p->p_flag & P_PROFIL) == 0 || ticks == 0)
		return;

	if (pc < up->pr_off || (i = PC_TO_INDEX(pc, up)) >= up->pr_size) {
		return;
	}

	addr = up->pr_base + i;
	if (copyin(addr, (caddr_t) &v, sizeof(v)) == 0) {
		v += ticks;
		if (copyout((caddr_t) &v, addr, sizeof(v)) == 0) {
			return;
		}
	}
	stopprofclock(p);
}
