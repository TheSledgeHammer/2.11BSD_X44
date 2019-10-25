/*
 * edf.c
 *
 *  Created on: 17 Oct 2019
 *      Author: marti
 *
 *  Earliest Deadline First Scheduler, ported from Plan 9. Needs work...!
 *  Missing pieces.
 *  lock: threading, process lock
 */
#include "../test/edf.h"

#include <sys/errno.h>
#include <sys/trace.h>
#include "../test/fmt.h"

/* debugging */
enum {
	Dontprint = 1,
};

#define DPRINT if(Dontprint) {} else print

static long	    now;			/* Low order 32 bits of time in µs */
extern u_long	delayedscheds;
extern Schedq	runq[Nrq];      /* Queue Schedule */
extern int	    nrdy;
extern u_long	runvec;

/* Statistics stuff */
u_long		nilcount;
u_long		scheds;
u_long		edfnrun;
int		    misseddeadlines;

/* Edfschedlock protects modification of admission params */
int		    edfinited;
//QLock		edfschedlock;
//static Lock	thelock;

enum{
	Dl,	/* Invariant for schedulability test: Dl < Rl */
	Rl,
};

static char *testschedulability(struct proc*);
struct proc *qs;

enum {
	Onemicrosecond =	1,
	Onemillisecond =	1000,
	Onesecond =			1000000,
	OneRound = 			Onemillisecond/2,
};

//uvlong = unsigned long long = u_quad_t
//vlong =  long long = quad_t
static int
timeconv(Fmt *f)
{
	char buf[128], *sign;
	quad_t qt;
	u_quad_t uqt;
	long t;

	buf[0] = 0;
	switch(f->r) {
	case 'U':
		qt = va_arg(f->args, uqt);
		break;
	case 't':			/* vlong in nanoseconds */
		qt = va_arg(f->args, t);
		break;
	default:
		return fmtstrcpy(f, "(timeconv)");
	}
	if (t < 0) {
			sign = "-";
			qt = -qt;
	} else {
		sign = "";
		if (qt > Onesecond) {
			qt += OneRound;
			snprint(buf, sizeof buf, "%s%d.%.3ds", sign, (int)(qt / Onesecond), (int)(qt % Onesecond)/Onemillisecond);
		} else if (qt > Onemillisecond) {
			snprint(buf, sizeof buf, "%s%d.%.3dms", sign, (int)(qt / Onemillisecond), (int)(qt % Onemillisecond));
		} else {
			snprint(buf, sizeof buf, "%s%dµs", sign, (int)qt);
		}
	}
	return strcpy(f, buf);
}

long edfcycles;

Edf*
edflock(struct proc *p)
{
	Edf *e;
	if (p->edf == NULL)
		return NULL;
	ilock(&thelock);
	if((e = p->edf) && (e->flags & Admitted)) {
		thelock.pc = getcallerpc(&p);

#ifdef EDFCYCLES
		edfcycles -= lcycles();
#endif
		now = µs();
		return e;
	}
	iunlock(&thelock);
	return NULL;
}

void
edfunlock(void)
{
#ifdef EDFCYCLES
	edfcycles += lcycles();
#endif
	edfnrun++;
	iunlock(&thelock);
}

void
edfinit(struct proc *p)
{
	if(!edfinited){
		fmtinstall('t', timeconv);
		edfinited++;
	}
	now = µs();
	DPRINT("%lud edfinit %lud[%s]\n", now, p->p_pid, statename[p->state]);
	p->edf = malloc(sizeof(Edf));
	if(p->edf == NULL)
		error(ENOMEM);
	return;
}

static void
deadlineintr(Ureg*, time_t *t)
{
	/* Proc reached deadline */
	extern int panicking;
	struct proc *p;
	void (*pt)(struct proc*, int, vlong);

	if(panicking || active.exiting)
		return;

	p = t->ta;
	now = µs();
	DPRINT("%lud deadlineintr %lud[%s]\n", now, p->p_pid, statename[p->state]);
	/* If we're interrupting something other than the proc pointed to by t->a,
	 * we've already achieved recheduling, so we need not do anything
	 * Otherwise, we must cause a reschedule, but if we call sched()
	 * here directly, the timer interrupt routine will not finish its business
	 * Instead, we cause the resched to happen when the interrupted proc
	 * returns to user space
	 */
	if(p == up){
		if(up->trace && (pt = proctrace))
			pt(up, SInts, 0);
		up->delaysched++;
 		delayedscheds++;
	}
}

