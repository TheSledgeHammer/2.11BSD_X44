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


int	maxthread;// = NTHREAD;

struct tidhashhead *tidhashtbl;
u_long tidhash;
struct tgrphashhead *tgrphashtbl;
u_long tgrphash;

struct kthreadlist 	allkthread;
struct kthreadlist 	zombkthread;
struct kthreadlist 	freekthread;

struct lock_holder 	kthread_loholder;

void
kthreadinit(kt)
	struct kthread *kt;
{
	ktqinit(kt);
	tidhashtbl = hashinit(maxthread / 4, M_PROC, &tidhash);
	tgrphashtbl = hashinit(maxthread / 4, M_PROC, &tgrphash);

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
	register struct kthread *kt;
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

/* Insert a kthread onto allkthread list and remove kthread from the freekthread list */
void
kthread_enqueue(kt)
	struct kthread *kt;
{
	LIST_REMOVE(kt, kt_list);			/* off freekthread */
	LIST_INSERT_HEAD(kt, &allkthread, kt_list);	/* onto allkthread */
}

/* Remove a kthread from allkthread list and insert kthread onto the zombkthread list */
void
kthread_dequeue(kt)
	struct kthread *kt;
{
	LIST_REMOVE(kt, kt_list);			/* off allkthread */
	LIST_INSERT_HEAD(kt, &zombkthread, kt_list);	/* onto zombkthread */
	kt->kt_state = KT_SZOMB;
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

/*
 * An mxthread is a multiplexed kernel thread.
 * Mxthreads are confined to the kthread which created it.
 * A single kthread can create a limited number (TBA) of mxthreads.
 */

#define M_MXTHREAD 95

struct mxthread {
	LIST_ENTRY(mxthread) 	mx_list;
	struct kthread			*mx_kthread; /* kernel thread */

	pid_t 					mx_tid;
	struct pgrp 			*mx_pgrp;

	int 					mx_channel;
};

void
mxthread_init(kt)
	struct kthread *kt;
{
	KASSERT(kt != NULL);

	LIST_INIT(kt->kt_mxthreads);


}

struct mxthread *
mxthread_alloc(channel)
	int channel;
{
	register struct mxthread *mx;

	mx = (struct mxthread *)calloc(channel, sizeof(struct mxthread), M_MXTHREAD, M_WAITOK);
	mx->mx_channel = channel;
	return (mx);
}

void
mxthread_free(mx)
	struct mxthread *mx;
{
	free(mx, M_MXTHREAD);
}

void
mxthread_add(kt, channel)
	struct kthread 	*kt;
	int channel;
{
	register struct mxthread *mx;

	mx = mxthread_alloc(channel);
	mx->mx_kthread = kt;
	//mx->mx_tid	= kt->kt_tid;
	//mx->mx_pgrp = kt->kt_pgrp;

	KTHREAD_LOCK(kt);
	LIST_INSERT_HEAD(kt->kt_mxthreads, mx, mx_list);
	KTHREAD_UNLOCK(kt);
}

struct mxthread *
mxthread_find(kt, channel)
	struct kthread 	*kt;
	int channel;
{
	register struct mxthread *mx;

	KTHREAD_LOCK(kt);
	LIST_FOREACH(mx, kt->kt_mxthreads, mx_list) {
		if (mx->mx_kthread == kt && mx->mx_channel == channel) {
			KTHREAD_UNLOCK(kt);
			return (mx);
		}
	}
	KTHREAD_UNLOCK(kt);
	return (NULL);
}

void
mxthread_remove(kt, channel)
	struct kthread 	*kt;
	int channel;
{
	register struct mxthread *mx;

	KTHREAD_LOCK(kt);
	mx = mxthread_find(kt, channel);
	if (mx != NULL) {
		mx->mx_channel = -1;
		LIST_REMOVE(mx, mx_list);
	}
	if (mx == NULL) {
		mxthread_free(mx);
	}
	KTHREAD_UNLOCK(kt);
}

