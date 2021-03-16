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
kmstartup()
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
	if (p->tolimit < MINARCS)
		p->tolimit = MINARCS;
	else if (p->tolimit > MAXARCS)
		p->tolimit = MAXARCS;
	p->tossize = p->tolimit * sizeof(struct tostruct);
	cp = (char *)malloc(p->kcountsize + p->fromssize + p->tossize,
	    M_GPROF, M_NOWAIT);
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
sysctl_doprof(name, namelen, oldp, oldlenp, newp, newlen, p)
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
