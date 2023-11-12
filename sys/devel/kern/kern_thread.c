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

/* Common KThread & UThread functions */

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

extern struct kthread 		 kthread0;
struct kthread *curkthread = &kthread0;

#define NTHREAD (maxproc * 2)
int maxthread = NTHREAD;

struct tidhashhead *tidhashtbl;
u_long tidhash;
struct tgrphashhead *tgrphashtbl;
u_long tgrphash;

struct kthreadlist 	allkthread;
struct kthreadlist 	zombkthread;
struct kthreadlist 	freekthread;

struct lock_holder 	kthread_loholder;

int				kthread_alloc(void (*)(void *), void *, struct kthread *, char *);
void			kthread_add(struct kthreadlist *, struct kthread *, int);
void			kthread_remove(struct kthread *, int);
struct kthread *kthread_find(struct kthreadlist *, int);
void			kthread_dispatch(struct kthread *, int, char);

int
proc_create(func, arg, p, name)
	void (*func)(void *);
	void *arg;
	struct proc **p;
	char *name;
{
	return (kthread_create(func, arg, p, name));
}

void
kthread_init(p, kt)
	register struct proc  	*p;
	register struct kthread *kt;
{
	register_t 	rval[2];
	int 		error;

	/* initialize current kthread & proc overseer from kthread0 */
	p->p_kthreado = &kthread0;
	kt = p->p_kthreado;
    curkthread = kt;

	/* Initialize kthread and kthread group structures. */
    kthreadinit(kt);

	/* set up kernel thread */
    LIST_INSERT_HEAD(&allkthread, kt, kt_list);
    kt->kt_pgrp = &pgrp0;
    LIST_INSERT_HEAD(TGRPHASH(0), &pgrp0, pg_hash);
	p->p_kthreado = kt;

	/* give the kthread the same creds as the initial thread */
	kt->kt_ucred = p->p_ucred;
	crhold(kt->kt_ucred);
}

void
kthreadinit(kt)
	struct kthread *kt;
{
	ktqinit(kt);
	tidhashtbl = hashinit(maxthread / 4, M_PROC, &tidhash);
	tgrphashtbl = hashinit(maxthread / 4, M_PROC, &tgrphash);

	/* setup kthread multiplexor */
	kt->kt_mpx = mpx_allocate(maxthread);

    /* setup kthread mutex */
    mtx_init(kt->kt_mtx, &kthread_loholder, "kthread mutex", (struct kthread *)kt, kt->kt_tid);
}

/*
 * init the kthread queues
 */
void
ktqinit(kt)
	register struct kthread *kt;
{
	LIST_INIT(&allkthread);
	LIST_INIT(&zombkthread);
	LIST_INIT(&freekthread);

	/*
	 * most procs are initially on freequeue
	 *	nb: we place them there in their "natural" order.
	 */
	LIST_INSERT_HEAD(&freekthread, kt, kt_list);
	LIST_INSERT_HEAD(&allkthread, kt, kt_list);
	LIST_FIRST(&zombkthread) = NULL;
}

/*
 * remove kthread from thread group
 */
int
leavektgrp(kt)
	struct kthread *kt;
{
	LIST_REMOVE(kt, kt_pglist);
	if (LIST_FIRST(&kt->kt_pgrp->pg_mem) == 0) {
		pgdelete(kt->kt_pgrp);
	}
	kt->kt_pgrp = 0;
	return (0);
}

/*
 * Locate a thread group by number
 */
struct pgrp *
tgfind(pgid)
	register pid_t pgid;
{
	register struct pgrp *tgrp;
	LIST_FOREACH(tgrp, TGRPHASH(pgid), pg_hash) {
		if (tgrp->pg_id == pgid) {
			return (tgrp);
		}
	}
	return (NULL);
}

/*
 * Locate a kthread by number & channel
 */
struct kthread *
ktfind(tid, chan)
	register pid_t tid;
	register int chan;
{
	register struct kthread *kt;

	kt = kthread_find(TIDHASH(tid), chan);
	if (kt->kt_tid == tid) {
		return (kt);
	}
	return (NULL);
}

