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
#include "devel/sys/threadpool.h"

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
	kt->kt_uthreado = &uthread0;
    ut = kt->kt_uthreado;
    curuthread = ut;

    /* set up uthreads */
    LIST_INSERT_HEAD(&alluthread, ut, ut_list);
    ut->ut_pgrp = &pgrp0;
    LIST_INSERT_HEAD(TGRPHASH(0), &pgrp0, pg_hash);
    kt->kt_uthreado = ut;

	/* give the uthread the same creds as the initial kthread */
	ut->ut_ucred = kt->kt_ucred;
	crhold(ut->ut_ucred);

    /* setup uthread locks */
	mtx_init(ut->ut_mtx, &uthread_loholder, "uthread mutex", (struct uthread *)ut, ut->ut_tid, ut->ut_pgrp);

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
	int 		error;

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
uthreadpool_itpc_send(itpc, utpool, pid, cmd)
	struct threadpool_itpc *itpc;
    struct uthreadpool *utpool;
    pid_t pid;
    int cmd;
{
    /* sync itpc to threadpool */
	itpc->itpc_utpool = utpool;
	itpc->itpc_jobs = utpool->utp_jobs;
	utpool->utp_issender = TRUE;
	utpool->utp_isreciever = FALSE;

	/* command / action */
	switch(cmd) {
	case ITPC_SCHEDULE:
		uthreadpool_schedule_job(utpool, itpc->itpc_jobs);
		break;

	case ITPC_CANCEL:
		uthreadpool_cancel_job(utpool, itpc->itpc_jobs);
		break;

	case ITPC_DESTROY:
		uthreadpool_job_destroy(utpool->utp_jobs);
		break;

	case ITPC_DONE:
		uthreadpool_job_done(utpool->utp_jobs);
		break;
	}

	/* update job pool */
	itpc_check_uthreadpool(itpc, pid);
}

void
uthreadpool_itpc_recieve(itpc, utpool, pid, cmd)
	struct threadpool_itpc *itpc;
	struct uthreadpool 		*utpool;
	pid_t pid;
	int cmd;
{
	/* sync itpc to threadpool */
	itpc->itpc_utpool = utpool;
	itpc->itpc_jobs = utpool->utp_jobs;
	utpool->utp_issender = FALSE;
	utpool->utp_isreciever = TRUE;

	/* command / action */
	switch(cmd) {
	case ITPC_SCHEDULE:
		uthreadpool_schedule_job(utpool, itpc->itpc_jobs);
		break;
	case ITPC_CANCEL:
		uthreadpool_cancel_job(utpool, itpc->itpc_jobs);
		break;
	case ITPC_DESTROY:
		uthreadpool_job_destroy(itpc->itpc_jobs);
		utpool->utp_jobs = itpc->itpc_jobs;
		break;
	case ITPC_DONE:
		uthreadpool_job_done(itpc->itpc_jobs);
		break;
	}

	/* update job pool */
	itpc_verify_uthreadpool(itpc, pid);
}

/*
 * Locate a uthread by number
 */
struct uthread *
utfind(tid)
	register pid_t tid;
{
	register struct uthread *ut;
	for (ut = LIST_FIRST(TIDHASH(tid)); ut != 0; ut = LIST_NEXT(ut, ut_hash)) {
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
	LIST_REMOVE(ut, ut_pglist);
	if (LIST_FIRST(&ut->ut_pgrp->pg_mem) == 0) {
		pgdelete(ut->ut_pgrp);
	}
	ut->ut_pgrp = 0;
	return (0);
}
