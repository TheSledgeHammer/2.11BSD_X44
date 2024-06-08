/*	$NetBSD: kern_kthread.c,v 1.3 1998/12/22 21:21:36 kleink Exp $	*/
/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/wait.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <machine/stdarg.h>

int	kthread_create_now;

#ifdef notyet
/*
 * Fork a kernel thread.  Any process can request this to be done.
 * The VM space and limits, etc. will be shared with proc0.
 */
/* creates a new thread */
int
kthread_create(func, arg, newtd, name, forkproc)
	void (*func)(void *);
	void *arg;
	struct thread **newtd;
	char *name;
	bool_t forkproc;
{
	error = newthread(newtd, name, THREAD_STACK, forkproc);
	if (__predict_false(error != 0)) {
		return (error);
	}
	return (0);
}

/*
 * Cause a kernel thread to exit.  Assumes the exiting thread is the
 * current context.
 */
void
kthread_exit(ecode)
	int ecode;
{
	struct thread *curthread;

	curthread = curproc->p_curthread;

	if (ecode != 0) {
		printf("WARNING: thread `%s' (%d) exits with status %d\n", curthread->td_name, curthread->td_tid, ecode);
	}
	thread_exit(W_EXITCODE(ecode, 0));

	for (;;);
}
#endif
/*
 * Fork a kernel thread.  Any process can request this to be done.
 * The VM space and limits, etc. will be shared with proc0.
 */
int
kthread_create(func, arg, newpp, name)
	void (*func)(void *);
	void *arg;
	struct proc **newpp;
	char *name;
{
	struct proc *p;
	register_t 	rval[2];
	int 		error;

	/* First, create the new process. */
	error = newproc(0);
	if (__predict_false(error != 0)) {
		panic("kthread_create");
		return (error);
	}

	if (rval[1]) {
		p->p_flag |= P_INMEM | P_SYSTEM | P_NOCLDWAIT;
	}

	/* Name it as specified. */
	bcopy(p->p_comm, name, MAXCOMLEN);

	if (newpp != NULL) {
		*newpp = p;
	}
	return (0);
}

/*
 * Cause a kernel thread to exit.  Assumes the exiting thread is the
 * current context.
 */
void
kthread_exit(ecode)
	int ecode;
{
	/*
	 * XXX What do we do with the exit code?  Should we even bother
	 * XXX with it?  The parent (proc0) isn't going to do much with
	 * XXX it.
	 */
	if (ecode != 0) {
		printf("WARNING: thread `%s' (%d) exits with status %d\n", curproc->p_comm, curproc->p_pid, ecode);
	}
	exit(W_EXITCODE(ecode, 0));

	for (;;);
}

struct kthread_queue {
	SIMPLEQ_ENTRY(kthread_queue) 	kq_q;
	void 							(*kq_func)(void *);
	void 							*kq_arg;
};

SIMPLEQ_HEAD(, kthread_queue) kthread_queue = SIMPLEQ_HEAD_INITIALIZER(kthread_queue);

void
kthread_create_deferred(func, arg)
	void (*func)(void *);
	void *arg;
{
	struct kthread_queue *kq;
	if (kthread_create_now) {
		(*func)(arg);
		return;
	}

	kq = malloc(sizeof(*kq), M_TEMP, M_NOWAIT | M_ZERO);
	if (kq == NULL) {
		panic("unable to allocate kthread_queue");
	}

	kq->kq_func = func;
	kq->kq_arg = arg;

	SIMPLEQ_INSERT_TAIL(&kthread_queue, kq, kq_q);
}

void
kthread_run_deferred_queue(void)
{
	struct kthread_queue *kq;

	/* No longer need to defer kthread creation. */
	kthread_create_now = 1;

	while ((kq = SIMPLEQ_FIRST(&kthread_queue)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&kthread_queue, kq_q);
		(*kq->kq_func)(kq->kq_arg);
		free(kq, M_TEMP);
	}
}
