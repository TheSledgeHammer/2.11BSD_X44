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
 * Kthread renamed:
 * Are now known as threads.
 * All thread information in this .c file corresponds to thread.h
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mutex.h>

#include <devel/sys/thread.h>
#include <devel/sys/threadpool.h>

#include <vm/include/vm_param.h>

#define M_THREAD 101

/*
 * TODO:
 * - Setup thread pgrp's and tid's to work.
 */

void
thread_add(p, td)
	struct proc *p;
	struct thread *td;
{
    if (td->td_procp == p) {
        LIST_INSERT_HEAD(&p->p_allthread, td, td_sibling);
    }
    LIST_INSERT_HEAD(&p->p_allthread, td, td_list);
    p->p_nthreads++;
}

void
thread_remove(p, td)
	struct proc *p;
	struct thread *td;
{
	if (td->td_procp == p) {
		LIST_REMOVE(td, td_sibling);
	}
	LIST_REMOVE(td, td_list);
	p->p_nthreads--;
}

struct thread *
thread_find(p)
	struct proc *p;
{
    struct thread *td;

    LIST_FOREACH(td, &p->p_allthread, td_list) {
        if ((td->td_procp == p) && (td != NULL)) {
            return (t);
        }
    }
    return (NULL);
}

struct thread *
thread_alloc(p, stack)
	struct proc *p;
	size_t stack;
{
    struct thread *td;

    td = (struct thread *)malloc(sizeof(struct thread), M_THREAD, M_NOWAIT);
    td->td_procp = p;
    td->td_stack = stack;
    td->td_stat = SIDL;
    td->td_flag = 0;
    if (!LIST_EMPTY(&p->p_allthread)) {
        p->p_kthreado = LIST_FIRST(&p->p_allthread);
    } else {
        p->p_kthreado = td;
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

int
proc_create(newpp)
	struct proc **newpp;
{
	struct proc *p;
	register_t 	rval[2];
	int 		error;

	/* First, create the new process. */
	error = newproc(0);
	if (__predict_false(error != 0)) {
		return (error);
	}

	if (rval[1]) {
		p->p_flag |= P_INMEM | P_SYSTEM | P_NOCLDWAIT;
	}

	if (newpp != NULL) {
		*newpp = p;
	}

	return (0);
}

int
newthread(newtd, name, stack)
	struct thread **newtd;
	char *name;
	size_t stack;
{
	struct proc *p;
	struct thread *td;
	register_t 	rval[2];
	int error;

	/* First, create the new process. */
	error = proc_create(&p);
	if (__predict_false(error != 0)) {
		panic("thread_create");
		return (error);
	}

	/* allocate and attach a new thread to process */
	td = thread_alloc(p, stack);

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

int
thread_create(func, arg, newtd, name)
	void (*func)(void *);
	void *arg;
	struct thread **newtd;
	char *name;
{
	return (newthread(newtd, name, THREAD_STACK));
}
