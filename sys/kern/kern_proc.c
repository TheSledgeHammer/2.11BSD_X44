/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_proc.c	2.1 (2.11BSD) 1999/8/11
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/malloc.h>

struct prochd qs[NQS];		/* as good a place as any... */
struct prochd rtqs[NQS];	/* Space for REALTIME queues too */
struct prochd idqs[NQS];	/* Space for IDLE queues too */

volatile struct proc *allproc;	/* all processes */
struct proc *zombproc;		/* just zombies */

/*
 * Structure associated with user cacheing.
 */
struct uidinfo {
	struct	uidinfo *ui_next;
	struct	uidinfo **ui_prev;
	uid_t	ui_uid;
	long	ui_proccnt;
} **uihashtbl;
u_long	uihash;		/* size of hash table - 1 */
#define	UIHASH(uid)	(&uihashtbl[(uid) & uihash])

void
usrinfoinit()
{
	uihashtbl = hashinit(maxproc / 16, M_PROC, &uihash);
}

/*
 * Is p an inferior of the current process?
 */
int
inferior(p)
	register struct proc *p;
{
	for (; p != u->u_procp || p != curproc; p = p->p_pptr)
		if (p->p_ppid == 0)
			return (0);
	return (1);
}

/*
 * Locate a process by number
 */
struct proc *
pfind(pid)
	register int pid;
{
	register struct proc *p = pidhash[PIDHASH(pid)];

	for (; p; p = p->p_hash)
		if (p->p_pid == pid)
			return (p);
	return ((struct proc *)0);
}

/*
 * init the process queues
 */
void
pqinit()
{
	register struct proc *p;

	/*
	 * most procs are initially on freequeue
	 *	nb: we place them there in their "natural" order.
	 */

	freeproc = NULL;
	for (p = procNPROC; --p > proc; freeproc = p)
		p->p_nxt = freeproc;

	/*
	 * but proc[0] is special ...
	 */

	allproc = p;
	p->p_nxt = NULL;
	p->p_prev = &allproc;

	zombproc = NULL;
}

/*
 * Change the count associated with number of processes
 * a given user is using.
 */
int
chgproccnt(uid, diff)
	uid_t	uid;
	int		diff;
{
		register struct uidinfo **uipp, *uip, *uiq;
		uipp = &uihashtbl[UIHASH(uid)];
		for (uip = *uipp; uip; uip = uip->ui_next)
		{
			if (uip->ui_uid == uid)
			{
				break;
			}
		}
		if (uip) {
			uip->ui_proccnt += diff;
			if (uip->ui_proccnt > 0)
			{
				return (uip->ui_proccnt);
			}
			if (uip->ui_proccnt < 0)
			{
				panic("chgproccnt: procs < 0");
			}
			if ((uiq = uip->ui_next))
			{
				uiq->ui_prev = uip->ui_prev;
			}
			*uip->ui_prev = uiq;
			rmfree(uip, sizeof(uip));
			return (0);
		}
		if (diff <= 0)
		{
			if (diff == 0)
			{
				return (0);
			}
			panic("chgproccnt: lost user");
		}
		rmalloc(uip, sizeof(*uip));
		if ((uiq = *uipp))
		{
			uiq->ui_prev = &uip->ui_next;
		}
		uip->ui_next = uiq;
		uip->ui_prev = uipp;
		*uipp = uip;
		uip->ui_uid = uid;
		uip->ui_proccnt = diff;
		return (diff);
}

/*
 * Locate a process group by number
 */
struct pgrp *
pgfind(pgid)
	register pid_t pgid;
{
	register struct pgrp *pgrp;
	for (pgrp = pgrphash[PIDHASH(pgid)]; pgrp != NULL; pgrp = pgrp->pg_hforw)
	{
		if (pgrp->pg_id == pgid)
		{
			return (pgrp);
		}
	}
	return (NULL);
}

/*
 * remove process from process group
 */
