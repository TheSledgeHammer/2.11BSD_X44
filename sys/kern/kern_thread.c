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
 * - Update:
 * 		- flags: additional flags
 * 		- kill:
 * 			- add capability to kill thread\s if needed
 *		- reparent: Needs to change!
 *			- A thread's sleep and run queues should not be transferred from one process to another. 
 *				1) a thread should not be waiting to run/running or sleeping. Whether it is stealable or not.  
 *   				2) If it is stealable, the process has already likely been set to be a zombie, with all it's threads.
 *
 *      		 - New logic: check thread's runqueue and sleepqueue before carrying out the reparenting. 
 *				- Return if the runqueue and sleepqueue are not empty.
 * 		- steal:
 * 			- add a time counter for how long a thread can remain held as stealable before being released.
 * 				- prevent indefinite loop, where a thread is flagged as stealable but not need by a process
 * 				- reduce resource hogging, with threads stuck in this indefinite loop.
 *
 * - Implement:
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
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/user.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/thread.h>
#include <sys/wait.h>

#include <vm/include/vm_param.h>

struct tidhashhead *tidhashtbl;
u_long tidhash;

struct lock_holder 	thread_loholder;

int ppnthreadmax;

int thread_stacklimit(struct thread *);

struct uthread *uthread_alloc(struct thread *);
void uthread_free(struct thread *, struct uthread *);
struct thread *uthread_overseer(struct uthread *);

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
	LIST_INIT(p->p_allthread);

	LIST_INSERT_HEAD(p->p_allthread, td, td_list);
}

void
thread_add(p, td)
	struct proc *p;
	struct thread *td;
{
	THREAD_LOCK(td);
	if ((td->td_procp == p) && (td->td_ptid == p->p_pid)) {
		LIST_INSERT_HEAD(p->p_allthread, td, td_sibling);
	}
	LIST_INSERT_HEAD(TIDHASH(td->td_tid), td, td_hash);
	LIST_INSERT_HEAD(p->p_allthread, td, td_list);
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

/* Deprecated: thread tid's no longer tied to there process */
pid_t
thread_tidmask(p)
	register struct proc *p;
{
	int tid = -1;

	if (p->p_pid != -1) {
		tid = (p->p_pid + p->p_nthreads + TID_MIN);
	}
	if (tid >= TID_MAX) {
		printf("thread_tidmask: tid max reached");
		tid = NO_PID;
	}

	return ((pid_t)tid);
}

struct thread *
tdfind(tid)
	pid_t tid;
{
	register struct thread *td;

	for (td = LIST_FIRST(TIDHASH(tid)); td != 0; td = LIST_NEXT(td, td_hash)) {
		if (td->td_tid == tid) {
			return (td);
		}
	}
	return (NULL);
}

#ifdef deprecated
struct thread *
proc_tdfind(p, tid)
	struct proc *p;
	pid_t tid;
{
	p->p_threado = tdfind(tid);
	if (p->p_threado != NULL) {
		return (p->p_threado);
	}
	return (NULL);
}
#endif

struct thread *
proc_tdfind(p, tid)
	struct proc *p;
	pid_t tid;
{
	register struct thread *td;

	td = tdfind(tid);
	if (td != NULL) {
		if (((td->td_procp == p) && (td->td_ptid == p->p_pid))) {
			return (td);
		}
	}
	return (NULL);
}

/*
 * TODO: Fix runqueue and sleepqueue logic
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
	if (LIST_EMPTY(from->p_allthread) || from == NULL || to == NULL || td == NULL) {
		return;
	}

	if (td->td_stat != (SZOMB | SRUN)) {
		thread_remove(from, td);
		td->td_procp = to;
		td->td_flag = 0;
		td->td_ptid = to->p_pid;
		td->td_pgrp = to->p_pgrp;
		thread_add(to, td);
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

	LIST_FOREACH(from->p_threado, from->p_allthread, td_sibling) {
		if ((from != to) && (from->p_threado == td) && (td->td_ptid == from->p_pid)) {
			if ((from->p_stat == SZOMB) && (to->p_stat != (SZOMB | SRUN))) {
				thread_reparent(from, to, td);
				td->td_flag &= ~TD_STEALABLE;
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
	td->td_tid = 0;
	td->td_ptid = p->p_pid;
	td->td_pgrp = p->p_pgrp;
	td->td_pri = thread_primask(p);
	td->td_ppri = p->p_pri;
	td->td_steal = 0;

	if (!LIST_EMPTY(p->p_allthread)) {
		p->p_threado = LIST_FIRST(p->p_allthread);
	} else {
		p->p_threado = td;
	}
	td->td_uthread = uthread_alloc(td);
	return (td);
}

void
thread_free(p, td)
	struct proc *p;
	struct thread *td;
{
	if (td != NULL) {
		if ((p->p_threado == td) && (td->td_procp == p)) {
			uthread_free(td, td->td_uthread);
			free(td, M_THREAD);
		}
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
	maxsize = (td->td_stacksize - sizeof(struct thread));
	for (i = 0; i < maxthread; i++) {
		size += sizeof(struct thread);
		if ((size >= maxsize) && (size <= td->td_stacksize)) {
			stacklimit = i;
			break;
		}
	}
	return (stacklimit);
}

/* find process from thread */
#ifdef deprecated
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
#endif
struct proc *
thread_pfind(td, pid)
	struct thread *td;
	pid_t pid;
{
    register struct proc *p;

    p = pfind(pid);
    if (p != NULL) {
        if (((p->p_threado == td) && (p->p_pid == td->td_ptid))) {
        	return (p);
        }
    }
    return (NULL);
}

/* thread priority mask */
int
thread_primask(p)
	struct proc *p;
{
	int pri;

