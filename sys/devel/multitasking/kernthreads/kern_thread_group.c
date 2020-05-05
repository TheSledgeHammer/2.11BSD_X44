/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/malloc.h>
#include "kthread.h"

LIST_HEAD(tidhashhead, kthread) *tidhashtbl;
struct tgrphashhead *tgrphashtbl;

struct 	tgrp tgrp0;
struct 	kthread kthread0;

void
threadinit()
{
	tidhashtbl = hashinit(maxthread / 4, M_THREAD, &tid_hash);
	tgrphashtbl = hashinit(maxthread / 4, M_THREAD, &tgrphash);
}

/*
 * Is kt an inferior of the current kernel thread? Update user/proc struct
 */
int
inferior(kt)
	register struct kthread *kt;
{
	for (; kt != u->u_procp || kt != curproc; kt = kt->kt_tptr)
		if (kt->kt_ptid == 0)
			return (0);
	return (1);
}

struct kthread *
ktfind(tid)
register int tid;
{
	register struct kthread *t = ktidhash(TIDHASH(tid));
	for(; t; t = t->kt_hash) {
		if(t->kt_tid == tid) {
			return (t);
		}
	}
	return ((struct kthread *) 0);
}

/*
 * remove kernel thread from thread group
 */
int
leavetgrp(kt)
	register struct kthread *kt;
{
	register struct kthread *ktt = &kt->kt_tgrp->tg_mem;

	for (; *ktt; ktt = &(*ktt)->kt_tgrpnxt) {
		if (*ktt == kt) {
			*ktt = kt->kt_tgrpnxt;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (ktt == NULL)
		panic("leavetgrp: can't find kt in tgrp");
#endif
	if (!kt->kt_tgrp->tg_mem)
		tgdelete(kt->kt_tgrp);
	kt->kt_tgrp = 0;
	return (0);
}

struct tgrp *
tgfind(tgid)
	register tid_t tgid;
{
	register struct tgrp *tgrp;
	for(tgrp = tgrphash[TIDHASH(tgid)]; tgrp != NULL; tgrp = tgrp->tg_hforw) {
		if(tgrp->tg_id == tgid) {
			return (tgrp);
		}
	}
	return (NULL);
}

/*
 * delete a thread group
 */
void
tgdelete(tgrp)
	register struct tgrp *tgrp;
{
	register struct tgrp **tgp = &tgrphash[TIDHASH(tgrp->tg_id)];

	if (tgrp->tg_session->s_ttyp != NULL &&
	    tgrp->tg_session->s_ttyp->t_pgrp == tgrp)
		tgrp->tg_session->s_ttyp->t_pgrp = NULL;
	for (; *tgp; tgp = &(*tgp)->tg_hforw) {
		if (*tgp == tgrp) {
			*tgp = tgrp->tg_hforw;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (pgp == NULL)
		panic("tgdelete: can't find tgrp on hash chain");
#endif
	if (--tgrp->tg_session->s_count == 0)
		FREE(tgrp, M_TGRP);
	FREE(tgrp, M_TGRP);
}


/*
 * Move p to a new or existing process group (and session)
 */
int
entertgrp(kt, tgid, mksess)
	register struct kthread *kt;
	tid_t tgid;
	int mksess;
{
	register struct tgrp *tgrp = tgfind(tgid);
	register struct kthread **ktt;
	int n;
#ifdef DIAGNOSTIC
	if (tgrp != NULL && mksess)	/* firewalls */
		panic("entertgrp: setsid into non-empty tgrp");
	if (SESS_LEADER(p))
		panic("entertgrp: session leader attempted set tgrp");
#endif
	if (tgrp == NULL) {
		tid_t savetid = kt->kt_tid;
		struct kthread *nkt;
		/*
		 * new process group
		 */
#ifdef DIAGNOSTIC
		if (kt->kt_pid != tgid)
			panic("entertgrp: new tgrp and tid != tgid");
#endif
		MALLOC(tgrp, struct tgrp *, sizeof(struct tgrp), M_TGRP, M_WAITOK);
		if ((nkt = tgfind(savetid)) == NULL || nkt != kt)
			return (ESRCH);
		if (mksess) {
			register struct session *sess;

			/*
			 * new session
			 */
			MALLOC(sess, struct session *, sizeof(struct session), M_SESSION, M_WAITOK);
			sess->s_leader = kt;
			sess->s_count = 1;
			sess->s_ttyvp = NULL;
			sess->s_ttyp = NULL;
			bcopy(kt->kt_session->s_login, sess->s_login,
			    sizeof(sess->s_login));
			kt->kt_flag &= ~P_CONTROLT;
			tgrp->tg_session = sess;
#ifdef DIAGNOSTIC
			if (p != curproc)
				panic("enterpgrp: mksession and p != curproc");
#endif
		} else {
			tgrp->tg_session = kt->kt_session;
			tgrp->tg_session->s_count++;
		}
		tgrp->tg_id = tgid;
		tgrp->tg_hforw = pgrphash[n = TIDHASH(tgid)];
		pgrphash[n] = tgrp;
		tgrp->tg_jobc = 0;
		tgrp->tg_mem = NULL;
	} else if (tgrp == kt->kt_tgrp)
		return (0);

	/*
	 * Adjust eligibility of affected pgrps to participate in job control.
	 * Increment eligibility counts before decrementing, otherwise we
	 * could reach 0 spuriously during the first call.
	 */
	fixjobc(kt, tgrp, 1);
	fixjobc(kt, kt->kt_tgrp, 0);

	/*
	 * unlink p from old process group
	 */
	for (ktt = &kt->kt_tgrp->tg_mem; *ktt; ktt = &(*ktt)->kt_tgrpnxt) {
		if (*ktt == kt) {
			*ktt = kt->kt_tgrpnxt;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (ktt == NULL)
		panic("entertgrp: can't find kt on old tgrp");
#endif
	/*
	 * delete old if empty
	 */
	if (kt->kt_tgrp->tg_mem == 0)
		pgdelete(kt->kt_tgrp);
	/*
	 * link into new one
	 */
	kt->kt_tgrp = tgrp;
	kt->kt_tgrpnxt = tgrp->tg_mem;
	tgrp->tg_mem = kt;
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
fixjobc(kt, tgrp, entering)
	register struct kthread *kt;
	register struct tgrp *tgrp;
	int entering;
{
	register struct tgrp *histgrp;
	register struct session *mysession = tgrp->tg_session;

	/*
	 * Check p's parent to see whether p qualifies its own process
	 * group; if so, adjust count for p's process group.
	 */
	if ((histgrp = kt->kt_tptr->kt_tgrp) != tgrp &&
	    histgrp->tg_session == mysession)
		if (entering)
			tgrp->tg_jobc++;
		else if (--tgrp->tg_jobc == 0)
			orphanpg(tgrp);

	/*
	 * Check this process' children to see whether they qualify
	 * their process groups; if so, adjust counts for children's
	 * process groups.
	 */
	for (kt = kt->kt_cptr; kt; kt = kt->kt_ostptr)
		if ((histgrp = kt->kt_tgrp) != tgrp &&
		    histgrp->tg_session == mysession &&
		    kt->kt_stat != SZOMB)
			if (entering)
				histgrp->tg_jobc++;
			else if (--histgrp->tg_jobc == 0)
				orphanpg(histgrp);
}

/*
 * A process group has become orphaned;
 * if there are any stopped processes in the group,
 * hang-up all process in that group.
 */
static void
orphanpg(tg)
	struct tgrp *tg;
{
	register struct kthread *kt;

	for (kt = tg->tg_mem; kt; kt = kt->kt_tgrpnxt) {
		if (kt->kt_stat == SSTOP) {
			for (kt = tg->tg_mem; kt; kt = kt->kt_tgrpnxt) {
				psignal(kt->kt_procp, SIGHUP);
				psignal(kt->kt_procp, SIGCONT);
			}
			return;
		}
	}
}

#ifdef DEBUG
tgrpdump()
{
	register struct tgrp *tgrp;
	register struct kthread *kt;
	register i;

	for (i = 0; i <= tgrphash; i++) {
		if (tgrp == tgrphashtbl[i]) {
			printf("\tindx %d\n", i);
			for (; tgrp != 0; tgrp = tgrp->tg_hforw) {
				printf("\ttgrp %x, tgid %d, sess %x, sesscnt %d, mem %x\n",
						tgrp, tgrp->tg_id, tgrp->tg_session,
					    tgrp->tg_session->s_count,
						tgrp->tg_mem);
				for(kt = tgrp->tg_mem; kt != 0; kt = kt->kt_nxt) {
					printf("\t\ttid %d addr %x tgrp %x\n",
										    kt->kt_tid, kt, kt->kt_tgrp);
				}
			}
		}
	}
}
#endif /* DEBUG */
