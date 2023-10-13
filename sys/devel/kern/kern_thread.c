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
void			kthread_free(struct kthread *, int);
struct kthread *kthread_find(struct kthreadlist *, int);

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
 * Locate a kthread by number
 */
struct kthread *
ktfind(tid)
	register pid_t tid;
{
	register struct kthread *kt;

	LIST_FOREACH(kt, TIDHASH(tid), kt_hash) {
		if (kt->kt_tid == tid) {
			return (kt);
		}
	}
	return (NULL);
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

struct kthread *
ktfind1(tid, chan)
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

/* Insert a kthread onto allkthread list and remove kthread from the freekthread list */
void
kthread_enqueue(kt, chan)
	struct kthread *kt;
	int chan;
{
	kthread_remove(kt, chan);						/* off freekthread */
	kthread_add(&allkthread, kt, chan);				/* onto allkthread */
}

/* Remove a kthread from allkthread list and insert kthread onto the zombkthread list */
void
kthread_dequeue(kt, chan)
	struct kthread *kt;
	int chan;
{
	kthread_remove(kt, chan);						/* off allkthread */
	kthread_add(&zombkthread, kt, chan);			/* onto zombkthread */
	kt->kt_stat = KT_SZOMB;
}

int
kthread_alloc(func, arg, kt, name)
	void (*func)(void *);
	void *arg;
	struct kthread *kt;
	char *name;
{
	struct proc *p;
	struct mpx *mpx;
	int error;

	error = kthread_create(func, arg, &p, name);
	if (error != 0) {
		return (error);
	}
	kt = p->p_kthreado;
	if (kt != NULL) {
		kt->kt_flag |= KT_INMEM | KT_SYSTEM | KT_NOCLDWAIT;
	}
	return (0);
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
kthread_free(kt, chan)
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

#ifdef notyet
/* Insert a kthread onto allkthread list and remove kthread from the freekthread list */
void
kthread_enqueue(kt)
	struct kthread *kt;
{
	kthread_remove(kt, chan);						/* off freekthread */
	kthread_add(&allkthread, kt, chan)				/* onto allkthread */

	LIST_REMOVE(kt, kt_list);						/* off freekthread */
	LIST_INSERT_HEAD(&allkthread, kt, kt_list);		/* onto allkthread */
}

/* Remove a kthread from allkthread list and insert kthread onto the zombkthread list */
void
kthread_dequeue(kt)
	struct kthread *kt;
{
	LIST_REMOVE(kt, kt_list);						/* off allkthread */
	LIST_INSERT_HEAD(&zombkthread, kt, kt_list);	/* onto zombkthread */
	kt->kt_stat = KT_SZOMB;
}

/* return kthread from kthreadlist (i.e. allkthread, freekthread, zombkthread) list if not null and matching tid */
struct kthread *
kthread_find(ktlist, tid)
	struct kthreadlist *ktlist;
	pid_t tid;
{
	struct kthread *kt;
	LIST_FOREACH(kt, ktlist, kt_list) {
		if (kt != NULL && kt->kt_tid == tid) {
			return (kt);
		}
	}
	return (NULL);
}

/* reap all non-null zombie kthreads from zombkthread list */
void
kthread_zombie(void)
{
	struct kthread *kt;
	if (!LIST_EMPTY(&zombkthread)) {
		LIST_FOREACH(kt, &zombkthread, kt_list) {
			if (kt != NULL) {
				LIST_REMOVE(kt, kt_list);						/* off zombkthread */
				LIST_INSERT_HEAD(&freekthread, kt, kt_list);	/* onto freekthread */
			}
		}
	}
}
#endif

/*
 * An mxthread is a multiplexed kernel thread.
 * Mxthreads are confined to the kthread which created it.
 * A single kthread can create a limited number (TBA) of mxthreads.
 */
