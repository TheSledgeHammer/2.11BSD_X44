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

struct tidhashhead *tidhashtbl;
u_long tidhash;
struct tgrphashhead *tgrphashtbl;
u_long tgrphash;

int	maxthread;// = NTHREAD;

struct kthreadlist allkthread;
struct kthreadlist zombkthread;

void
threadinit()
{
	ktqinit();
	utqinit();
	tidhashtbl = hashinit(maxthread / 4, M_PROC, &tidhash);
	tgrphashtbl = hashinit(maxthread / 4, M_PROC, &tgrphash);
}

/*
 * init the kthread queues
 */
void
ktqinit()
{
	LIST_INIT(&allkthread);
	LIST_INIT(&zombkthread);
}

/*
 * init the uthread queues
 */
void
utqinit()
{
	LIST_INIT(&alluthread);
	LIST_INIT(&zombuthread);
}

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
    threadinit();

	/* set up kernel thread */
    LIST_INSERT_HEAD(&allkthread, kt, kt_list);
    kt->kt_pgrp = &pgrp0;
    LIST_INSERT_HEAD(TGRPHASH(0), &pgrp0, pg_hash);
	p->p_kthreado = kt;

	/* give the kthread the same creds as the initial thread */
	kt->kt_ucred = p->p_ucred;
	crhold(kt->kt_ucred);

    /* setup kthread locks */
    mtx_init(kt->kt_mtx, &kthread_loholder, "kthread mutex", (struct kthread *)kt, kt->kt_tid, kt->kt_pgrp);

    /* initialize kthreadpools */
    kthreadpool_init();
}

/*
 * Locate a kthread by number
 */
struct kthread *
ktfind(tid)
	register pid_t tid;
{
	register struct kthread *kt;
	for (kt = LIST_FIRST(TIDHASH(tid)); kt != 0; kt = LIST_NEXT(kt, kt_hash))
		if(kt->kt_tid == tid)
			return (kt);
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
	for (tgrp = LIST_FIRST(TGRPHASH(pgid)); tgrp != NULL; tgrp = LIST_NEXT(tgrp, pg_hash)) {
		if (tgrp->pg_id == pgid) {
			return (tgrp);
		}
	}
	return (NULL);
}