	pri = p->p_pri + p->p_nthreads;
	return (pri);
}

/* thread usr priority mask */
int
thread_usrprimask(p)
	struct proc *p;
{
	int pri;

	pri = p->p_usrpri + p->p_nthreads;
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

	pri = thread_primask(p);
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

	for (tq = TAILQ_FIRST(p->p_threadrq); tq != NULL; tq = TAILQ_NEXT(tq, td_link)) {
		if (tq == td) {
			panic("thread_setrq");
		}
	}
#endif
	TAILQ_INSERT_HEAD(p->p_threadrq, td, td_link);
	p->p_tqsready++;
}

/* remove thread from runqueue */
void
thread_remrq(p, td)
	register struct proc *p;
	register struct thread *td;
{
	register struct thread *tq;

	if (td == TAILQ_FIRST(p->p_threadrq)) {
		TAILQ_REMOVE(p->p_threadrq, td, td_link);
		p->p_tqsready--;
	} else {
		for (tq = TAILQ_FIRST(p->p_threadrq); tq != NULL; tq = TAILQ_NEXT(tq, td_link)) {
			if (tq == td) {
				TAILQ_REMOVE(p->p_threadrq, tq, td_link);
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

	if (td == TAILQ_FIRST(p->p_threadrq)) {
		return (td);
	} else {
		for (tq = TAILQ_FIRST(p->p_threadrq); tq != NULL; tq = TAILQ_NEXT(tq, td_link)) {
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
#ifdef DIAGNOSTIC
	register struct thread *tq;

	for (tq = TAILQ_FIRST(p->p_threadsq); tq != NULL; tq = TAILQ_NEXT(tq, td_link)) {
		if (tq == td) {
			panic("thread_setsq");
		}
	}
#endif
	TAILQ_INSERT_TAIL(p->p_threadsq, td, td_link);
	p->p_tqssleep++;
}

/* remove thread from sleepqueue */
void
thread_remsq(p, td)
	struct proc *p;
	struct thread *td;
{
	TAILQ_REMOVE(p->p_threadsq, td, td_link);
	p->p_tqssleep--;
}

/* get thread from sleepqueue */
struct thread *
thread_getsq(p, td)
	register struct proc *p;
	register struct thread *td;
{
	register struct thread *tq;

	if (td == TAILQ_FIRST(p->p_threadsq)) {
		return (td);
	} else {
		for (tq = TAILQ_FIRST(p->p_threadsq); tq != NULL; tq = TAILQ_NEXT(tq, td_link)) {
			if (tq == td ) {
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
	TAILQ_INIT(p->p_threadrq);
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
	TAILQ_INIT(p->p_threadsq);
	p->p_tqssleep = 0;
}

void
thread_setrun(p, td)
	struct proc *p;
	struct thread *td;
{
	struct thread *ntd;
	int error;

	if ((p->p_tqsready == 0) && ((td->td_flag & TD_STEALABLE) == 0)) {
		thread_steal(p, td);
	} else if((p->p_tqsready > 0) && (p->p_stat != SRUN)) {
		error = newthread(&ntd, td->td_name, THREAD_STACK, TRUE);
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
		if (TAILQ_EMPTY(p->p_threadrq)) {
			thread_setrq(p, td);
		}
		break;
	case SZOMB:
		if (!TAILQ_EMPTY(p->p_threadrq)) {
			thread_remrq(p, td);
		}
		break;
	case SSTOP:
	case SSLEEP:
		if (!TAILQ_EMPTY(p->p_threadsq)) {
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

	/* check if thread is flagged as stealable and increment steal counter */
	if ((td->td_flag & TD_STEALABLE) == 0) {
		td->td_steal++;
	}
	/* check if steal counter reaches limit, unflag thread as stealable */
	if (td->td_steal == TD_STEALCOUNTMAX) {
		td->td_flag &= ~TD_STEALABLE;
		td->td_steal = 0;
	}
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
		td->td_flag |= TD_STEALABLE;
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

/*
 * Consider changing to return an int.
 * Would offer better thread control in schedcpu.
 */
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
		td->td_pri = thread_usrprimask(p);
		thread_setpri(p, td);
		thread_setrq(p, td);
	} else {
		td->td_pri = thread_usrprimask(p);
		thread_setpri(p, td);
	}
}

/* schedule multiple threads */
void
thread_schedcpu(p)
	struct proc *p;
{
	register struct thread *td;

	if (!LIST_EMPTY(p->p_allthread)) {
		LIST_FOREACH(td, p->p_allthread, td_list) {
			thread_schedule(p, td);
		}
	}
}

/*
 * create a new thread.
 */
int
newthread1(newpp, newtd, name, stack, forkproc)
	struct proc **newpp;
	struct thread **newtd;
	char *name;
	size_t stack;
	bool_t forkproc;
{
	struct proc *p;
	struct thread *td;
	register_t 	rval[2];
	int error;

	if (forkproc == TRUE) {
		/* First, create the new process. */
		error = proc_create(&p);
		if (__predict_false(error != 0)) {
			panic("newthread: proc_create");
			return (error);
		}
	} else {
		if (newpp == NULL) {
			*newpp = curproc;
		}
		p = *newpp;
	}
	/* allocate and attach a new thread to process */
	td = thread_alloc(p, stack);
	thread_add(p, td);

	if (rval[1]) {
		td->td_flag |= TD_INMEM | TD_SYSTEM;
	}

	/* Name it as specified. */
	bcopy(td->td_name, name, MAXCOMLEN);

	if (newtd != NULL) {
		*newtd = td;
	}
	return (0);
}

/*
 * create a new threads.
 */
int
newthread(newtd, name, stack, forkproc)
	struct thread **newtd;
	char *name;
	size_t stack;
	bool_t forkproc;
{
	return (newthread1(&curproc, newtd, name, stack, forkproc));
}

void
thread_exit(ecode, all)
	int ecode, all;
{
	struct proc *p;
	struct thread *td;

	p = u.u_procp;
	td = uthread_overseer(u.u_uthread);
	//td = u.u_threado;
	td->td_flag &= (TD_PPWAIT | TD_SULOCK);
	td->td_flag |= TD_WEXIT;
	untimeout(realitexpire, (caddr_t)p);

	td->td_stat = SZOMB;
	thread_remove(p, td);
	if ((p->p_stat == SZOMB) && ((td->td_flag & TD_STEALABLE) != 0)) {
		thread_psignal(td->td_procp, SIGTHD, all);
		wakeup((caddr_t)td->td_procp);
		thread_free(p, td);
		return;
	}

	thread_psignal(td->td_procp, SIGTHD, all);
	wakeup((caddr_t)td->td_procp);

	p->p_curthread = NULL;
}

int
thread_ltsleep(chan, pri, wmesg, timo, interlock)
	void *chan;
	int pri;
	char *wmesg;
	u_short	timo;
	__volatile struct lock_object *interlock;
{
	register struct proc *p;
	register struct thread *td;
	int sig, catch;

	p = u.u_procp;
	td = uthread_overseer(u.u_uthread);
	//td = u.u_threado;

	KASSERT(p->p_pri == pri);

	if (panicstr) {
		if (interlock != NULL) {
			simple_unlock(interlock);
		}
		return (0);
	}

	/* thread's on same process */
	if ((td->td_procp == p) && (td->td_ptid == p->p_pid)) {
		catch = pri & PCATCH;
		td->td_wchan = (caddr_t)chan;
		td->td_wmesg = wmesg;
		td->td_pri = thread_primask(p) & PRIMASK;

		thread_setsq(p, td);
		if (timo) {
			timeout((void *)thread_endtsleep, (caddr_t)td, timo);
		}
		if (interlock != NULL) {
			simple_unlock(interlock);
		}
		/*
		 * thread's on the same process do not need to signal a process
		 * interrupt.
		 */
		if (catch) {
			td->td_flag |= TD_SINTR;
			if ((p->p_flag && P_SINTR) != 0) {
				if (td->td_wchan) {
					thread_unsleep(p, td);
				}
				td->td_stat = SRUN;
				goto resume;
			}
			if (td->td_wchan == 0) {
				catch = 0;
				goto resume;
			}
		}
		td->td_stat = SSLEEP;
		//u.u_ru.ru_nvcsw++;
		thread_swtch(p);
		/* switch counter? */
resume:
		td->td_flag &= ~TD_SINTR;
		if (td->td_flag & TD_TIMEOUT) {
			td->td_flag &= ~TD_TIMEOUT;
			if (interlock != NULL) {
				simple_unlock(interlock);
			}
			return (EWOULDBLOCK);
		} else if (timo) {
			untimeout((void *)thread_endtsleep, (caddr_t)td);
		}
	} else {
		/* just return if a different process */
		if (interlock != NULL) {
			simple_unlock(interlock);
		}
		return (EWOULDBLOCK);
	}
	if (interlock != NULL) {
		simple_unlock(interlock);
	}
	return (0);
}

int
thread_tsleep(chan, pri, wmesg, timo)
	void *chan;
	int pri;
	char *wmesg;
	u_short	timo;
{
	return (thread_ltsleep(chan, pri, wmesg, timo, NULL));
}

void
thread_sleep(chan, pri)
	void *chan;
	int pri;
{
	register int priority = pri;

	if (pri > PZERO) {
		priority |= PCATCH;
	}

	u.u_error = thread_tsleep(chan, priority, "thread_sleep", 0);

	if ((priority & PCATCH) == 0 || (u.u_error == 0)) {
		return;
	}
}

void
thread_unsleep(p, td)
	register struct proc *p;
	register struct thread *td;
{
	if ((td->td_procp == p) && (td->td_ptid == p->p_pid)) {
		if (td->td_wchan) {
			thread_remsq(p, td);
			td->td_stat = SRUN;
			td->td_wchan = 0;
		}
	}
}

void
thread_endtsleep(p, td)
	register struct proc *p;
	register struct thread *td;
{
	if (td->td_wchan) {
		if	(p->p_stat == SSLEEP) {
			thread_setrun(p, td);
		} else {
			thread_unsleep(p, td);
		}
		td->td_flag |= TD_TIMEOUT;
	}
}

void
thread_wakeup(p, chan)
	register struct proc *p;
	register const void *chan;
{
	register struct thread *td, *tq;
	struct threadhd *qt;

	qt = p->p_threadsq;
restart:
	if ((td->td_procp == p) && (td->td_ptid == p->p_pid)) {
		for (tq = TAILQ_FIRST(qt); tq != NULL; td = tq) {
			if (td->td_stat != SSLEEP && td->td_stat != SSTOP) {
				panic("thread_wakeup");
			}
			if (td->td_wchan == chan) {
				td->td_wchan = 0;
				tq = TAILQ_NEXT(td, td_link);
				if (td->td_stat == SSLEEP) {
					thread_updatepri(p, td);
					td->td_stat = SRUN;
					thread_setrq(p, td);
					goto restart;
				}
			} else {
				tq = TAILQ_NEXT(td, td_link);
			}
		}
	}
}

/*
 * signal a specified thread on that process.
 */
static void
thread_signal(p, td, sig)
	struct proc *p;
	struct thread *td;
	int sig;
{
	switch (td->td_stat) {
	case SSLEEP:
		if ((td->td_flag & TD_SINTR) == 0) {
			return;
		}
		goto run;
		/*NOTREACHED*/
	case SSTOP:
		if (sig == SIGKILL)
			goto run;
		if (td->td_wchan && (td->td_flag & TD_SINTR)) {
			thread_unsleep(p, td);
		}
		return;
		/*NOTREACHED*/
	default:
		return;
	}
	/*NOTREACHED*/

run:
	thread_updatepri(p, td);
	thread_setrun(p, td);
}

/*
 * signal all threads on that process.
 */
static void
thread_signal_all(p, sig)
	struct proc *p;
	int sig;
{
	register struct thread *td;

	LIST_FOREACH(td, p->p_allthread, td_list) {
		if ((td->td_procp == p) && (td->td_ptid == p->p_pid)) {
			thread_signal(p, td, sig);
		}
	}
}

/*
 * Send the specified signal to
 * all threads on that process, while the process
 * is not interruptible.
 * all: 0 = all threads, 1 = single thread
 */
void
thread_psignal(p, sig, all)
	struct proc *p;
	int sig, all;
{
	if ((p->p_flag & P_SINTR) != 0) {
		switch (p->p_stat) {
		case SSLEEP:
			if (!all) {
				thread_signal(p, sig);
			} else if (all){
				thread_signal_all(p, sig);
			}
			return;

		case SSTOP:
			if (!all) {
				thread_signal(p, sig);
			} else if (all){
				thread_signal_all(p, sig);
			}
			return;
		}
	} else {
		psignal(p, sig);
	}
}

/*
 * Check if thread is idle.
 * Will also check if proc is idle.
 */
void
thread_idle_check(p)
	struct proc *p;
{
	if (u.u_procp != curproc) {
		idle_check();
	} else {
		u.u_procp = curproc;
	}
	p = u.u_procp;
}

void
thread_swtch(p)
	struct proc *p;
{
	register struct thread *tp, *tq;
	register int n;
	struct thread *tdp, *tdq;
	struct threadhd *tqs;
	char pri;

	if (u.u_threado != p->p_curthread) {
		thread_idle_check(p);
	} else {
		u.u_threado = p->p_curthread;
	}

	tqs = p->p_threadrq;

loop:
#ifdef DIAGNOSTIC
	for(tp = TAILQ_FIRST(tqs); tp; tp = TAILQ_NEXT(tp, td_link)) {
		if (tp->td_stat != SRUN) {
			panic("thread_swtch SRUN");
		}
	}
#endif
	tdp = NULL;
	tq = NULL;
	n = 128;

	/*
	 * search for highest-priority runnable thread
	 */
	for (tp = TAILQ_FIRST(tqs); tp; tp = TAILQ_NEXT(tp, td_link)) {
		if ((tp->td_flag & TD_INMEM) && tp->td_pri < n) {
			tdp = tp;
			tdq = tq;
			n = tp->td_pri;
		}
		tq = tp;
	}

	/*
	 * if no thread is runnable, idle.
	 */
	tp = tdp;
	if (tp == NULL) {
		/* */
		goto loop;
	}
	if (tdq) {
		TAILQ_INSERT_AFTER(tqs, tdq, tp, td_link);
	} else {
		TAILQ_INSERT_HEAD(tqs, tp, td_link);
	}
	pri = n;
}

/* allocate uthread */
struct uthread *
uthread_alloc(td)
	struct thread *td;
{
	struct uthread *utd;

	utd = (struct uthread *)malloc(sizeof(struct uthread), M_THREAD, M_NOWAIT);
	utd->utd_thread = td;
	utd->utd_threado = u.u_threado;
	utd->utd_addr = &u;
	return (utd);
}

/* free uthread */
void
uthread_free(td, utd)
	struct thread *td;
	struct uthread *utd;
{
	if (utd != NULL) {
		if ((td->td_uthread == utd) && (utd->utd_thread == td)) {
			free(utd, M_THREAD);
		}
	}
}

/* return uthread overseer */
struct thread *
uthread_overseer(utd)
	struct uthread *utd;
{
	struct proc *p;
	struct thread *td;

	p = u.u_procp;
	if (p != NULL) {
		utd->utd_threado = u.u_threado;
		td = utd->utd_threado;
	}
	if (td != NULL) {
		return (td);
	}
	return (NULL);
}

#ifdef notyet
void thread_yield(struct proc *);
void thread_preempt(struct proc *, struct thread *);

void
thread_yield(p)
	struct proc *p;
{
	struct thread *td;

	td = p->p_curthread;
	td->td_pri = thread_usrprimask(p);
	td->td_stat = SRUN;
	thread_setrq(p, td);
	td->td_procp->p_stats->p_ru.ru_nvcsw++;
	thread_swtch(p);
}

void
thread_preempt(p, td)
	struct proc *p;
	struct thread *td;
{
	td->td_pri = thread_usrprimask(p);
	td->td_stat = SRUN;
	thread_setrq(p, td);
	td->td_procp->p_stats->p_ru.ru_nvcsw++;
	thread_swtch(p);
}
#endif
