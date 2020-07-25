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
mutex_init(mtx, lkp, prio, wmesg, timo, flags)
    mutex_t mtx;
	struct lock *lkp;
    char *wmesg;
    int prio, timo;
    unsigned int flags;
{
    bzero(mtx, sizeof(struct mutex));
    simple_lock_init(&mtx->mtx_interlock);
    mtx->mtx_lockp = lkp;
    mtx->mtx_lock = 0;
    mtx->mtx_flags = flags & MTX_EXTFLG_MASK;
    mtx->mtx_prio = prio;
    mtx->mtx_timo = timo;
    mtx->mtx_wmesg = wmesg;
    mtx->mtx_lockholder = MTX_NOTHREAD;
    set_proc_lock(&mtx->mtx_lockp, NULL);
    set_kthread_lock(&mtx->mtx_lockp, NULL);
    set_uthread_lock(&mtx->mtx_lockp, NULL);
}

/* Initialize a mutex_lock */
mutex_init(mtx, lkp)
	mutex_t mtx;
	struct lock *lkp;
{
	bzero(mtx, sizeof(mutex_t));
	mtx->mtx_lockp = lkp;
    mtx->mtx_lock = lkp->lk_lock;
    mtx->mtx_flags = lkp->lk_flags;
    mtx->mtx_prio = lkp->lk_prio;
    mtx->mtx_timo = lkp->lk_timo;
    mtx->mtx_wmesg = lkp->lk_wmesg;
    mtx->mtx_lockholder = lkp->lk_lockholder;
    set_kthread_lock(&mtx->mtx_lockp, NULL);
    set_uthread_lock(&mtx->mtx_lockp, NULL);
	lockinit(lkp, mtx->mtx_prio, mtx->mtx_wmesg, mtx->mtx_timo, mtx->mtx_flags);
}

int
mutexmgr(mtx, flags, pid)
	mutex_t mtx;
	unsigned int flags;
	pid_t pid;
{
	int error;
	int extflags;
	struct lock *lkp = mtx->mtx_lockp;
	error = 0;

	if (!pid) {
		pid = MTX_THREAD;
	}
	mutex_lock(&mtx);
	if (flags & MTX_INTERLOCK) {
		mutex_unlock(&mtx);
	}
	extflags = (flags | mtx->mtx_flags) & MTX_EXTFLG_MASK;
	if(lkp != NULL) {
		if(get_proc_lock(lkp, pid)) {
			struct proc *p = get_proc_lock(lkp, pid);
			return (lockmgr(lkp, flags, mtx->mtx_interlock, p));
		}
		if(get_kthread_lock(lkp, pid)) {
			struct kthread *kt = get_kthread_lock(lkp, pid);
			return (lockmgr(lkp, flags, mtx->mtx_interlock, kt->kt_procp));
		}
		if(get_uthread_lock(lkp, pid)) {
			struct uthread *ut = get_uthread_lock(lkp, pid);
			return (lockmgr(lkp, flags, mtx->mtx_interlock, ut->ut_userp->u_procp));
		}
	} else {
		error = EBUSY;
	}
	return (error);
}

void
mutex_lock(mtx)
    __volatile mutex_t mtx;
{
    if (mtx->mtx_lock == 1) {
    	simple_lock(&mtx->mtx_interlock);
    }
}

void
mutex_unlock(mtx)
    __volatile mutex_t mtx;
{
    if (mtx->mtx_lock == 0) {
    	simple_unlock(&mtx->mtx_interlock);
    }
}

int
mutex_enter(mtx)
	__volatile mutex_t mtx;
{
	if(mtx->mtx_lock != 1) {
		mtx_lock(mtx);
	}
	return (0);
}

int
mutex_exit(mtx)
	__volatile mutex_t mtx;
{
	if(mtx->mtx_lock != 0) {
		mtx_unlock(mtx);
	}
	return (0);
}

int
mutex_lock_try(mtx)
    __volatile mutex_t mtx;
{
    return (simple_lock_try(&mtx->mtx_interlock));
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
