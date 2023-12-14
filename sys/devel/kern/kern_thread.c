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

/*
 * TODO:
 * - Name conflicts:
 * 		- vm_glue: thread_block, thread_sleep, thread_wakeup
 * - Implement:
 * `	- thred destruction:
 * 			- when to free a thread from memory?
 * 		- thread_steal function
 * 			- run in: wait, exit, fork?
 * 		- thread scheduling:  (co-operative or pre-emptive?)
 * 			- runqueues/tasks?
 * 			- sleep: flags sleep and wakeup
 * 		- Process related: (what a thread should do when below routine is called?)
 * 			- wait:
 * 			- exit: thread_exit
 * 			- fork:
 * 				- original process retains threads.
 * 				- if creating a new process with threads. use newthread or kthread_create!
 * 			- reparent: runs thread_reparent
 *
 * 			Note: Most of above are dependent on the process containing attached threads.
 * 			As process's are independent of any thread (but not vice-versa). i.e. a process can
 * 			be created and run without any threads, but a thread cannot be run without a process.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mutex.h>

#include <devel/sys/thread.h>

#include <vm/include/vm_param.h>

#define M_THREAD 101

int ppnthreadmax;

struct tidhashhead *tidhashtbl;
u_long tidhash;

static struct thread  thread0;
static struct thread *curthread = &thread0;
struct lock_holder 	thread_loholder;

int thread_stacklimit(struct thread *);

void
thread_init(p, td)
	register struct proc  	*p;
	register struct thread *td;
{
	/* initialize current thread & proc overseer from thread0 */
	p->p_threado = td = &thread0;
	p->p_curthread = curthread = td;

	/* Initialize thread structures. */
	threadinit(p, td);

	/* set up kernel thread */
    LIST_INSERT_HEAD(&p->p_allthread, td, td_list);
    td->td_pgrp = &pgrp0;

	/* give the thread the same creds as the initial thread */
	td->td_ucred = p->p_ucred;
	crhold(td->td_ucred);

	thread_rqinit(p);
	thread_sqinit(p);
}

void
threadinit(p, td)
	struct proc *p;
	struct thread *td;
{
	tdqinit(p, td);
	tidhashtbl = hashinit(maxthread / 4, M_THREAD, &tidhash);
	ppnthreadmax = thread_stacklimit(td);

	/* setup thread mutex */
	mtx_init(td->td_mtx, &thread_loholder, "thread_mutex", (struct thread *)td, td->td_tid);
}

void
tdqinit(p, td)
	register struct proc *p;
	register struct thread *td;
{
	LIST_INIT(&p->p_allthread);

	LIST_INSERT_HEAD(&p->p_allthread, td, td_list);
}

void
thread_add(p, td)
	struct proc *p;
	struct thread *td;
{
	THREAD_LOCK(td);
    if ((td->td_procp == p) && (td->td_ptid == p->p_pid)) {
        LIST_INSERT_HEAD(&p->p_allthread, td, td_sibling);
    }
    LIST_INSERT_HEAD(TIDHASH(td->td_tid), td , td_hash);
    LIST_INSERT_HEAD(&p->p_allthread, td, td_list);
    THREAD_UNLOCK(td);
    p->p_nthreads++;
}

void
thread_remove(p, td)
	struct proc *p;
	struct thread *td;
{
	THREAD_LOCK(td);
	if ((td->td_procp == p) && (td->td_ptid == p->p_pid)) {
		LIST_REMOVE(td, td_sibling);
	}
	LIST_REMOVE(td, td_hash);
	LIST_REMOVE(td, td_list);
	THREAD_UNLOCK(td);
	p->p_nthreads--;
}

pid_t
tidmask(p)
	register struct proc *p;
{
	pid_t tid = p->p_pid + p->p_nthreads + TID_MIN;
	if (tid >= TID_MAX) {
		printf("tidmask: tid max reached");
		tid = NO_TID;
	}
	return (tid);
}

