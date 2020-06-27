/*
 * Copyright (c) 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code contains ideas from software contributed to Berkeley by
 * Avadis Tevanian, Jr., Michael Wayne Young, and the Mach Operating
 * System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * 	@(#)kern_lock.c 8.18 (Berkeley)  5/21/95
 */
/* Kernel Mutex is an extension/ abstraction of a simple lock (kern_lock) for compatibility with kernel & user threads. */

#include <sys/param.h>
#include <sys/proc.h>
#include "../sys/lock.h"
#include "devel/sys/kthread.h"
#include "devel/sys/mutex.h"
#include "devel/sys/uthread.h"


/* Initialize a mutex_lock */
void
mutex_init(mtx, prio, wmesg, timo, flags)
    mutex_t mtx;
    char *wmesg;
    int prio, timo;
    unsigned int flags;
{
/*    bzero(mtx, sizeof(struct mutex));
    simple_lock_init(&mtx->mtx_interlock);
    mtx->mtx_lock = 0;
    mtx->mtx_flags = flags & MTX_EXTFLG_MASK;
    mtx->mtx_prio = prio;
    mtx->mtx_timo = timo;
    mtx->mtx_wmesg = wmesg;
    mtx->mtx_lockholder = MTX_NOTHREAD;
    mtx->mtx_ktlockholder = NULL;
    mtx->mtx_utlockholder = NULL;
    */
    lock_init(&mtx->mtx_interlock, prio, wmesg, timo, flags);
}

int
mutexstatus(mtx)
    mutex_t mtx;
{
    int lock_type = 0;
    mutex_lock(mtx);
    if(mtx->mtx_exclusivecount != 0) {
        lock_type = MTX_EXCLUSIVE;
    } else if(mtx->mtx_sharecount != 0) {
        lock_type = MTX_SHARED;
    }
    mutex_unlock(mtx);
    return (lock_type);
}

