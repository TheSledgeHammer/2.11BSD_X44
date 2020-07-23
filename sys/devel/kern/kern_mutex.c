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
#include <sys/proc.h>
#include "devel/sys/mutex.h"
#include "devel/sys/kthread.h"
#include "devel/sys/uthread.h"

/*
 * Mutex's:
 * - mutex: use pid on lockmgr (akin to rwlockmgr)
 * - determine thread then run lockmgr
 * - may remove, ktlockholder & utlockholder from lock_init
 * 	- setup in mutex
 */

/* Initialize a mutex_lock */
void
mutex_init(mtx, prio, wmesg, timo, flags)
    mutex_t mtx;
    char *wmesg;
    int prio, timo;
    unsigned int flags;
{
    bzero(mtx, sizeof(struct mutex));
    simple_lock_init(&mtx->mtx_interlock);
    mtx->mtx_lock = 0;
    mtx->mtx_flags = flags & MTX_EXTFLG_MASK;
    mtx->mtx_prio = prio;
    mtx->mtx_timo = timo;
    mtx->mtx_wmesg = wmesg;
    mtx->mtx_lockholder = MTX_NOTHREAD;
    mtx->mtx_ktlockholder = NULL;
    mtx->mtx_utlockholder = NULL;
}

void
mutex_lock(mtx)
    __volatile mutex_t mtx;
{
    if (mtx->mtx_lock == 1) {
    	simple_lock(&mtx->mtx_interlock);
    }
}

int
mutex_lock_try(mtx)
    __volatile mutex_t mtx;
{
    return (simple_lock_try(&mtx->mtx_interlock));
}

void
mutex_unlock(mtx)
    __volatile mutex_t mtx;
{
    if (mtx->mtx_lock == 0) {
    	simple_unlock(&mtx->mtx_interlock);
    }
}

/* Not yet implemented */
int
mutex_timedlock(mtx)
	__volatile mutex_t mtx;
{
	simple_timelock(&mtx->mtx_interlock);
	return 0;
}

/* Not yet implemented */
int
mutex_destroy(mtx)
    __volatile mutex_t mtx;
{
	simple_destroy(&mtx->mtx_interlock);
    return 0;
}


void
set_proc_lock(lkp, p)
	struct lock *lkp;
	struct proc *p;
{
	lkp->lk_prlockholder = p;
	if(p == NULL) {
		lkp->lk_prlockholder = NULL;
	}
}

void
set_kthread_lock(lkp, kt)
	struct lock *lkp;
	struct kthread *kt;
{
	lkp->lk_ktlockholder = kt;
	if(kt == NULL) {
		lkp->lk_ktlockholder = NULL;
	}
}

void
set_uthread_lock(lkp, ut)
	struct lock *lkp;
	struct uthread *ut;
{
	lkp->lk_utlockholder = ut;
	if(ut == NULL) {
		lkp->lk_utlockholder = NULL;
	}
}

/* Returns proc if pid matches and lockholder is not null */
struct proc *
get_proc_lock(lkp, pid)
	struct lock *lkp;
	pid_t pid;
{
	struct proc *plk = lkp->lk_prlockholder;
	if(plk != NULL && plk == pfind(pid)) {
		return (plk);
	}
	return (NULL);
}

/* Returns kthread if pid matches and lockholder is not null */
struct kthread *
get_kthread_lock(lkp, pid)
	struct lock *lkp;
	pid_t pid;
{
	struct kthread *klk =  lkp->lk_ktlockholder;
	if(klk != NULL && klk == pfind(pid)) {
		return (klk);
	}
	return (NULL);
}

/* Returns uthread if pid matches and lockholder is not null */
struct uthread *
get_uthread_lock(lkp, pid)
	struct lock *lkp;
	pid_t pid;
{
	struct uthread *ulk = lkp->lk_utlockholder;
	if(ulk != NULL && ulk == pfind(pid)) {
		return (ulk);
	}
	return (NULL);
}