struct thread *
tdfind(p)
	register struct proc *p;
{
	register struct thread *td;
	register pid_t tid;

	tid = tidmask(p);
	LIST_FOREACH(td, TIDHASH(tid), td_hash) {
		if (td->td_tid == tid) {
			return (td);
		}
	}
	return (NULL);
}

/*
 * reparent a thread from one proc to another.
 * from: current thread parent.
 * to: new thread parent.
 * td: the thread in question
 */
void
thread_reparent(from, to, td)
	struct proc *from, *to;
	struct thread *td;
{
	if (LIST_EMPTY(&from->p_allthread) || from == NULL || to == NULL || td == NULL) {
		return;
	}

	if (td->td_stat != (SZOMB | SRUN)) {
		if (!TAILQ_EMPTY(&from->p_threadrq)) {
			thread_remrq(from, td);
		}
		thread_remove(from, td);
		td->td_procp = to;
    	td->td_flag = 0;
    	td->td_tid = tidmask(to);
    	td->td_ptid = to->p_pid;
    	td->td_pgrp = to->p_pgrp;
		thread_add(to, td);
		thread_setrq(to, td);
	}
}

/*
 * Like thread_reparent, except all thread siblings are
 * reparented to the new proc, if the current parent state is a zombie.
 */
void
thread_steal(to, td)
	struct proc *to;
	struct thread *td;
{
	register struct proc *from;

	LIST_FOREACH(from, &from->p_allthread, td_sibling) {
		if ((from != to) && (from->p_threado == td) && (td->td_tid == tidmask(from))) {
			if ((from->p_stat == SZOMB) && (to->p_stat != (SZOMB | SRUN))) {
				thread_reparent(from, to, td);
				td->td_flag &= ~THREAD_STEALABLE;
			}
		}
	}
}

struct thread *
thread_alloc(p, stack)
	struct proc *p;
	size_t stack;
{
    struct thread *td;

    td = (struct thread *)malloc(sizeof(struct thread), M_THREAD, M_NOWAIT);
    td->td_procp = p;
    td->td_stack = (struct thread *)td;
    td->td_stacksize = stack;
    td->td_stat = SIDL;
    td->td_flag = 0;
    td->td_tid = tidmask(p);
    td->td_ptid = p->p_pid;
    td->td_pgrp = p->p_pgrp;
    td->td_pri = primask(p);
    td->td_ppri = p->p_pri;

    if (!LIST_EMPTY(&p->p_allthread)) {
        p->p_threado = LIST_FIRST(&p->p_allthread);
    } else {
        p->p_threado = td;
    }
    return (td);
}

void
thread_free(p, td)
	struct proc *p;
	struct thread *td;
{
	if (td != NULL) {
		free(td, M_THREAD);
	}
}

/* returns maximum number of threads per process within the thread stacksize */
int
thread_stacklimit(td)
	struct thread *td;
{
	size_t size, maxsize;
	int i, stacklimit;

	size = 0;
	maxsize = (td->td_stacksize - sizeof(*td));
	for (i = 0; i < maxthread; i++) {
		size += sizeof(*td);
		if ((size >= maxsize) && (size <= td->td_stacksize)) {
			stacklimit = i;
			break;
		}
	}
	return (stacklimit);
}

/* find process from thread */
struct proc *
thread_pfind(td)
	register struct thread *td;
{
	td->td_procp = pfind(td->td_ptid);
	if (td->td_procp != NULL) {
		return (td->td_procp);
	}
	return (NULL);
}

/* thread priority mask */
int
primask(p)
	struct proc *p;
{
	int pri;

	pri = p->p_pri + p->p_nthreads;
	return (pri);
}

/* update thread priority from proc priority */
void
thread_updatepri(p, td)
	struct proc *p;
	struct thread *td;
{
    int pri, retry;

    retry = 0;
    if ((td->td_ppri != p->p_pri)) {
check:
        if (td->td_pri == (td->td_ppri + p->p_nthreads)) {
        	pri = thread_setpri(p, td);
        	td->td_pri = pri;
        	return;
        } else if (td->td_pri != (td->td_ppri + p->p_nthreads)){
            printf("thread_updatepri: priority mismatch!\n");
            printf("thread_updatepri: updating priority...\n");
            pri = thread_setpri(p, td);
            td->td_pri = pri;
            return;
        }
        if (retry != 0) {
            printf("thread_updatepri: no change in priority\n");
        }
    } else {
        retry = 1;
        goto check;
    }
}

