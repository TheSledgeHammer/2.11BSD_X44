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

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include "devel/sys/mutex.h"
#include "devel/sys/rwlock.h"
#include "devel/sys/uthread.h"

extern struct uthread uthread0;
extern struct uthreadpool uthreadpool;
struct uthread *curuthread = &uthread0;

void
startuthread(ut)
	register struct uthread *ut;
{
	/* initialize current uthread(0) */
    ut = &uthread0;
    curuthread = ut;

    /* Set thread to idle & waiting */
    ut->ut_stat |= TSIDL | TSWAIT | TSREADY;

    /* setup uthread mutex manager */
    uthread_mutex_init(uthread_mtx, ut);

    /* setup uthreadpool */
    start_uthreadpool(&uthreadpool);
}

void
start_uthreadpool(utpool)
	struct uthreadpool *utpool;
{
	TAILQ_INIT(utpool->utp_idle_threads);
	if(utpool == NULL) {
		MALLOC(utpool, struct uthreadpool *, sizeof(struct uthreadpool *), M_UTHREADPOOL, M_WAITOK);
	}
}

int
uthread_create(kt)
	struct kthread *kt;
{
	register struct uthread *ut;
	int error;

	if(!kthread0.kt_stat) {
		panic("uthread_create called too soon");
	}
	if(ut == NULL) {
		startuthread(ut);
	}
}

int
uthread_join(uthread_t ut)
{


}

int
uthread_cancel(uthread_t ut)
{

}

int
uthread_exit(uthread_t ut)
{

}

int
uthread_detach(uthread_t ut)
{

}

int
uthread_equal(uthread_t ut1, uthread_t ut2)
{
	if(ut1 > ut2) {
		return (1);
	} else if(ut1 < ut2) {
		return (-1);
	}
	return (0);
}

int
uthread_kill(uthread_t ut)
{

}

void
uthreadpool_itc_send(utpool, itc)
    struct uthreadpool *utpool;
	struct threadpool_itpc *itc;
{
    /* command / action */
	itc->itc_utpool = utpool;
	itc->itc_job = utpool->utp_jobs;
	/* send flagged jobs */
	utpool->utp_issender = TRUE;
	utpool->utp_isreciever = FALSE;

	/* update job pool */
}

void
uthreadpool_itc_recieve(utpool, itc)
    struct uthreadpool *utpool;
	struct threadpool_itpc *itc;
{
    /* command / action */
	itc->itc_utpool = utpool;
	itc->itc_job = utpool->utp_jobs;
	utpool->utp_issender = FALSE;
	utpool->utp_isreciever = TRUE;

	/* update job pool */
}

/* Initialize a Mutex on a kthread
 * Setup up Error flags */
int
uthread_mutex_init(lkp, ut)
    struct lock *lkp;
    uthread_t ut;
{
    int error = 0;
    lockinit(lkp, lkp->lk_prio, lkp->lk_wmesg, lkp->lk_timo, lkp->lk_flags);
    ut->ut_lock = ut;
    lkp->lk_utlockholder = ut;
    return (error);
}

int
uthread_mutexmgr(lkp, flags, ut)
	struct lock *lkp;
	u_int flags;
	uthread_t ut;
{
    pid_t pid;
    if (ut) {
        pid = ut->ut_tid;
    } else {
        pid = LK_THREAD;
    }
    return lockmgr(lkp, flags, lkp->lk_interlock, ut->ut_userp->u_procp);
}

/* Initialize a rwlock on a kthread
 * Setup up Error flags */
int
uthread_rwlock_init(rwl, ut)
	rwlock_t rwl;
	uthread_t ut;
{
	int error = 0;
	rwlock_init(rwl, rwl->rwl_prio, rwl->rwl_wmesg, rwl->rwl_timo, rwl->rwl_flags);
	ut->ut_rwlock = rwl;
	rwl->rwl_utlockholder = ut;
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
		pid = LK_THREAD;
	}
	return rwlockmgr(rwl, flags, pid);
}
