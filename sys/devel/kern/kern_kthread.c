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
#include <sys/rwlock.h>
#include "devel/sys/kthread.h"

#include <vm/include/vm_param.h>

extern struct kthread 			kthread0;
struct kthread *curkthread = 	&kthread0;

void
kthread_init(p, kt)
	register struct proc  *p;
	register struct kthread *kt;
{
	register_t rval[2];
	int error;

	/* initialize current kthread 0 */
    kt = &kthread0;
    curkthread = kt;

	/* Initialize kthread and kthread group structures. */
    threadinit();

	/* set up kernel thread */
    allkthread = (struct kthread *) kt;
    kt->kt_prev = (struct kthread **)&allkthread;
	tgrphash[0] = &pgrp0;

    /* Set kthread to idle & waiting */
    kt->kt_stat |= KT_SIDL | KT_SWAIT | KT_SREADY;

    /* setup kthread locks */
    kthread_lock_init(kthread_lkp, kt);
    kthread_rwlock_init(kthread_rwl, kt);

	/* kthread overseer */
	if(newproc(0))
		panic("kthread overseer creation");
	if(rval[1])
		kt = p->p_kthreado;
		kt->kt_flag |= KT_INMEM | KT_SYSTEM;

		/* initialize kthreadpools */
		kthreadpools_init();
}

/* create new kthread */
int
kthread_create(kt)
	struct kthread *kt;
{
	struct kthread *newthread;
	register_t rval[2];

	if(newkthread(0)) {
		panic("kthread creation");
	}

	if(rval[1]) {
		newthread = kt;
		newthread->kt_flag |= KT_INMEM | KT_SYSTEM;
	}

	return (0);
}

int
kthread_join(kthread_t kt)
{
	return (0);
}

int
kthread_cancel(kthread_t kt)
{
	return (0);
}

int
kthread_exit(rv)
	int rv;
{
	register struct kthread *kt;
	register struct proc 	*p;
	register struct vmspace *vm;

	if(kt->kt_tid == 1) {
		panic("init died (signal %d, exit %d)", WTERMSIG(rv), WEXITSTATUS(rv));
	}

	MALLOC(kt->kt_ru, struct rusage *, sizeof(struct rusage), M_ZOMBIE, M_WAITOK);
	kt = p->p_kthreado;
	kt->kt_flag &= ~(KT_TRACED | KT_PPWAIT | KT_SULOCK);
	kt->kt_sigignore = ~0;
	kt->kt_sigacts = 0;
	untimeout(realitexpire, (caddr_t)kt);

	fdfree(kt->kt_procp);

	vm = kt->kt_vmspace;
	if (vm->vm_refcnt == 1) {
		(void) vm_map_remove(&vm->vm_map, VM_MIN_ADDRESS, VM_MAXUSER_ADDRESS);
	}

	if (kt->kt_tid == 1)
		panic("init died");
	if (*kt->kt_prev == kt->kt_nxt)		/* off allkthread queue */
		kt->kt_nxt->kt_prev = kt->kt_prev;
	if (kt->kt_nxt == zombkthread)		/* onto zombkthread */
		kt->kt_nxt->kt_prev = &kt->kt_nxt;
	kt->kt_prev = &zombkthread;
	zombkthread = kt;
	kt->kt_stat = KT_SZOMB;

	LIST_REMOVE(kt->kt_procp, p_hash);

	//calcru(p, &p->p_ru->ru_utime, &p->p_ru->ru_stime, NULL);
	ruadd(&kt->kt_procp->p_ru, &u->u_cru);

	curkthread = NULL;
	return (0);
}

int
kthread_detach(kthread_t kt)
{
	return (0);
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
	return (0);
}

/* Threadpool's FIFO Queue (IPC) */
void
kthreadpool_itc_send(itpc, ktpool)
	struct threadpool_itpc *itpc;
    struct kthreadpool *ktpool;
{
    /* command / action */
    itpc->itc_ktpool = ktpool;
    itpc->itc_jobs = ktpool->ktp_jobs;  /* add/ get current job */
	/* send flagged jobs */
	ktpool->ktp_issender = TRUE;
	ktpool->ktp_isreciever = FALSE;

	/* update job pool */
}

void
kthreadpool_itc_recieve(itpc, ktpool)
	struct threadpool_itpc *itpc;
    struct kthreadpool *ktpool;
{
    /* command / action */
	itpc->itc_ktpool = ktpool;
	itpc->itc_jobs = ktpool->ktp_jobs; /* add/ get current job */
	ktpool->ktp_issender = FALSE;
	ktpool->ktp_isreciever = TRUE;

	/* update job pool */
}

/* Initialize a lock on a kthread */
int
kthread_lock_init(lkp, kt)
    struct lock *lkp;
    kthread_t kt;
{
    int error = 0;
    lockinit(lkp, lkp->lk_prio, lkp->lk_wmesg, lkp->lk_timo, lkp->lk_flags);
    set_kthread_lockholder(lkp->lk_lockholder, kt);
    if(get_kthread_lockholder(lkp->lk_lockholder, kt->kt_tid) == NULL) {
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

/* Initialize a rwlock on a kthread */
int
kthread_rwlock_init(rwl, kt)
	rwlock_t rwl;
	kthread_t kt;
{
	int error = 0;
	rwlock_init(rwl, rwl->rwl_prio, rwl->rwl_wmesg, rwl->rwl_timo, rwl->rwl_flags);
	set_kthread_lockholder(rwl->rwl_lockholder, kt);
    if(get_kthread_lockholder(rwl->rwl_lockholder, kt->kt_tid) == NULL) {
    	panic("kthread rwlock unavailable");
    	error = EBUSY;
    }
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

/*
 * Locate a kthread by number
 */
struct kthread *
ktfind(tid)
	register int tid;
{
	register struct kthread *kt;
	for (kt = TIDHASH(tid); kt != 0; kt = LIST_NEXT(kt, kt_hash))
		if(kt->kt_tid == tid)
			return (kt);
	return (NULL);
}

/*
 * remove kthread from thread group
 */
int
leavektgrp(kt)
	register struct kthread *kt;
{
	register struct kthread **ktt = &kt->kt_pgrp->pg_mem;

	for (; *ktt; ktt = &(*ktt)->kt_pgrpnxt) {
		if (*ktt == kt) {
			*ktt = kt->kt_pgrpnxt;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (ktt == NULL)
		panic("leavepgrp: can't find p in pgrp");
#endif
	if (!kt->kt_pgrp->pg_mem)
		tgdelete(kt->kt_pgrp);
	kt->kt_pgrp = 0;
	return (0);
}
