/*
 * user_thread_mutex.c
 *
 *  Created on: 6 Mar 2020
 *      Author: marti
 */
#include <sys/param.h>
#include <sys/proc.h>
#include <mutex.h>
#include <uthread.h>

int
uthread_mutex_init(mtx, ut)
    mutex_t mtx;
    uthread_t ut;
{
    int error = 0;
    mutex_init(mtx, mtx->mtx_prio, mtx->mtx_wmesg, mtx->mtx_timo, mtx->mtx_flags);
    ut->ut_mutex = mtx;
    mtx->mtx_utlockholder = ut;
    return (error);
}

int
uthread_mutexmgr(mtx, flags, interlkp, ut)
    mutex_t mtx;
    unsigned int flags;
    struct simplelock *interlkp;
    uthread_t ut;
{
    int error = 0;
    tid_t tid;
    if (ut) {
        tid = ut->ut_tid;
    } else {
        tid = MTX_THREAD;
    }
    return mutexmgr(mtx, flags, tid, interlkp);
}
