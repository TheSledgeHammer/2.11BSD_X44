/*
 * user_threads.c
 *
 *  Created on: 17 Feb 2020
 *      Author: marti
 */

#include <test/multitasking/uthreads.h>
#include <sys/malloc.h>

struct 	tgrp tgrp0;
struct 	uthread uthread0;


/*
 * Is ut an inferior of the current user thread? Update user/proc struct
 */
int
inferior(ut)
	register struct uthread *ut;
{
	for (; ut != u->u_procp || ut != curproc; ut = ut->ut_tptr)
		if (ut->ut_ptid == 0)
			return (0);
	return (1);
}

struct uthread *
utfind(tid)
	register int tid;
{
	register struct uthread *ut = utidhash(TIDHASH(tid));
	for(; ut; ut = ut->ut_hash) {
		if(ut->ut_tid == tid) {
			return (ut);
		}
	}
	return ((struct uthread *) 0);
}

/*
 * remove kernel thread from thread group
 */
int
leavetgrp(ut)
	register struct uthread *ut;
{
	register struct uthread *utt = &ut->ut_tgrp->tg_mem;

	for (; *utt; utt = &(*utt)->ut_tgrpnxt) {
		if (*utt == ut) {
			*utt = ut->ut_tgrpnxt;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (ktt == NULL)
		panic("leavetgrp: can't find kt in tgrp");
#endif
	if (!ut->ut_tgrp->tg_mem)
		tgdelete(ut->ut_tgrp);
	ut->ut_tgrp = 0;
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
entertgrp(ut, tgid, mksess)
	register struct uthread *ut;
	tid_t tgid;
	int mksess;
{
	register struct tgrp *tgrp = tgfind(tgid);
	register struct uthread **utt;
	int n;
#ifdef DIAGNOSTIC
	if (tgrp != NULL && mksess)	/* firewalls */
		panic("entertgrp: setsid into non-empty tgrp");
	if (SESS_LEADER(p))
		panic("entertgrp: session leader attempted set tgrp");
#endif
	if (tgrp == NULL) {
		tid_t savetid = ut->ut_tid;
		struct uthread *nut;
		/*
		 * new process group
		 */
#ifdef DIAGNOSTIC
		if (ut->ut_pid != tgid)
			panic("entertgrp: new tgrp and tid != tgid");
#endif
		MALLOC(tgrp, struct tgrp *, sizeof(struct tgrp), M_TGRP, M_WAITOK);
		if ((nut = tgfind(savetid)) == NULL || nut != ut)
			return (ESRCH);
		if (mksess) {
			register struct session *sess;

			/*
			 * new session
			 */
			MALLOC(sess, struct session *, sizeof(struct session), M_SESSION, M_WAITOK);
			sess->s_leader = ut;
			sess->s_count = 1;
			sess->s_ttyvp = NULL;
			sess->s_ttyp = NULL;
			bcopy(ut->ut_session->s_login, sess->s_login,
			    sizeof(sess->s_login));
			ut->ut_flag &= ~P_CONTROLT;
			tgrp->tg_session = sess;
#ifdef DIAGNOSTIC
			if (p != curproc)
				panic("enterpgrp: mksession and p != curproc");
#endif
		} else {
			tgrp->tg_session = ut->ut_session;
			tgrp->tg_session->s_count++;
		}
		tgrp->tg_id = tgid;
		tgrp->tg_hforw = pgrphash[n = TIDHASH(tgid)];
		pgrphash[n] = tgrp;
		tgrp->tg_jobc = 0;
		tgrp->tg_mem = NULL;
	} else if (tgrp == ut->ut_tgrp)
		return (0);

	/*
	 * Adjust eligibility of affected pgrps to participate in job control.
	 * Increment eligibility counts before decrementing, otherwise we
	 * could reach 0 spuriously during the first call.
	 */
	fixjobc(ut, tgrp, 1);
	fixjobc(ut, ut->ut_tgrp, 0);

	/*
	 * unlink p from old process group
	 */
	for (utt = &ut->ut_tgrp->tg_mem; *utt; utt = &(*utt)->ut_tgrpnxt) {
		if (*utt == ut) {
			*utt = ut->ut_tgrpnxt;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (utt == NULL)
		panic("entertgrp: can't find ut on old tgrp");
#endif
	/*
	 * delete old if empty
	 */
	if (ut->ut_tgrp->tg_mem == 0)
		pgdelete(ut->ut_tgrp);
	/*
	 * link into new one
	 */
	ut->ut_tgrp = tgrp;
	ut->ut_tgrpnxt = tgrp->tg_mem;
	tgrp->tg_mem = ut;
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
fixjobc(ut, tgrp, entering)
	register struct uthread *ut;
	register struct tgrp *tgrp;
	int entering;
{
	register struct tgrp *histgrp;
	register struct session *mysession = tgrp->tg_session;

	/*
	 * Check p's parent to see whether p qualifies its own process
	 * group; if so, adjust count for p's process group.
	 */
	if ((histgrp = ut->ut_tptr->ut_tgrp) != tgrp &&
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
	for (ut = ut->ut_cptr; ut; ut = ut->ut_ostptr)
		if ((histgrp = ut->ut_tgrp) != tgrp &&
		    histgrp->tg_session == mysession &&
		    ut->ut_stat != SZOMB)
			if (entering)
				histgrp->tg_jobc++;
			else if (--histgrp->tg_jobc == 0)
				orphanpg(histgrp);
}

/*
 * A thread group has become orphaned;
 * if there are any stopped processes in the group,
 * hang-up all process in that group.
 */
static void
orphanpg(tg)
	struct tgrp *tg;
{
	register struct uthread *ut;

	for (ut = tg->tg_mem; ut; ut = ut->ut_tgrpnxt) {
		if (ut->ut_stat == SSTOP) {
			for (ut = tg->tg_mem; ut; ut = ut->ut_tgrpnxt) {
				psignal(ut->ut_userp->u_procp, SIGHUP);
				psignal(ut->ut_userp->u_procp, SIGCONT);
			}
			return;
		}
	}
}

#ifdef DEBUG
tgrpdump()
{
	register struct tgrp *tgrp;
	register struct uthread *ut;
	register i;

	for (i = 0; i <= tgrphash; i++) {
		if (tgrp == tgrphashtbl[i]) {
			printf("\tindx %d\n", i);
			for (; tgrp != 0; tgrp = tgrp->tg_hforw) {
				printf("\ttgrp %x, tgid %d, sess %x, sesscnt %d, mem %x\n",
						tgrp, tgrp->tg_id, tgrp->tg_session,
					    tgrp->tg_session->s_count,
						tgrp->tg_mem);
				for(ut = tgrp->tg_mem; ut != 0; ut = ut->ut_nxt) {
					printf("\t\ttid %d addr %x tgrp %x\n",
										    ut->ut_tid, ut, ut->ut_tgrp);
				}
			}
		}
	}
}
#endif /* DEBUG */
