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

/* NOTE:
 * Libuthread
 * - Is a proof of concept that is still in early development.
 * - To be moved into userspace
*/

#include "uthread.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/rwlock.h>
#include <sys/user.h>
#include "devel/sys/kthread.h"

extern struct uthread uthread0;
struct uthread *curuthread = &uthread0;
int	uthread_create_now;

void
uthread_init(kt, ut)
	register struct kthread *kt;
	register struct uthread *ut;
{
	register_t rval[2];
	int error;

	/* initialize current uthread(0) */
    ut = &kthread0->kt_uthreado = &uthread0;
    curuthread = ut;

    /* set up uthreads */
    alluthread = (struct uthread *)ut;
    ut->ut_prev = (struct uthread **)&alluthread;
    tgrphash[0] = &pgrp0;
    kthread0->kt_uthreado = ut;

	/* give the uthread the same creds as the initial kthread */
	ut->ut_ucred = kt->kt_ucred;
	crhold(ut->ut_ucred);

    /* setup uthread locks */
    uthread_lock_init(uthread_lkp, ut);
    uthread_rwlock_init(uthread_rwl, ut);

	/* initialize uthreadpools  */
	uthreadpool_init();
}

int
uthread_create(func, arg, newkp, name)
	void (*func)(void *);
	void *arg;
	struct kthread **newkp;
	const char 		*name;
{
	struct kthread *kt;
	register_t rval[2];

	error = newproc(0);
	if(__predict_false(error != 0)) {
		panic("uthread_create");
		return (error);
	}

	if(rval[1]) {
		kt->kt_flag |= KT_SYSTEM | KT_NOCLDWAIT;
		kt->kt_uthreado->ut_flag |= UT_INMEM | UT_SYSTEM;
	}

	/* Name it as specified. */
	bcopy(kt->kt_uthreado->ut_comm, name, MAXCOMLEN);

	if (newkp != NULL) {
		*newkp = kt;
	}
	return (0);
}

/*
 * Cause a kernel thread to exit.  Assumes the exiting thread is the
 * current context.
 */
void
uthread_exit(ecode)
	int ecode;
{
	/*
	 * XXX What do we do with the exit code?  Should we even bother
	 * XXX with it?  The parent (kthread0) isn't going to do much with
	 * XXX it.
	 */
	if (ecode != 0) {
		printf("WARNING: thread `%s' (%d) exits with status %d\n", curkthread->kt_comm, curkthread->kt_tid, ecode);
	}
	exit(W_EXITCODE(ecode, 0));

	for (;;);
}

struct uthread_queue {
	SIMPLEQ_ENTRY(uthread_queue) 	uq_q;
	void 							(*uq_func)(void *);
	void 							*uq_arg;
};

SIMPLEQ_HEAD(, uthread_queue) uthread_queue = SIMPLEQ_HEAD_INITIALIZER(uthread_queue);

void
uthread_create_deferred(void (*func)(void *), void *arg)
{
	struct uthread_queue *uq;
	if (uthread_create_now) {
		(*func)(arg);
		return;
	}

	uq = malloc(sizeof *uq, M_TEMP, M_NOWAIT | M_ZERO);
	if (uq == NULL) {
		panic("unable to allocate kthread_queue");
	}

	uq->uq_func = func;
	uq->uq_arg = arg;

	SIMPLEQ_INSERT_TAIL(&uthread_queue, uq, uq_q);
}

void
uthread_run_deferred_queue(void)
{
	struct uthread_queue *uq;

	/* No longer need to defer uthread creation. */
	uthread_create_now = 1;

	while ((uq = SIMPLEQ_FIRST(&uthread_queue)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&uthread_queue, uq_q);
		(*uq->uq_func)(uq->uq_arg);
		free(uq, M_TEMP, sizeof(*uq));
	}
}

void
uthreadpool_itpc_send(itpc, utpool, cmd)
	struct threadpool_itpc *itpc;
    struct uthreadpool *utpool;
    int cmd;
{
    /* sync itpc to threadpool */
	itpc->itc_utpool = utpool;
	itpc->itc_jobs = utpool->utp_jobs;
	utpool->utp_issender = TRUE;
	utpool->utp_isreciever = FALSE;

	/* command / action */
	switch(cmd) {
	case ITPC_SCHEDULE:
		uthreadpool_schedule_job(utpool, utpool->utp_jobs);
		break;

	case ITPC_CANCEL:
		uthreadpool_cancel_job(utpool, utpool->utp_jobs);
		break;

	case ITPC_DESTROY:
		uthreadpool_job_destroy(utpool->utp_jobs);
		break;

	case ITPC_DONE:
		uthreadpool_job_done(utpool->utp_jobs);
		break;
	}

	itpc_check_uthreadpool(itpc, utpool);

	/* update job pool */
}