int
leavepgrp(p)
	register struct proc *p;
{
	register struct proc **pp = &p->p_pgrp->pg_mem;

	for (; *pp; pp = &(*pp)->p_pgrpnxt) {
		if (*pp == p) {
			*pp = p->p_pgrpnxt;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (pp == NULL)
		panic("leavepgrp: can't find p in pgrp");
#endif
	if (!p->p_pgrp->pg_mem)
		pgdelete(p->p_pgrp);
	p->p_pgrp = 0;
	return (0);
}

/*
 * delete a process group
 */
void
pgdelete(pgrp)
	register struct pgrp *pgrp;
{
	register struct pgrp **pgp = &pgrphash[PIDHASH(pgrp->pg_id)];

	if (pgrp->pg_session->s_ttyp != NULL &&
	    pgrp->pg_session->s_ttyp->t_pgrp == pgrp)
		pgrp->pg_session->s_ttyp->t_pgrp = NULL;
	for (; *pgp; pgp = &(*pgp)->pg_hforw) {
		if (*pgp == pgrp) {
			*pgp = pgrp->pg_hforw;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (pgp == NULL)
		panic("pgdelete: can't find pgrp on hash chain");
#endif
	if (--pgrp->pg_session->s_count == 0)
		rmfree(pgrp->pg_session, sizeof(pgrp->pg_session));
	rmfree(pgrp, sizeof(pgrp));
}

/*
 * Move p to a new or existing process group (and session)
 */
int
enterpgrp(p, pgid, mksess)
	register struct proc *p;
	pid_t pgid;
	int mksess;
{
	register struct pgrp *pgrp = pgfind(pgid);
	register struct proc **pp;
	int n;
#ifdef DIAGNOSTIC
	if (pgrp != NULL && mksess)	/* firewalls */
		panic("enterpgrp: setsid into non-empty pgrp");
	if (SESS_LEADER(p))
		panic("enterpgrp: session leader attempted setpgrp");
#endif
	if (pgrp == NULL) {
		pid_t savepid = p->p_pid;
		struct proc *np;
		/*
		 * new process group
		 */
#ifdef DIAGNOSTIC
		if (p->p_pid != pgid)
			panic("enterpgrp: new pgrp and pid != pgid");
#endif
		//MALLOC(pgrp, struct pgrp *, sizeof(struct pgrp), M_PGRP, M_WAITOK);
		RMALLOC(pgrp, struct pgrp *, sizeof(struct pgrp));
		if ((np = pfind(savepid)) == NULL || np != p)
			return (ESRCH);
		if (mksess) {
			register struct session *sess;

			/*
			 * new session
			 */
			//MALLOC(sess, struct session *, sizeof(struct session), M_SESSION, M_WAITOK);
			RMALLOC(sess, struct session *, sizeof(struct session));
			sess->s_leader = p;
			sess->s_count = 1;
			sess->s_ttyvp = NULL;
			sess->s_ttyp = NULL;
			bcopy(p->p_session->s_login, sess->s_login,
			    sizeof(sess->s_login));
			p->p_flag &= ~P_CONTROLT;
			pgrp->pg_session = sess;
#ifdef DIAGNOSTIC
			if (p != curproc)
				panic("enterpgrp: mksession and p != curproc");
#endif
		} else {
			pgrp->pg_session = p->p_session;
			pgrp->pg_session->s_count++;
		}
		pgrp->pg_id = pgid;
		pgrp->pg_hforw = pgrphash[n = PIDHASH(pgid)];
		pgrphash[n] = pgrp;
		pgrp->pg_jobc = 0;
		pgrp->pg_mem = NULL;
	} else if (pgrp == p->p_pgrp)
		return (0);

	/*
	 * Adjust eligibility of affected pgrps to participate in job control.
	 * Increment eligibility counts before decrementing, otherwise we
	 * could reach 0 spuriously during the first call.
	 */
	fixjobc(p, pgrp, 1);
	fixjobc(p, p->p_pgrp, 0);

	/*
	 * unlink p from old process group
	 */
	for (pp = &p->p_pgrp->pg_mem; *pp; pp = &(*pp)->p_pgrpnxt) {
		if (*pp == p) {
			*pp = p->p_pgrpnxt;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (pp == NULL)
		panic("enterpgrp: can't find p on old pgrp");
#endif
	/*
	 * delete old if empty
	 */
	if (p->p_pgrp->pg_mem == 0)
		pgdelete(p->p_pgrp);
	/*
	 * link into new one
	 */
	p->p_pgrp = pgrp;
	p->p_pgrpnxt = pgrp->pg_mem;
	pgrp->pg_mem = p;
	return (0);
}

static void orphanpg();

/*
 * Adjust pgrp jobc counters when specified process changes process group.
 * We count the number of processes in each process group that "qualify"
 * the group for terminal job control (those with a parent in a different
 * process group of the same session).  If that count reaches zero, the
 * process group becomes orphaned.  Check both the specified process'
 * process group and that of its children.
 * entering == 0 => p is leaving specified group.
 * entering == 1 => p is entering specified group.
 */
void
fixjobc(p, pgrp, entering)
	register struct proc *p;
	register struct pgrp *pgrp;
	int entering;
{
	register struct pgrp *hispgrp;
	register struct session *mysession = pgrp->pg_session;

	/*
	 * Check p's parent to see whether p qualifies its own process
	 * group; if so, adjust count for p's process group.
	 */
	if ((hispgrp = p->p_pptr->p_pgrp) != pgrp &&
	    hispgrp->pg_session == mysession)
		if (entering)
			pgrp->pg_jobc++;
		else if (--pgrp->pg_jobc == 0)
			orphanpg(pgrp);

	/*
	 * Check this process' children to see whether they qualify
	 * their process groups; if so, adjust counts for children's
	 * process groups.
	 */
	for (p = p->p_cptr; p; p = p->p_osptr)
		if ((hispgrp = p->p_pgrp) != pgrp &&
		    hispgrp->pg_session == mysession &&
		    p->p_stat != SZOMB)
			if (entering)
				hispgrp->pg_jobc++;
			else if (--hispgrp->pg_jobc == 0)
				orphanpg(hispgrp);
}

/*
 * A process group has become orphaned;
 * if there are any stopped processes in the group,
 * hang-up all process in that group.
 */
static void
orphanpg(pg)
	struct pgrp *pg;
{
	register struct proc *p;

	for (p = pg->pg_mem; p; p = p->p_pgrpnxt) {
		if (p->p_stat == SSTOP) {
			for (p = pg->pg_mem; p; p = p->p_pgrpnxt) {
				psignal(p, SIGHUP);
				psignal(p, SIGCONT);
			}
			return;
		}
	}
}

/* Taken From 2.11BSD's vm_proc */
/*
 * Change the size of the data+stack regions of the process.
 * If the size is shrinking, it's easy -- just release the extra core.
 * If it's growing, and there is core, just allocate it and copy the
 * image, taking care to reset registers to account for the fact that
 * the system's stack has moved.  If there is no core, arrange for the
 * process to be swapped out after adjusting the size requirement -- when
 * it comes in, enough core will be allocated.  After the expansion, the
 * caller will take care of copying the user's stack towards or away from
 * the data area.  The data and stack segments are separated from each
 * other.  The second argument to expand specifies which to change.  The
 * stack segment will not have to be copied again after expansion.
 */
void
expand(newsize, segment)
	int newsize, segment;
{
	register struct proc *p;
	register int i, n;
	int a1, a2;

	p = u->u_procp;
	if (segment == S_DATA) {
		n = p->p_dsize;
		p->p_dsize = newsize;
		a1 = p->p_daddr;
		if(n >= newsize) {
			n -= newsize;
			mfree(coremap, n, a1+newsize);
			return;
		}
	} else {
		n = p->p_ssize;
		p->p_ssize = newsize;
		a1 = p->p_saddr;
		if(n >= newsize) {
			n -= newsize;
			p->p_saddr += n;
			mfree(coremap, n, a1);
			/*
			 *  Since the base of stack is different,
			 *  segmentation registers must be repointed.
			 */
			return;
		}
	}
	if (setjmp(&u->u_ssave)) {
		if (segment == S_STACK) {
			a1 = p->p_saddr;
			i = newsize - n;
			a2 = a1 + i;
			/*
			 * i is the amount of growth.  Copy i clicks
			 * at a time, from the top; do the remainder
			 * (n % i) separately.
			 */
			while (n >= i) {
				n -= i;
				copy(a1+n, a2+n, i);
			}
			copy(a1, a2, n);
		}
		return;
	}
	if (u->u_fpsaved == 0) {
		savfp(&u->u_fps);
		u->u_fpsaved = 1;
	}
	a2 = malloc(coremap, newsize);
	if (a2 == NULL) {
		if (segment == S_DATA)
			swapout(p);
		else
			swapout(p);
		p->p_flag |= P_SSWAP;
		swtch();
		/* NOTREACHED */
	}
	if (segment == S_STACK) {
		p->p_saddr = a2;
		/*
		 * Make the copy put the stack at the top of the new area.
		 */
		a2 += newsize - n;
	} else
		p->p_daddr = a2;
	copy(a1, a2, n);
	mfree(coremap, n, a1);
}