int
thread_setpri(p, td)
	struct proc *p;
	struct thread *td;
{
	int pri;

	pri = primask(p);
	td->td_pri = pri;
	td->td_ppri = p->p_pri;
	return (pri);
}

/* add thread to runqueue */
void
thread_setrq(p, td)
	register struct proc *p;
	register struct thread *td;
{
#ifdef DIAGNOSTIC
	register struct thread *tq;

	for (tq = TAILQ_FIRST(&p->p_threadrq); tq != NULL; tq = TAILQ_NEXT(tq, p_link)) {
		if (tq == td) {
			panic("thread_setrq");
		}
	}
#endif
	TAILQ_INSERT_HEAD(&p->p_threadrq, td, td_link);
	p->p_tqsready++;
}

/* remove thread from runqueue */
void
thread_remrq(p, td)
	register struct proc *p;
	register struct thread *td;
{
	register struct thread *tq;

	if (td == TAILQ_FIRST(&p->p_threadrq)) {
		TAILQ_REMOVE(&p->p_threadrq, td, td_link);
		p->p_tqsready--;
	} else {
		for (tq = TAILQ_FIRST(&p->p_threadrq); tq != NULL; tq = TAILQ_NEXT(tq, td_link)) {
			if (tq == td) {
				TAILQ_REMOVE(&p->p_threadrq, tq, td_link);
				p->p_tqsready--;
				return;
			}
			panic("thread_remrq");
		}
	}
}

/* get thread from runqueue */
struct thread *
thread_getrq(p, td)
	register struct proc *p;
	register struct thread *td;
{
	register struct thread *tq;

	if (td == TAILQ_FIRST(&p->p_threadrq)) {
		return (td);
	} else {
		for (tq = TAILQ_FIRST(&p->p_threadrq); tq != NULL; tq = TAILQ_NEXT(tq, td_link)) {
			if (tq == td) {
				return (tq);
			} else {
				goto done;
			}
		}
	}

done:
	panic("thread_getrq");
	return (NULL);
}

/* add thread to sleepqueue */
void
thread_setsq(p, td)
	struct proc *p;
	struct thread *td;
{
	if ((td->td_procp == p) && (td->td_ptid == p->p_pid)) {
		TAILQ_INSERT_TAIL(&p->p_threadsq, td, td_link);
		p->p_tqssleep++;
	}
}

/* remove thread from sleepqueue */
void
thread_remsq(p, td)
	struct proc *p;
	struct thread *td;
{
	if ((td->td_procp == p) && (td->td_ptid == p->p_pid)) {
		TAILQ_REMOVE(&p->p_threadsq, td, td_link);
		p->p_tqssleep--;
	}
}

/* get thread from sleepqueue */
struct thread *
thread_getsq(p, td)
	register struct proc *p;
	register struct thread *td;
{
	register struct thread *tq;

	if (td == TAILQ_FIRST(&p->p_threadsq)) {
		return (td);
	} else {
		for (tq = TAILQ_FIRST(&p->p_threadsq); tq != NULL; tq = TAILQ_NEXT(tq, td_link)) {
			if (tq == td) {
				return (tq);
			} else {
				goto done;
			}
		}
	}

done:
	panic("thread_getsq");
	return (NULL);
}

/*
 * Initialize the (doubly-linked) run queues
 * to be empty.
 */
void
thread_rqinit(p)
	struct proc *p;
{
	TAILQ_INIT(&p->p_threadrq);
	p->p_tqsready = 0;
}

/*
 * Initialize the (doubly-linked) sleep queues
 * to be empty.
 */
void
thread_sqinit(p)
	struct proc *p;
{
	TAILQ_INIT(&p->p_threadsq);
	p->p_tqssleep = 0;
}

