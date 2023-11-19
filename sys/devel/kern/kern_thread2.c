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
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mutex.h>

#include <devel/sys/kthread.h>
#include <devel/sys/threadpool.h>
#include <devel/libuthread/uthread.h>

#include <vm/include/vm_param.h>

/*
 * TODO:
 * - Setup thread pgrp's and tid's to work.
 */

void
thread_add(p, kt)
	struct proc *p;
	struct kthread *kt;
{
    if (kt->kt_procp == p) {
        LIST_INSERT_HEAD(&p->p_allthread, kt, kt_sibling);
    }
    //LIST_INSERT_HEAD(TIDHASH(kt->kt_tid), kt, kt_hash);
    /* add to proc/thread grp */
    LIST_INSERT_HEAD(&p->p_allthread, kt, kt_list);
    p->p_nthreads++;
}

void
thread_remove(p, kt)
	struct proc *p;
	struct kthread *kt;
{
	if (kt->kt_procp == p) {
		LIST_REMOVE(kt, kt_sibling);
	}
	//LIST_REMOVE(t, kt_hash);
	/* remove from proc/thread grp */
	LIST_REMOVE(kt, kt_list);
	p->p_nthreads--;
}

struct kthread *
thread_find(p)
	struct proc *p;
{
    struct kthread *kt;

    LIST_FOREACH(kt, &p->p_allthread, kt_list) {
        if ((kt->kt_procp == p) && (kt != NULL)) {
            return (t);
        }
    }
    return (NULL);
}

struct kthread *
thread_alloc(p, stack)
	struct proc *p;
	size_t stack;
{
    struct kthread *kt;

    kt = (struct kthread *)malloc(sizeof(struct kthread));
    kt->kt_procp = p;
    kt->kt_stack = stack;
    kt->kt_stat = SIDL;

    if (!LIST_EMPTY(&p->p_allthread)) {
        p->p_kthreado = LIST_FIRST(&p->p_allthread);
    } else {
        p->p_kthreado = kt;
    }
    thread_add(p, kt);
    return (kt);
}

void
thread_free(struct proc *p, struct kthread *kt)
{
	if (kt != NULL) {
		p->p_kthreado = NULL;
		free(t);
	}
}

int
proc_create(newpp, name)
	struct proc **newpp;
	char *name;
{
	struct proc *p;
	register_t 	rval[2];
	int 		error;

	/* First, create the new process. */
	error = newproc(0);
	if (__predict_false(error != 0)) {
		panic("proc_create");
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

int
thread_create(func, arg, newkt, name, stack)
	void (*func)(void *);
	void *arg;
	struct kthread **newkt;
	char *name;
	size_t stack;
{
	struct kthread *kt;
	struct proc *p;
	register_t 	rval[2];
	int error;

	error = proc_create(&p, name);
	if (__predict_false(error != 0)) {
		return (error);
	}

	kt = thread_alloc(p, stack);
	if (kt == NULL) {
		panic("thread_create");
		return (1);
	}

	if (rval[1]) {
		kt->kt_flag |= KT_INMEM | KT_SYSTEM;
	}

	/* Name it as specified. */
	bcopy(kt->kt_name, name, MAXCOMLEN);

	if (newkt != NULL) {
		*newkt = kt;
	}
	return (0);
}
