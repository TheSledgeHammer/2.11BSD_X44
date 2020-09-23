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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include "devel/sys/rwlock.h"
#include "devel/sys/kthread.h"

int nthread = maxthread;

/* fork kthread from an kthread overseer */
int
newthread(p, isvfork)
	struct proc *p;
	int isvfork;
{
	struct kthread *kt1, *kt2;
	static int mpid, tidchecked = 0;
	register_t *retval;
	mpid++;
retry:
	if (mpid >= PID_MAX) {
		mpid = 100;
		tidchecked = 0;
	}
	if (mpid >= tidchecked) {
		int doingzomb = 0;

		tidchecked = PID_MAX;

		kt2 = allkthread;
again:
	for (; kt2 != NULL; kt2 = kt2->kt_nxt) {
			while (kt2->kt_tid == mpid || kt2->kt_pgrp->pg_id == mpid) {
				mpid++;
				if (mpid >= tidchecked)
					goto retry;
			}
			if (kt2->kt_tid > mpid && tidchecked > kt2->kt_tid)
				tidchecked = kt2->kt_tid;
			if (kt2->kt_pgrp->pg_id > mpid && tidchecked > kt2->kt_pgrp->pg_id)
				tidchecked = kt2->kt_pgrp->pg_id;
		}
		if (!doingzomb) {
			doingzomb = 1;
			kt2 = zombkthread;
			goto again;
		}
	}
	if ((kt2 = freekthread) == NULL)
		panic("no kthreads");

	freekthread = kt2->kt_nxt; 				/* off freekthread */

	/*
	 * Make a kthread table entry for the new kthread.
	 */
	nthread++;
	kt1 = p->p_kthreado;
	kt2->kt_stat = KTSIDL;
	kt2->kt_tid = mpid;
	kt2->kt_realtimer.it_value = 0;
	kt2->kt_flag = P_SLOAD;
	kt2->kt_uid = kt1->kt_uid;
	kt2->kt_pgrp = kt1->kt_pgrp;
	kt2->kt_nice = kt1->kt_nice;
	kt2->kt_ptid = kt1->kt_tid;
	kt2->kt_pptr = kt1;
	kt2->kt_rtime = 0;
	kt2->kt_cpu = 0;
	kt2->kt_sigmask = kt1->kt_sigmask;
	kt2->kt_sigcatch = kt1->kt_sigcatch;
	kt2->kt_sigignore = kt1->kt_sigignore;
	/* take along any pending signals like stops? */
	kt2->kt_wchan = 0;
	kt2->kt_slptime = 0;

	LIST_INSERT_HEAD(TIDHASH(kt2->kt_tid), kt1->kt_procp, p_hash);

	kt2->kt_nxt = allkthread;				/* onto allkthread */
	kt2->kt_nxt->kt_prev = &kt2->kt_nxt;	/* (allkthread is never NULL) */
	kt2->kt_prev = &allkthread;
	allkthread = kt2;

	bzero(&kt2->kt_startzero, (unsigned) ((caddr_t)&kt2->kt_endzero - (caddr_t)&kt2->kt_startzero));
	bzero(&kt1->kt_startzero, (unsigned) ((caddr_t)&kt2->kt_endzero - (caddr_t)&kt2->kt_startzero));

	kt2->kt_flag = P_INMEM;
	MALLOC(kt2->kt_cred, struct pcred *, sizeof(struct pcred), M_PROC, M_WAITOK);
	bcopy(kt1->kt_cred, kt2->kt_cred, sizeof(*kt2->kt_cred));
	kt2->kt_cred->p_refcnt = 1;
	crhold(kt1->kt_ucred);

	/* bump references to the text vnode (for procfs) */
	kt2->kt_textvp = kt1->kt_textvp;
	if (kt2->kt_textvp)
		VREF(kt2->kt_textvp);

	kt2->kt_fd = fdcopy(kt1);

	if (kt1->kt_limit->p_lflags & PL_SHAREMOD)
		kt2->kt_limit = limcopy(kt1->kt_limit);
	else {
		kt2->kt_limit = kt1->kt_limit;
		kt2->kt_limit->p_refcnt++;
	}

	if (kt1->kt_session->s_ttyvp != NULL && (kt1->kt_flag & P_CONTROLT))
		kt2->kt_flag |= P_CONTROLT;

	if (isvfork)
		kt2->kt_flag |= P_PPWAIT;

	kt2->kt_pgrpnxt = kt1->kt_pgrpnxt;
	kt1->kt_pgrpnxt = kt2;
	kt2->kt_pptr = kt1;
	kt2->kt_ostptr = kt1->kt_cptr;
	if (kt1->kt_cptr)
		kt1->kt_cptr->kt_ysptr = kt2;
	kt1->kt_cptr = kt2;

	/*
	 * set priority of child to be that of parent
	 */
	kt2->kt_estcpu = kt1->kt_estcpu;

	kt1->kt_flag |= P_NOSWAP;
	retval[0] = kt1->kt_tid;
	retval[1] = 1;
	if(vm_thread_fork(kt1, kt2, isvfork)) {
		/*
		 * Child process.  Set start time and get to work.
		 */
		(void) splclock();
		kt2->kt_stats->p_start = time;
		(void) spl0();
		kt2->kt_acflag = AFORK;
		return (0);
	}

	kt2->kt_stat = KTSRUN;

	kt1->kt_flag |= P_NOSWAP;

	retval[0] = kt2->kt_tid;
	retval[1] = 0;
	return (0);
}

#include <sys/resourcevar.h>
int
vm_thread_fork(kt1, kt2, isvfork)
	register struct kthread *kt1, *kt2;
	int isvfork;
{
	register struct user *up;
	vm_offset_t addr;

	(void)vm_map_inherit(&kt1->kt_vmspace->vm_map, UPT_MIN_ADDRESS-UPAGES*NBPG, VM_MAX_ADDRESS, VM_INHERIT_NONE);

	kt2->kt_vmspace = vmspace_fork(kt1->kt_vmspace);

	/*
	 * p_stats and p_sigacts currently point at fields
	 * in the user struct but not at &u, instead at p_addr.
	 * Copy p_sigacts and parts of p_stats; zero the rest
	 * of p_stats (statistics).
	 */
	kt2->kt_stats = &up->u_stats;
	kt2->kt_sigacts = &up->u_sigacts;
	up->u_sigacts = *kt1->kt_sigacts;
	bzero(&up->u_stats.pstat_startzero, (unsigned) ((caddr_t) &up->u_stats.pstat_endzero - (caddr_t) &up->u_stats.pstat_startzero));
	bcopy(&kt1->kt_stats->pstat_startcopy, &up->u_stats.pstat_startcopy, ((caddr_t) &up->u_stats.pstat_endcopy - (caddr_t) &up->u_stats.pstat_startcopy));

	return (0);
}