void
thread_setrun(p, td)
	struct proc *p;
	struct thread *td;
{
	struct thread *ntd;
	int error;

	if ((p->p_tqsready == 0) && ((td->td_flag & THREAD_STEALABLE) == 0)) {
		thread_steal(p, td);
	} else if((p->p_tqsready > 0) && (p->p_stat != SRUN)) {
		error = newthread(&ntd, NULL, THREAD_STACK, TRUE);
		if (error != 0) {
			panic("thread_setrun: newthread");
			return;
		}
	}
	switch (td->td_stat) {
	case 0:
	case SWAIT:
	case SRUN:
	case SIDL:
		if (TAILQ_EMPTY(&p->p_threadrq)) {
			thread_setrq(p, td);
		}
		break;
	case SZOMB:
		if (!TAILQ_EMPTY(&p->p_threadrq)) {
			thread_remrq(p, td);
		}
		break;
	case SSTOP:
	case SSLEEP:
		if (!TAILQ_EMPTY(&p->p_threadsq)) {
			thread_unsleep(p, td);
		}
		break;
	default:
		panic("thread_setrun");
		break;
	}
	thread_updatepri(p, td);
	td->td_stat = SRUN;
	thread_setrq(p, td);
	td->td_pri = thread_setpri(p, td);
}

void
thread_run(p, td)
	struct proc *p;
	struct thread *td;
{
	register int s;

	s = splhigh();
	switch (p->p_stat) {
	case 0:
	case SWAIT:
	case SRUN:
		thread_setrun(p, td);
		break;
	case SIDL:
		thread_setrun(p, td);
		break;
	case SZOMB:
		td->td_flag |= THREAD_STEALABLE;
		thread_setrun(p, td);
		break;
	case SSTOP:
	case SSLEEP:
		thread_setrun(p, td);
		break;
	default:
		panic("thread_run");
		break;
	}
	splx(s);
}

/* schedule a single thread */
void
thread_schedule(p, td)
	struct proc *p;
	struct thread *td;
{
	thread_run(p, td);
	if ((td != p->p_curthread) &&
				(td->td_stat == SRUN) &&
				(td->td_flag & TD_INMEM) &&
				(td->td_pri != (p->p_usrpri + p->p_nthreads))) {
		thread_remrq(p, td);
		td->td_pri = p->p_usrpri + p->p_nthreads;
		thread_setpri(p, td);
		thread_setrq(p, td);
	} else {
		td->td_pri = p->p_usrpri + p->p_nthreads;
		thread_setpri(p, td);
	}
}

/* schedule multiple threads */
void
thread_schedcpu(p)
	struct proc *p;
{
	register struct thread *td;

	THREAD_LOCK(td);
	LIST_FOREACH(td, &p->p_allthread, td_list) {
		thread_schedule(p, td);
	}
	THREAD_UNLOCK(td);
}

void
thread_exit(ecode)
	int ecode;
{
	struct proc *p;
	struct thread *td;

	p = u.u_procp;
	td = u.u_threado;

	if (ecode != 0) {
		printf("WARNING: thread `%s' (%d) exits with status %d\n", td->td_name, td->td_tid, ecode);
	}

	td->td_stat = SZOMB;
	thread_remove(p, td);
	if ((p->p_stat == SZOMB) && ((td->td_flag & THREAD_STEALABLE) != 0)) {
		thread_free(p, td);
	}
	td = NULL;
	p->p_curthread = td;
	exit(W_EXITCODE(ecode, 0));
	for (;;);
}

void
thread_sleep(chan, pri)
	void *chan;
	int pri;
{
	register struct proc *p;
	register struct thread *td;

	p = u.u_procp;
	td = u.u_threado;

	td->td_wchan = chan;
	td->td_pri = primask(p) & PRIMASK;
	td->td_stat = SSLEEP;
	thread_setsq(p, td);
}

void
thread_unsleep(p, td)
	struct proc *p;
	struct thread *td;
{
	if (td->td_wchan) {
		thread_remsq(p, td);
		td->td_stat = SRUN;
		td->td_wchan = 0;
	}
}