void
uthreadpool_itpc_recieve(itpc, utpool, cmd)
	struct threadpool_itpc *itpc;
	struct uthreadpool 		*utpool;
	int cmd;
{
	/* sync itpc to threadpool */
	itpc->itc_utpool = utpool;
	itpc->itc_jobs = utpool->utp_jobs;
	utpool->utp_issender = FALSE;
	utpool->utp_isreciever = TRUE;

	/* command / action */
	switch(cmd) {
	case ITPC_SCHEDULE:
		uthreadpool_schedule_job();
		break;
	case ITPC_CANCEL:
		uthreadpool_cancel_job();
		break;
	case ITPC_DESTROY:
		uthreadpool_job_destroy(itpc->itc_jobs);
		utpool->utp_jobs = itpc->itc_jobs;
		break;
	case ITPC_DONE:
		uthreadpool_job_done();
		break;
	}

	itpc_verify_uthreadpool(itpc, utpool);

	/* update job pool */
}

/* Initialize a Mutex on a kthread
 * Setup up Error flags */
int
uthread_lock_init(lkp, ut)
    struct lock *lkp;
    uthread_t ut;
{
    int error = 0;
    lockinit(lkp, lkp->lk_prio, lkp->lk_wmesg, lkp->lk_timo, lkp->lk_flags);
	set_uthread_lockholder(lkp->lk_lockholder, ut);
    if(get_uthread_lockholder(lkp->lk_lockholder, ut->ut_tid) == NULL) {
    	panic("uthread lock unavailable");
    	error = EBUSY;
    }
    return (error);
}

int
uthread_lockmgr(lkp, flags, ut)
	struct lock *lkp;
	u_int flags;
	uthread_t ut;
{
    pid_t pid;
    if (ut) {
        pid = ut->ut_tid;
    } else {
        pid = LK_KERNPROC;
    }
    return lockmgr(lkp, flags, lkp->lk_lnterlock, pid);
}

/* Initialize a rwlock on a uthread
 * Setup up Error flags */
int
uthread_rwlock_init(rwl, ut)
	rwlock_t rwl;
	uthread_t ut;
{
	int error = 0;
	rwlock_init(rwl, rwl->rwl_prio, rwl->rwl_wmesg, rwl->rwl_timo, rwl->rwl_flags);
	set_uthread_lockholder(rwl->rwl_lockholder, ut);
    if(get_uthread_lockholder(rwl->rwl_lockholder, ut->ut_tid) == NULL) {
    	panic("uthread rwlock unavailable");
    	error = EBUSY;
    }
	return (error);
}

int
uthread_rwlockmgr(rwl, flags, ut)
	rwlock_t rwl;
	u_int flags;
	uthread_t ut;
{
	pid_t pid;
	if (ut) {
		pid = ut->ut_tid;
	} else {
		pid = LK_KERNPROC;
	}
	return rwlockmgr(rwl, flags, pid);
}

/*
 * Locate a uthread by number
 */
struct uthread *
utfind(tid)
	register int tid;
{
	register struct uthread *ut;
	for (ut = TIDHASH(tid); ut != 0; ut = LIST_NEXT(ut, ut_hash)) {
		if(ut->ut_tid == tid) {
			return (ut);
		}
	}
	 return (NULL);
}

/*
 * remove uthread from thread group
 */
int
leaveutgrp(ut)
	register struct uthread *ut;
{
	register struct uthread **utt = &ut->ut_pgrp->pg_mem;

	for (; *utt; utt = &(*utt)->ut_pgrpnxt) {
		if (*utt == ut) {
			*utt = ut->ut_pgrpnxt;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (utt == NULL)
		panic("leavetgrp: can't find uthread in tgrp");
#endif
	if (!ut->ut_pgrp->pg_mem)
		tgdelete(ut->ut_pgrp);
	ut->ut_pgrp = 0;
	return (0);
}
