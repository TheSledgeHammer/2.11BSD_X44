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

extern struct kthread 			kthread0;
struct kthread *curkthread = 	&kthread0;

void
startkthread(kt)
	register struct kthread *kt;
{
	/* initialize current kthread(0) */
    kt = &kthread0;
    curkthread = kt;

    /* Set kthread to idle & waiting */
    kt->kt_stat |= KTSIDL | KTSWAIT | KTSREADY;

    /* init kthread queues */
    ktqinit();

    /* setup kthread locks */
    kthread_lock_init(kthread_lkp, kt);
    kthread_rwlock_init(kthread_rwl, kt);
}

int
kthread_create(p)
	struct proc *p;
{
	register struct kthread *kt;
	int error;

	if(!proc0.p_stat) {
		panic("kthread_create called too soon");
	}

	if(kt == NULL) {
		kt = p->p_kthreado;
	}

	return (0);
}

int
kthread_join(kthread_t kt)
{

}

int
kthread_cancel(kthread_t kt)
{

}

int
kthread_exit(kthread_t kt)
{

}

int
kthread_detach(kthread_t kt)
{

}

int
kthread_equal(kthread_t kt1, kthread_t kt2)
{
	if(kt1 > kt2) {
		return (1);
	} else if(kt1 < kt2) {
		return (-1);
	}
	return (0);
}

int
kthread_kill(kthread_t kt)
{

}

/*
 * init the kthread queues
 */
void
ktqinit()
{
	register struct kthread *kt;

	freekthread = NULL;
	for (kt = kthreadNKTHREAD; --kt > kthread0; freekthread = kt)
		kt->kt_nxt = freekthread;

	allkthread = kt;
	kt->kt_nxt = NULL;
	kt->kt_prev = &allkthread;

	zombkthread = NULL;
}

/*
 * Locate a kthread by number
 */
struct kthread *
ktfind(pid)
	register int pid;
{
	register struct kthread *kt;
	for (kt = PIDHASH(pid); kt != 0; kt = kt->kt_hash.le_next) {
		if(kt->kt_tid == pid) {
			return (kt);
		}
	}
	return (NULL);
}

/* Threadpool's FIFO Queue (IPC) */
void
kthreadpool_itc_send(ktpool, itc)
    struct kthreadpool *ktpool;
	struct threadpool_itpc *itc;
{
    /* command / action */
	itc->itc_ktpool = ktpool;
	itc->itc_job = ktpool->ktp_jobs;  /* add/ get current job */
	/* send flagged jobs */
	ktpool->ktp_issender = TRUE;
	ktpool->ktp_isreciever = FALSE;

	/* update job pool */
}

void
kthreadpool_itc_recieve(ktpool, itc)
    struct kthreadpool *ktpool;
	struct threadpool_itpc *itc;
{
    /* command / action */
	itc->itc_ktpool = ktpool;
	itc->itc_job = ktpool->ktp_jobs; /* add/ get current job */
	ktpool->ktp_issender = FALSE;
	ktpool->ktp_isreciever = TRUE;

	/* update job pool */
}

/* Initialize a Mutex on a kthread
 * Setup up Error flags */
int
kthread_lock_init(lkp, kt)
    struct lock *lkp;
    kthread_t kt;
{
    int error = 0;
    lockinit(lkp, lkp->lk_prio, lkp->lk_wmesg, lkp->lk_timo, lkp->lk_flags);
    set_kthread_lock(lkp, kt);
    if(lkp->lk_ktlockholder == NULL) {
    	panic("kthread lock unavailable");
    	error = EBUSY;
    }
    return (error);
}

int
kthread_lockmgr(lkp, flags, kt)
	struct lock *lkp;
	u_int flags;
    kthread_t kt;
{
    pid_t pid;
    if (kt) {
        pid = kt->kt_tid;
    } else {
        pid = LK_KERNPROC;
    }
    return lockmgr(lkp, flags, lkp->lk_lnterlock, pid);
}

/* Initialize a rwlock on a kthread
 * Setup up Error flags */
int
kthread_rwlock_init(rwl, kt)
	rwlock_t rwl;
	kthread_t kt;
{
	int error = 0;
	rwlock_init(rwl, rwl->rwl_prio, rwl->rwl_wmesg, rwl->rwl_timo, rwl->rwl_flags);
	set_kthread_rwlock(rwl, kt);
	return (error);
}

int
kthread_rwlockmgr(rwl, flags, kt)
	rwlock_t rwl;
	u_int flags;
	kthread_t kt;
{
	pid_t pid;
	if (kt) {
		pid = kt->kt_tid;
	} else {
		pid = LK_KERNPROC;
	}
	return rwlockmgr(rwl, flags, pid);
}

int
newkthread(p, isvfork)
	struct proc *p;
	int isvfork;
{
	struct kthread *kt1, *kt2;
	static int mpid, pidchecked = 0;
	register_t *retval;
	mpid++;
retry:
	if (mpid >= PID_MAX) {
		mpid = 100;
		pidchecked = 0;
	}
	if (mpid >= pidchecked) {
		int doingzomb = 0;

		pidchecked = PID_MAX;

		kt2 = allkthread;
again:
	for (; kt2 != NULL; kt2 = kt2->kt_nxt) {
			while (kt2->kt_tid == mpid || kt2->kt_pgrp->pg_id == mpid) {
				mpid++;
				if (mpid >= pidchecked)
					goto retry;
			}
			if (kt2->kt_tid > mpid && pidchecked > kt2->kt_tid)
				pidchecked = kt2->kt_tid;
			if (kt2->kt_pgrp->pg_id > mpid && pidchecked > kt2->kt_pgrp->pg_id)
				pidchecked = kt2->kt_pgrp->pg_id;
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
	nproc++;
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

	LIST_INSERT_HEAD(PIDHASH(kt2->kt_tid), kt1->kt_procp, p_hash);

	kt2->kt_nxt = allkthread;				/* onto allkthread */
	kt2->kt_nxt->kt_prev = &kt2->kt_nxt;	/* (allkthread is never NULL) */
	kt2->kt_prev = &allkthread;
	allkthread = kt2;

	bzero(&kt2->kt_startzero, (unsigned) ((caddr_t)&kt2->kt_endzero - (caddr_t)&kt2->kt_startzero));
	bzero(&kt1->kt_startzero, (unsigned) ((caddr_t)&kt2->kt_endzero - (caddr_t)&kt2->kt_startzero));

	kt2->kt_flag = P_INMEM;
	MALLOC(kt2->kt_cred, struct pcred *, sizeof(struct pcred), M_SUBPROC, M_WAITOK);
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
	if(vm_kthread_fork(kt1, kt2, isvfork)) {
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

int
vm_kthread_fork(kt1, kt2, isvfork)
	struct kthread *kt1, *kt2;
	int isvfork;
{

	return (cpu_kthread_fork(kt1, kt2));
}

int
cpu_kthread_fork(kt1, kt2)
	struct kthread *kt1, *kt2;
{
	return (0);
}