/* should apply a mutex lock */
int
mutexmgr(mtx, flags, tid)
    __volatile mutex_t mtx;
    unsigned int flags;
    tid_t tid;
{
    int error;
    int extflags;
    error = 0;

    mutex_lock(&mtx);
    if (flags & MTX_INTERLOCK) {
    	mutex_unlock(&mtx);
    }
    extflags = (flags | mtx->mtx_flags) & MTX_EXTFLG_MASK;
#ifdef DIAGNOSTIC
    if(mtx->mtx_flags & (MTX_DRAINING | MTX_DRAIN)) {
        if(mtx->mtx_flags & MTX_DRAIN)
            printf("mutexmgr: using decommissioned lock"); /* panic */
        if ((flags & MTX_TYPE_MASK) != MTX_RELEASE || mtx->mtx_lockholder != tid)
            printf("mutexmgr: non-release on draining lock: %d\n", flags & MTX_TYPE_MASK); /* panic */
        mtx->mtx_flags &= ~MTX_DRAINING;
        if ((flags & MTX_REENABLE) == 0)
            mtx->mtx_flags |= MTX_DRAINED;
    }
#endif DIAGNOSTIC

    switch (flags & MTX_TYPE_MASK) {

        case MTX_SHARED:
            if(mtx->mtx_lockholder != tid) {
                if ((extflags & MTX_NOWAIT) && (mtx->mtx_flags & (MTX_HAVE_EXCL | MTX_WANT_EXCL | MTX_WANT_UPGRADE))) {
                    error = EBUSY;
                    break;
                }
                ACQUIRE(mtx, error, extflags, mtx->mtx_flags & (MTX_HAVE_EXCL | MTX_WANT_EXCL | MTX_WANT_UPGRADE));
                if (error)
                    break;
                mtx->mtx_sharecount++;
               // COUNT(p, 1);
                break;
            }
            mtx->mtx_sharecount++;
            //COUNT(p, 1);
            //break;
        case MTX_DOWNGRADE:
    		if (mtx->mtx_lockholder != tid || mtx->mtx_exclusivecount == 0)
    			panic("mutexmgr: not holding exclusive lock");
    		mtx->mtx_sharecount += mtx->mtx_exclusivecount;
    		mtx->mtx_exclusivecount = 0;
    		mtx->mtx_flags &= ~MTX_HAVE_EXCL;
    		mtx->mtx_lockholder = MTX_NOTHREAD;
    		if (mtx->mtx_waitcount)
    			wakeup((void *)mtx);
    		break;

        case MTX_EXCLUPGRADE:
    		/*
    		 * If another process is ahead of us to get an upgrade,
    		 * then we want to fail rather than have an intervening
    		 * exclusive access.
    		 */
    		if (mtx->mtx_flags & MTX_WANT_UPGRADE) {
    			mtx->mtx_sharecount--;
    			//COUNT(p, -1);
    			error = EBUSY;
    			break;
    		}
    		/* fall into normal upgrade */

        case MTX_UPGRADE:
    		/*
    		 * Upgrade a shared lock to an exclusive one. If another
    		 * shared lock has already requested an upgrade to an
    		 * exclusive lock, our shared lock is released and an
    		 * exclusive lock is requested (which will be granted
    		 * after the upgrade). If we return an error, the file
    		 * will always be unlocked.
    		 */
    		if (mtx->mtx_lockholder == tid || mtx->mtx_sharecount <= 0)
    			panic("mutexmgr: upgrade exclusive lock");
    		mtx->mtx_sharecount--;
    		//COUNT(p, -1);
    		/*
    		 * If we are just polling, check to see if we will block.
    		 */
    		if ((extflags & MTX_NOWAIT) &&
    		    ((mtx->mtx_flags & MTX_WANT_UPGRADE) ||
    		    		mtx->mtx_sharecount > 1)) {
    			error = EBUSY;
    			break;
    		}
    		if ((mtx->mtx_flags & MTX_WANT_UPGRADE) == 0) {
    			/*
    			 * We are first shared lock to request an upgrade, so
    			 * request upgrade and wait for the shared count to
    			 * drop to zero, then take exclusive lock.
    			 */
    			mtx->mtx_flags |= MTX_WANT_UPGRADE;
    			ACQUIRE(mtx, error, extflags, mtx->mtx_sharecount);
    			mtx->mtx_flags &= ~MTX_WANT_UPGRADE;
    			if (error)
    				break;
    			mtx->mtx_flags |= MTX_HAVE_EXCL;
    			mtx->mtx_lockholder = tid;
    			if (mtx->mtx_exclusivecount != 0)
    				panic("mutexmgr: non-zero exclusive count");
    			mtx->mtx_exclusivecount = 1;
    			//COUNT(p, 1);
    			break;
    		}
    		/*
    		 * Someone else has requested upgrade. Release our shared
    		 * lock, awaken upgrade requestor if we are the last shared
    		 * lock, then request an exclusive lock.
    		 */
    		if (mtx->mtx_sharecount == 0 && mtx->mtx_waitcount)
    			wakeup((void *)mtx);
    		/* fall into exclusive request */
    		//break;
        case MTX_EXCLUSIVE:
            if(mtx->mtx_lockholder == tid && tid != MTX_THREAD) {
                if ((extflags & MTX_CANRECURSE) == 0)
                    printf("mutexmgr: locking against myself"); /* panic */
                mtx->mtx_exclusivecount++;
                break;
            }
            if ((extflags & MTX_NOWAIT) && ((mtx->mtx_flags & (MTX_HAVE_EXCL | MTX_WANT_EXCL | MTX_WANT_UPGRADE)) || mtx->mtx_sharecount != 0)) {
                error = EBUSY;
                break;
            }
            ACQUIRE(mtx, error, extflags, mtx->mtx_flags & (MTX_HAVE_EXCL | MTX_WANT_EXCL));
            if(error)
                break;
            mtx->mtx_flags |= MTX_WANT_EXCL;

            ACQUIRE(mtx, error, extflags, mtx->mtx_flags & MTX_WANT_UPGRADE);
            mtx->mtx_flags &= ~MTX_WANT_EXCL;
            if(error)
                break;
            mtx->mtx_flags |= MTX_HAVE_EXCL;
            mtx->mtx_lockholder = tid;
            if(mtx->mtx_exclusivecount != 0)
                printf("mutexmgr: non-zero exclusive count");  /* panic */
            mtx->mtx_exclusivecount = 1;
            break;
        case MTX_RELEASE:
    		if (mtx->mtx_exclusivecount != 0) {
    			if (tid != mtx->mtx_lockholder)
    				panic("mutexmgr: pid %d, not %s %d unlocking",
    				    tid, "exclusive lock holder",
						mtx->mtx_lockholder);
    			mtx->mtx_exclusivecount--;
    			COUNT(p, -1);
    			if (mtx->mtx_exclusivecount == 0) {
    				mtx->mtx_flags &= ~MTX_HAVE_EXCL;
    				mtx->mtx_lockholder = MTX_NOTHREAD;
    			}
    		} else if (mtx->mtx_sharecount != 0) {
    			mtx->mtx_sharecount--;
    			//COUNT(p, -1);
    		}
    		if (mtx->mtx_waitcount)
    			wakeup((void *)mtx);
    		break;
        case MTX_DRAIN:
    		/*
    		 * Check that we do not already hold the lock, as it can
    		 * never drain if we do. Unfortunately, we have no way to
    		 * check for holding a shared lock, but at least we can
    		 * check for an exclusive one.
    		 */
    		if (mtx->mtx_lockholder == tid)
    			panic("mutxmgr: draining against myself");
    		/*
    		 * If we are just polling, check to see if we will sleep.
    		 */
    		if ((extflags & MTX_NOWAIT) && ((mtx->mtx_flags &
    		     (MTX_HAVE_EXCL | MTX_WANT_EXCL | MTX_WANT_UPGRADE)) ||
    				mtx->mtx_sharecount != 0 || mtx->mtx_waitcount != 0)) {
    			error = EBUSY;
    			break;
    		}
    		PAUSE(mtx, ((mtx->mtx_flags &
    		     (MTX_HAVE_EXCL | MTX_WANT_EXCL | MTX_WANT_UPGRADE)) ||
    				mtx->mtx_sharecount != 0 || mtx->mtx_waitcount != 0));
    		for (error = 0; ((mtx->mtx_flags &
    		     (MTX_HAVE_EXCL | MTX_WANT_EXCL | MTX_WANT_UPGRADE)) ||
    				mtx->mtx_sharecount != 0 || mtx->mtx_waitcount != 0); ) {
    			mtx->mtx_flags |= MTX_WAITDRAIN;
    			mutex_unlock(&mtx);
    			if (error == tsleep((void *)&mtx->mtx_flags, mtx->mtx_prio,
    					mtx->mtx_wmesg, mtx->mtx_timo))
    				return (error);
    			if ((extflags) & MTX_SLEEPFAIL)
    				return (ENOLCK);
    			mutex_lock(&mtx);
    		}
    		mtx->mtx_flags |= MTX_DRAINING | MTX_HAVE_EXCL;
    		mtx->mtx_lockholder = tid;
    		mtx->mtx_exclusivecount = 1;
    		break;
        default:
        	mutex_unlock(&mtx);
            printf("mutexmgr: unknown locktype request %d", flags & MTX_TYPE_MASK); /* panic */
            break;
    }
    if ((mtx->mtx_flags & MTX_WAITDRAIN)
    && ((mtx->mtx_flags & (MTX_HAVE_EXCL | MTX_WANT_EXCL | MTX_WANT_UPGRADE)) == 0
    && mtx->mtx_sharecount == 0 && mtx->mtx_waitcount == 0)) {
        mtx->mtx_flags &= ~MTX_WAITDRAIN;
        wakeup((void *)&mtx->mtx_flags);
    }
    mutex_unlock(&mtx);
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
	return 0;
}

/* Not yet implemented */
int
mutex_destroy(mtx)
    __volatile mutex_t mtx;
{
    return 0;
}
