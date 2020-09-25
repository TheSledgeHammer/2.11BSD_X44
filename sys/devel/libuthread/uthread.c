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

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include "devel/sys/rwlock.h"
#include "devel/sys/kthread.h"
#include "devel/sys/uthread.h"

extern struct uthread uthread0;
struct uthread *curuthread = &uthread0;

void
uthread_init(kt, ut)
	register struct kthread *kt;
	register struct uthread *ut;
{
	register_t rval[2];
	int error;

	/* initialize current uthread(0) */
    ut = &uthread0;
    curuthread = ut;

    /* Initialize uthread structures. */
    utqinit();

    /* set up uthreads */
    alluthread = (struct uthread *)ut;
    ut->ut_prev = (struct uthread **)&alluthread;

    /* Set thread to idle & waiting */
    ut->ut_stat |= UT_SIDL | UT_SWAIT | UT_SREADY;

    /* setup uthread locks */
    uthread_lock_init(uthread_lkp, ut);
    uthread_rwlock_init(uthread_rwl, ut);

	/* uthread overseer */
	if(newthread(kt->kt_procp, 0))
		panic("uthread overseer creation");
	if(rval[1])
		ut = kt->kt_uthreado;
		ut->ut_flag |= KT_INMEM | KT_SYSTEM;

		/* initialize uthreadpools  */
		uthreadpools_init();
}

int
uthread_create(uthread_t ut)
{
	return (0);
}

int
uthread_join(uthread_t ut)
{
	return (0);
}

int
uthread_cancel(uthread_t ut)
{
	return (0);
}

int
uthread_exit(uthread_t ut)
{
	return (0);
}

int
uthread_detach(uthread_t ut)
{
	return (0);
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
	return (0);
}

/*
 * init the uthread queues
 */
void
utqinit()
{
	register struct uthread *ut;

	freeuthread = NULL;
	for (ut = uthreadNUTHREAD; --ut > uthread0; freeuthread = ut)
		ut->ut_nxt = freeuthread;

	alluthread = ut;
	ut->ut_nxt = NULL;
	ut->ut_prev = &alluthread;

	zombuthread = NULL;
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
uthread_lock_init(lkp, ut)
    struct lock *lkp;
    uthread_t ut;
{
    int error = 0;
    lockinit(lkp, lkp->lk_prio, lkp->lk_wmesg, lkp->lk_timo, lkp->lk_flags);
    set_uthread_lock(lkp, ut);
    if(lkp->lk_utlockholder == NULL) {
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
	set_uthread_rwlock(rwl, ut);
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
leavetgrp(ut)
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