int
kthread_alloc(func, arg, kt, name)
	void (*func)(void *);
	void *arg;
	struct kthread *kt;
	char *name;
{
	struct proc *p;
	int error;

	error = proc_create(func, arg, &p, name);
	if (error != 0) {
		return (error);
	}
	kt = p->p_kthreado;
	if (kt != NULL) {
		kt->kt_flag |= KT_INMEM | KT_SYSTEM | KT_NOCLDWAIT;
	}
	return (0);
}

/*
 * Kthread dispatch: changes thread from one queue to another queue depending on state.
 */
void
kthread_dispatch(kt, chan, state)
	struct kthread *kt;
	int chan;
	char state;
{
	struct kthread *nkt;

	switch (state) {
	case KT_ONFREE:
		/* freekthread list */
		if (LIST_EMPTY(&freekthread)) {
			break;
		}
		if (kt == NULL) {
			nkt = kthread_find(&freekthread, chan);
			if ((nkt != NULL) && (nkt->kt_stat == state)) {
				kt = nkt;
			} else {
				break;
			}
		}
		kthread_remove(kt, chan);						/* off freekthread */
		kthread_add(&allkthread, kt, chan);				/* onto allkthread */
		kt->kt_stat = KT_ONALL;
		break;

	case KT_ONALL:
		/* allkthread list */
		if (LIST_EMPTY(&allkthread)) {
			break;
		}
		if (kt == NULL) {
			nkt = kthread_find(&allkthread, chan);
			if ((nkt != NULL) && (nkt->kt_stat == state)) {
				kt = nkt;
			} else {
				break;
			}
		}
		kthread_remove(kt, chan);						/* off allkthread */
		kthread_add(&zombkthread, kt, chan);			/* onto zombkthread */
		kt->kt_stat = KT_ONZOMB;
		break;

	case KT_ONZOMB:
		/* zombkthread list */
		if (LIST_EMPTY(&zombkthread)) {
			break;
		}
		if (kt == NULL) {
			nkt = kthread_find(&zombkthread, chan);
			if ((nkt != NULL) && (nkt->kt_stat == state)) {
				kt = nkt;
			} else {
				break;
			}
		}
		kthread_remove(kt, chan);						/* off zombkthread */
		kthread_add(&freekthread, kt, chan);			/* onto freekthread */
		kt->kt_stat = KT_ONFREE;
		break;
	}
}

void
kthread_add(ktlist, kt, chan)
	struct kthreadlist *ktlist;
	struct kthread *kt;
	int chan;
{
	struct mpx *mpx;
	int ret;

	LIST_INSERT_HEAD(ktlist, kt, kt_list);
	mpx = kt->kt_mpx;
	if (mpx == NULL) {
		return;
	}
	ret = mpx_put(mpx, chan, kt);
	if (ret != 0) {
		return;
	}
}

void
kthread_remove(kt, chan)
	struct kthread *kt;
	int chan;
{
	struct mpx *mpx;
	int ret;

	mpx = kt->kt_mpx;
	if (mpx == NULL) {
		return;
	}

	ret = mpx_remove(mpx, chan, kt);
	if (ret != 0) {
		return;
	}
	LIST_REMOVE(kt, kt_list);
}

struct kthread *
kthread_find(ktlist, chan)
	struct kthreadlist *ktlist;
	int chan;
{
	struct kthread *kt;
	struct mpx *mpx;

	LIST_FOREACH(kt, ktlist, kt_list) {
		if (kt != NULL) {
			mpx = kt->kt_mpx;
			if (mpx) {
				ret = mpx_get(mpx, chan, kt);
				if (ret) {
					return (kt);
				}
			}
		}
	}
	return (NULL);
}

/* kernel thread runtime */
struct kthread_runtime {
	struct kthreadlist 	ktr_zombkthread;
	struct kthreadlist 	ktr_freekthread;
	struct kthreadlist 	ktr_allkthread;
	int					ktr_flags;

	struct proc			*ktr_proc;
	struct kthread		ktr_kthread;
	//send
	//receive
};
struct kthread_runtime kt_runtime;

void
runtime_init(struct kthread *kt)
{
	LIST_INIT(&kt_runtime.ktr_zombkthread);
	LIST_INIT(&kt_runtime.ktr_freekthread);
	LIST_INIT(&kt_runtime.ktr_allkthread);

	kt->kt_runtime = &kt_runtime;

	int i;

	for (i = 0; i < maxthread; i++) {
		kthread_dispatch(kt, i, kt->kt_stat);
	}
}



