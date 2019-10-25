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
#define	UIHASH(uid)	((uid) & uihash)

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
			mfree(uip);
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
		malloc(uip, sizeof(*uip));
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