static void
release(struct proc *p)
{
	/* Called with edflock held */
	Edf *e;
	void (*pt)(struct proc*, int, vlong);
	long n;
	quad_t nowns;

	e = p->edf;
	e->flags &= ~Yield;
	if(e->d - now < 0){
		e->periods++;
		e->r = now;
		if((e->flags & Sporadic) == 0){
			/*
			 * Non sporadic processes stay true to their period;
			 * calculate next release time.
			 * Second test limits duration of while loop.
			 */
			if((n = now - e->t) > 0){
				if(n < e->T)
					e->t += e->T;
				else
					e->t = now + e->T - (n % e->T);
			}
		}else{
			/* Sporadic processes may not be released earlier than
			 * one period after this release
			 */
			e->t = e->r + e->T;
		}
		e->d = e->r + e->D;
		e->S = e->C;
		DPRINT("%lud release %lud[%s], r=%lud, d=%lud, t=%lud, S=%lud\n",
			now, p->p_pid, statename[p->state], e->r, e->d, e->t, e->S);
		if(pt == proctrace){
			nowns = todget(NULL);
			pt(p, SRelease, nowns);
			pt(p, SDeadline, nowns + 1000LL*e->D);
		}
	}else{
		DPRINT("%lud release %lud[%s], too late t=%lud, called from %#p\n",
			now, p->p_pid, statename[p->state], e->t, getcallerpc(&p));
	}
}

static void
releaseintr(Ureg*, time_t *t)
{
	struct proc *p;
	extern int panicking;
	Schedq *rq;

	if(panicking || active.exiting)
		return;

	p = t->ta;
	if((edflock(p)) == NULL)
		return;
	DPRINT("%lud releaseintr %lud[%s]\n", now, p->p_pid, statename[p->state]);
	switch(p->state){
	default:
		edfunlock();
		return;
	case Ready:
		/* remove proc from current runq */
		rq = &runq[p->priority];
		if(dequeueproc(rq, p) != p){
			DPRINT("releaseintr: can't find proc or lock race\n");
			release(p);	/* It'll start best effort */
			edfunlock();
			return;
		}
		p->state = Waitrelease;
		break;
		/* fall through */
	case Waitrelease:
		release(p);
		edfunlock();
		if(p->state == Wakeme){
			iprint("releaseintr: wakeme\n");
		}
		ready(p);
		if(up){
			up->delaysched++;
			delayedscheds++;
		}
		return;
	case Running:
		release(p);
		edfrun(p, 1);
		break;
	case Wakeme:
		release(p);
		edfunlock();
		if(p->trend)
			wakeup(p->trend);
		p->trend = nil;
		if(up){
			up->delaysched++;
			delayedscheds++;
		}
		return;
	}
	edfunlock();
}

void
edfrecord(struct proc *p)
{
	long used;
	Edf *e;
	void (*pt)(struct proc*, int, vlong);

	if((e = edflock(p)) == NULL)
		return;
	used = now - e->s;
	if(e->d - now <= 0)
		e->edfused += used;
	else
		e->extraused += used;
	if(e->S > 0){
		if(e->S <= used){
			if(pt == proctrace)
				pt(p, SSlice, 0);
			DPRINT("%lud edfrecord slice used up\n", now);
			e->d = now;
			e->S = 0;
		}else
			e->S -= used;
	}
	e->s = now;
	edfunlock();
}

void
edfrun(struct proc *p, int edfpri)
{
	Edf *e;
	void (*pt)(struct proc*, int, vlong);
	long tns;

	e = p->edf;
	/* Called with edflock held */
	if(edfpri){
		tns = e->d - now;
		if(tns <= 0 || e->S == 0){
			/* Deadline reached or resources exhausted,
			 * deschedule forthwith
			 */
			p->delaysched++;
 			delayedscheds++;
			e->s = now;
			return;
		}
		if(e->S < tns)
			tns = e->S;
		if(tns < 20)
			tns = 20;
		e->tns = 1000LL * tns;	/* µs to ns */
		if(e->tt == nil || e->tf != deadlineintr){
			DPRINT("%lud edfrun, deadline=%lud\n", now, tns);
		}else{
			DPRINT("v");
		}
		if(p->trace && (pt = proctrace))
			pt(p, SInte, todget(NULL) + e->tns);
		e->tmode = Trelative;
		e->tf = deadlineintr;
		e->ta = p;
		timeradd(e);
	}else{
		DPRINT("<");
	}
	e->s = now;
}

