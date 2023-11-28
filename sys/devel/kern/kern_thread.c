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
 * - Implement:
 * 		- thread_steal function
 * 		- thread scheduling:
 * 			- exit
 * 			- wait: blocking thread siblings if a thread is already running
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

extern struct thread  thread0;
struct thread *curthread = &thread0;
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
    if ((td->td_procp == p) && (td->td_ptid == p->p_pid)) {
        LIST_INSERT_HEAD(&p->p_allthread, td, td_sibling);
    }
    LIST_INSERT_HEAD(TIDHASH(td->td_tid), td , td_hash);
    LIST_INSERT_HEAD(&p->p_allthread, td, td_list);
    p->p_nthreads++;
}

void
thread_remove(p, td)
	struct proc *p;
	struct thread *td;
{
	if ((td->td_procp == p) && (td->td_ptid == p->p_pid)) {
		LIST_REMOVE(td, td_sibling);
	}
	LIST_REMOVE(td, td_hash);
	LIST_REMOVE(td, td_list);
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

	if (to->p_stat == SZOMB) {
		return;
	}

	if (td->td_stat != SZOMB) {
		thread_remove(from, td);
		td->td_procp = to;
    	td->td_flag = 0;
    	td->td_tid = tidmask(to);
    	td->td_ptid = to->p_pid;
    	td->td_pgrp = to->p_pgrp;
		thread_add(to, td);
	} else {
		thread_remove(from, td);
	}
}

/*
 * Like thread_reparent, except all thread siblings are
 * reparented to the new proc, if the existing parent state is a zombie.
 */
void
thread_steal(to, td)
	struct proc *to;
	struct thread *td;
{
	register struct proc *from;

	LIST_FOREACH(from, &from->p_allthread, td_sibling) {
		if ((from->p_threado == td) && (td->td_tid == tidmask(from))) {
			switch (from->p_stat) {
			case SRUN:
			case SIDL:
			case SZOMB:
				if (to->p_stat != SZOMB) {
					thread_reparent(from, to, td);
				}
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

    if (!LIST_EMPTY(&p->p_allthread)) {
        p->p_threado = LIST_FIRST(&p->p_allthread);
    } else {
        p->p_threado = td;
    }
    thread_add(p, td);
    return (td);
}

void
thread_free(struct proc *p, struct thread *td)
{
	if (td != NULL) {
		thread_remove(p, td);
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
