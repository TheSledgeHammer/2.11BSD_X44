/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_proc.c	2.1 (2.11BSD) 1999/8/11
 */
/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kern_proc.c	8.10 (Berkeley) 2/23/95
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/acct.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <ufs/ufs/quota.h>
#include <sys/uio.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mutex.h>
#include <sys/null.h>

/*
 * Structure associated with user cacheing.
 */
struct uidinfo {
	LIST_ENTRY(uidinfo) ui_hash;
	uid_t				ui_uid;
	long				ui_proccnt;
};
#define	UIHASH(uid)	(&uihashtbl[(uid) & uihash])
LIST_HEAD(uihashhead, uidinfo) *uihashtbl;
u_long	uihash;					/* size of hash table - 1 */

struct pidhashhead *pidhashtbl;
u_long 				pidhash;
struct pgrphashhead *pgrphashtbl;
u_long 				pgrphash;

struct proclist 	allproc;
struct proclist 	zombproc;
struct proclist 	freeproc;

struct lock_holder 	proc_loholder;

void	pqinit(struct proc *);

void
procinit(p)
	struct proc *p;
{
	pqinit(p);
	pidhashtbl = hashinit(maxproc / 4, M_PROC, &pidhash);
	pgrphashtbl = hashinit(maxproc / 4, M_PROC, &pgrphash);
	uihashtbl = hashinit(maxproc / 16, M_PROC, &uihash);

	/* init proc lock (mutex) */
	mtx_init(p->p_mtx, &proc_loholder, "proc_mutex", (struct proc *)p, p->p_pid);
}

/*
 * init the process queues
 */
void
pqinit(p)
	register struct proc *p;
{
	LIST_INIT(&allproc);
	LIST_INIT(&zombproc);
	LIST_INIT(&freeproc);

	/*
	 * most procs are initially on freequeue
	 *	nb: we place them there in their "natural" order.
	 */
	LIST_INSERT_HEAD(&freeproc, p, p_list);

	/*
	 * but proc0 is special ...
	 */
	LIST_INSERT_HEAD(&allproc, p, p_list);

	LIST_FIRST(&zombproc) = NULL;
}

/*
 * Is p an inferior of the current process?
 */
int
inferior(p)
	register struct proc *p;
{
	for (; (p != u.u_procp || p != curproc); p = p->p_pptr)
		if (p->p_ppid == 0)
			return (0);
	return (1);
}

/*
 * Locate a process by number
 */