char *
edfadmit(struct proc *p)
{
	char *err;
	Edf *e;
	int i;
	struct proc *r;
	void (*pt)(struct proc*, int, vlong);
	long tns;

	e = p->edf;
	if (e->flags & Admitted)
		return "task state";	/* should never happen */

	/* simple sanity checks */
	if (e->T == 0)
		return "T not set";
	if (e->C == 0)
		return "C not set";
	if (e->D > e->T)
		return "D > T";
	if (e->D == 0)	/* if D is not set, set it to T */
		e->D = e->T;
	if (e->C > e->D)
		return "C > D";

	qlock(&edfschedlock);
	if (err == testschedulability(p)){
		qunlock(&edfschedlock);
		return err;
	}
	e->flags |= Admitted;

	edflock(p);

	if(p->trace && (pt = proctrace))
		pt(p, SAdmit, 0);

	/* Look for another proc with the same period to synchronize to */
	SET(r);
	for(i=0; i<conf.nproc; i++) {
		r = proctab(i);
		if(r->state == Dead || r == p)
			continue;
		if (r->edf == nil || (r->edf->flags & Admitted) == 0)
			continue;
		if (r->edf->T == e->T)
				break;
	}
	if (i == conf.nproc){
		/* Can't synchronize to another proc, release now */
		e->t = now;
		e->d = 0;
		release(p);
		if (p == up){
			DPRINT("%lud edfadmit self %lud[%s], release now: r=%lud d=%lud t=%lud\n",
				now, p->pid, statename[p->state], e->r, e->d, e->t);
			/* We're already running */
			edfrun(p, 1);
		}else{
			/* We're releasing another proc */
			DPRINT("%lud edfadmit other %lud[%s], release now: r=%lud d=%lud t=%lud\n",
				now, p->p_pid, statename[p->state], e->r, e->d, e->t);
			p->ta = p;
			edfunlock();
			qunlock(&edfschedlock);
			releaseintr(NULL, p);
			return NULL;
		}
	}else{
		/* Release in synch to something else */
		e->t = r->edf->t;
		if (p == up){
			DPRINT("%lud edfadmit self %lud[%s], release at %lud\n",
				now, p->p_pid, statename[p->state], e->t);
			edfunlock();
			qunlock(&edfschedlock);
			return NULL;
		}else{
			DPRINT("%lud edfadmit other %lud[%s], release at %lud\n",
				now, p->p_pid, statename[p->state], e->t);
			if(e->tt == NULL){
				e->tf = releaseintr;
				e->ta = p;
				tns = e->t - now;
				if(tns < 20)
					tns = 20;
				e->tns = 1000LL * tns;
				e->tmode = Trelative;
				timeradd(e);
			}
		}
	}
	edfunlock();
	qunlock(&edfschedlock);
	return NULL;
}

void
edfstop(struct proc *p)
{
	Edf *e;
	void (*pt)(struct proc*, int, vlong);

	if(e == edflock(p)){
		DPRINT("%lud edfstop %lud[%s]\n", now, p->p_pid, statename[p->state]);
		if(p->trace && (pt = proctrace))
			pt(p, SExpel, 0);
		e->flags &= ~Admitted;
		if(e->tt)
			timerdel(e);
		edfunlock();
	}
}

static int
yfn(void *)
{
	now = µs();
	return up->trend == nil || now - up->edf->r >= 0;
}

void
edfyield(void)
{
	/* sleep until next release */
	Edf *e;
	void (*pt)(struct proc*, int, vlong);
	long n;

	if((e = edflock(up)) == NULL)
		return;
	if(up->trace && (pt = proctrace))
		pt(up, SYield, 0);
	if((n = now - e->t) > 0){
		if(n < e->T)
			e->t += e->T;
		else
			e->t = now + e->T - (n % e->T);
	}
	e->r = e->t;
	e->flags |= Yield;
	e->d = now;
	if (up->tt == NULL){
		n = e->t - now;
		if(n < 20)
			n = 20;
		up->tns = 1000LL * n;
		up->tf = releaseintr;
		up->tmode = Trelative;
		up->ta = up;
		up->trend = &up->sleep;
		timeradd(up);
	}else if(up->tf != releaseintr)
		print("edfyield: surprise! %#p\n", up->tf);
	edfunlock();
	sleep(&up->sleep, yfn, NULL);
}

