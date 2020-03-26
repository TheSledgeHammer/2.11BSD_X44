/*
 * kern_thread_mutex.c
 *
 *  Created on: 6 Mar 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <mutex.h>
#include <kthread.h>

/* Initialize a Mutex on a kthread
 * Setup up Error flags */
int
kthread_mutex_init(mtx, kt)
    mutex_t mtx;
    kthread_t kt;
{
    int error = 0;
    mutex_init(mtx, mtx->mtx_prio, mtx->mtx_wmesg, mtx->mtx_timo, mtx->mtx_flags);
    kt->kt_mutex = mtx;
    mtx->mtx_ktlockholder = kt;
    return (error);
}

int
kthread_mutexmgr(mtx, flags, interlkp, kt)
    mutex_t mtx;
    unsigned int flags;
    struct simplelock *interlkp;
    kthread_t kt;
{
    int error = 0;
    tid_t tid;
    if (kt) {
        tid = kt->kt_tid;
    } else {
        tid = MTX_THREAD;
    }
    return mutexmgr(mtx, flags, tid, interlkp);
}