struct proc *
pfind(pid)
	register pid_t pid;
{
	register struct proc *p;
	for (p = LIST_FIRST(PIDHASH(pid)); p != 0; p = LIST_NEXT(p, p_hash))
		if (p->p_pid == pid)
			return (p);
	return (NULL);
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
		register struct uidinfo *uip, *uiq;
		register struct uihashhead *uipp;
		uipp = UIHASH(uid);

		for (uip = LIST_FIRST(uipp); uip != 0; uip = LIST_NEXT(uip, ui_hash)) {
			if (uip->ui_uid == uid) {
				break;
			}
		}
		if (uip) {
			uip->ui_proccnt += diff;
			if (uip->ui_proccnt > 0) {
				return (uip->ui_proccnt);
			}
			if (uip->ui_proccnt < 0) {
				panic("chgproccnt: procs < 0");
			}
			LIST_REMOVE(uip, ui_hash);
			FREE(uip, M_PROC);
			return (0);
		}
		if (diff <= 0) {
			if (diff == 0) {
				return (0);
			}
			panic("chgproccnt: lost user");
		}
		MALLOC(uip, struct uidinfo *, sizeof(*uip), M_PROC, M_WAITOK);
		LIST_INSERT_HEAD(uipp, uip, ui_hash);
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
	for (pgrp = LIST_FIRST(PGRPHASH(pgid)); pgrp != NULL; pgrp = LIST_NEXT(pgrp, pg_hash)) {
		if (pgrp->pg_id == pgid) {
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
	LIST_REMOVE(p, p_pglist);
	if(LIST_FIRST(&p->p_pgrp->pg_mem) == 0) {
		pgdelete(p->p_pgrp);
	}
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
	if (pgrp->pg_session->s_ttyp != NULL
			&& pgrp->pg_session->s_ttyp->t_pgrp == pgrp)
		pgrp->pg_session->s_ttyp->t_pgrp = NULL;
	LIST_REMOVE(pgrp, pg_hash);
	if (--pgrp->pg_session->s_count == 0)
		FREE(pgrp, M_PGRP);
	FREE(pgrp, M_PGRP);
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
	register struct pgrp *pgrp;
	register struct proc **pp;
	int n;

	pgrp = pgfind(pgid);
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
		MALLOC(pgrp, struct pgrp *, sizeof(struct pgrp), M_PGRP, M_WAITOK);
		if ((np = pfind(savepid)) == NULL || np != p)
			return (ESRCH);

		if (mksess) {
			register struct session *sess;

			/*
			 * new session
			 */
			MALLOC(sess, struct session *, sizeof(struct session), M_SESSION, M_WAITOK);
			sess->s_leader = p;
			sess->s_count = 1;
			sess->s_ttyvp = NULL;
			sess->s_ttyp = NULL;
			bcopy(p->p_session->s_login, sess->s_login, sizeof(sess->s_login));
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
		LIST_INIT(&pgrp->pg_mem);
		LIST_INSERT_HEAD(PGRPHASH(pgid), pgrp, pg_hash);
		pgrp->pg_jobc = 0;
	} else if (pgrp == p->p_pgrp) {
		return (0);
	}

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
	LIST_REMOVE(p, p_pglist);

	/*
	 * delete old if empty
	 */
	if (LIST_EMPTY(&p->p_pgrp->pg_mem))
		pgdelete(p->p_pgrp);
	/*
	 * link into new one
	 */
	p->p_pgrp = pgrp;
	LIST_INSERT_HEAD(&pgrp->pg_mem, p, p_pglist);
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
	for (p = LIST_FIRST(&p->p_children); p; p = LIST_NEXT(p, p_sibling)) {
		if ((hispgrp = p->p_pgrp) != pgrp&&
		hispgrp->pg_session == mysession &&
		p->p_stat != SZOMB)
			if (entering)
				hispgrp->pg_jobc++;
			else if (--hispgrp->pg_jobc == 0)
				orphanpg(hispgrp);
	}
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

	for (p = LIST_FIRST(&pg->pg_mem); p; p = LIST_NEXT(p, p_pglist)) {
		if (p->p_stat == SSTOP) {
			for (p = LIST_FIRST(&pg->pg_mem); p; p = LIST_NEXT(p, p_pglist)) {
				psignal(p, SIGHUP);
				psignal(p, SIGCONT);
			}
			return;
		}
	}
}

#ifdef DEBUG
pgrpdump()
{
	register struct pgrp *pgrp;
	register struct proc *p;
	register i;

	for (i = 0; i <= pgrphash; i++) {
		if (pgrp == LIST_FIRST(&pgrphashtbl[i])) {
			printf("\tindx %d\n", i);
			for (; pgrp != 0; pgrp = LIST_NEXT(pgrp, pg_hash)) {
				printf("\tpgrp %x, pgid %d, sess %x, sesscnt %d, mem %x\n",
						pgrp, pgrp->pg_id, pgrp->pg_session,
						pgrp->pg_session->s_count, pgrp->pg_mem);
				for (p = LIST_FIRST(&pgrp->pg_mem); p != 0;
						p = LIST_NEXT(p, p_pglist)) {
					printf("\t\tpid %d addr %x pgrp %x\n", p->p_pid, p,
							p->p_pgrp);
				}
			}
		}
	}
}
#endif /* DEBUG */