int
edfready(struct proc *p)
{
	Edf *e;
	Schedq *rq;
	struct proc *l, *pp;
	void (*pt)(struct proc*, int, vlong);
	long n;

	if((e = edflock(p)) == NULL)
		return 0;

	if(p->state == Wakeme && p->r){
		iprint("edfready: wakeme\n");
	}
	if(e->d - now <= 0){
		/* past deadline, arrange for next release */
		if((e->flags & Sporadic) == 0){
			/*
			 * Non sporadic processes stay true to their period;
			 * calculate next release time.
			 */
			if((n = now - e->t) > 0){
				if(n < e->T)
					e->t += e->T;
				else
					e->t = now + e->T - (n % e->T);
			}
		}
		if(now - e->t < 0){
			/* Next release is in the future, schedule it */
			if(e->t == NULL || e->tf != releaseintr){
				n = e->t - now;
				if(n < 20)
					n = 20;
				e->tns = 1000LL * n;
				e->tmode = Trelative;
				e->tf = releaseintr;
				e->ta = p;
				timeradd(e);
				DPRINT("%lud edfready %lud[%s], release=%lud\n",
					now, p->p_pid, statename[p->state], e->t);
			}
			if(p->state == Running && (e->flags & (Yield|Yieldonblock)) == 0 && (e->flags & Extratime)){
				/* If we were running, we've overrun our CPU allocation
				 * or missed the deadline, continue running best-effort at low priority
				 * Otherwise we were blocked.  If we don't yield on block, we continue
				 * best effort
				 */
				DPRINT(">");
				p->basepri = PriExtra;
				p->fixedpri = 1;
				edfunlock();
				return 0;	/* Stick on runq[PriExtra] */
			}
			DPRINT("%lud edfready %lud[%s] wait release at %lud\n",
				now, p->p_pid, statename[p->state], e->t);
			p->state = Waitrelease;
			edfunlock();
			return 1;	/* Make runnable later */
		}
		DPRINT("%lud edfready %lud %s release now\n", now, p->p_pid, statename[p->state]);
		/* release now */
		release(p);
	}
	edfunlock();
	DPRINT("^");
	rq = &runq[PriEdf];
	/* insert in queue in earliest deadline order */
	lock(runq);
	l = NULL;
	for(pp = rq->head; pp; pp = pp->p_nxt){
		if(pp->edf->d > e->d)
			break;
		l = pp;
	}
	p->p_nxt = pp;
	if (l == NULL)
		rq->head = p;
	else
		l->p_nxt = p;
	if(pp == NULL)
		rq->tail = p;
	rq->n++;
	nrdy++;
	runvec |= 1 << PriEdf;
	p->priority = PriEdf;
	p->readytime = m->ticks;
	p->state = Ready;
	unlock(runq);
	if(p->trace && (pt = proctrace))
		pt(p, SReady, 0);
	return 1;
}


static void
testenq(struct proc *p)
{
	struct proc *xp, **xpp;
	Edf *e;

	e = p->edf;
	e->testnext = NULL;
	if (qschedulability == NULL) {
		qschedulability = p;
		return;
	}
	SET(xp);
	for (xpp = &qschedulability; *xpp; xpp = &xp->edf->testnext) {
		xp = *xpp;
		if (e->testtime - xp->edf->testtime < 0
		|| (e->testtime == xp->edf->testtime && e->testtype < xp->edf->testtype)){
			e->testnext = xp;
			*xpp = p;
			return;
		}
	}
	assert(xp->edf->testnext == NULL);
	xp->edf->testnext = p;
}

static char *
testschedulability(struct proc *theproc)
{
	struct proc *p;
	long H, G, Cb, ticks;
	int steps, i;

	/* initialize */
	DPRINT("schedulability test %lud\n", theproc->p_pid);
	qschedulability = NULL;
	for(i=0; i<conf.nproc; i++) {
		p = proctab(i);
		if(p->state == Dead)
			continue;
		if ((p->edf == nil || (p->edf->flags & Admitted) == 0) && p != theproc)
			continue;
		p->edf->testtype = Rl;
		p->edf->testtime = 0;
		DPRINT("\tInit: edfenqueue %lud\n", p->p_pid);
		testenq(p);
	}
	H=0;
	G=0;
	for(steps = 0; steps < Maxsteps; steps++){
		p = qschedulability;
		qschedulability = p->edf->testnext;
		ticks = p->edf->testtime;
		switch (p->edf->testtype){
		case Dl:
			H += p->edf->C;
			Cb = 0;
			DPRINT("\tStep %3d, Ticks %lud, pid %lud, deadline, H += %lud → %lud, Cb = %lud\n",
				steps, ticks, p->pid, p->edf->C, H, Cb);
			if (H+Cb>ticks){
				DPRINT("not schedulable\n");
				return "not schedulable";
			}
			p->edf->testtime += p->edf->T - p->edf->D;
			p->edf->testtype = Rl;
			testenq(p);
			break;
		case Rl:
			DPRINT("\tStep %3d, Ticks %lud, pid %lud, release, G  %lud, C%lud\n",
				steps, ticks, p->p_pid, p->edf->C, G);
			if(ticks && G <= ticks){
				DPRINT("schedulable\n");
				return NULL;
			}
			G += p->edf->C;
			p->edf->testtime += p->edf->D;
			p->edf->testtype = Dl;
			testenq(p);
			break;
		default:
			assert(0);
		}
	}
	DPRINT("probably not schedulable\n");
	return "probably not schedulable";
}

