/*
 * user_thread.c
 *
 *  Created on: 2 May 2020
 *      Author: marti
 */
#include<sys/malloc.h>
#include <mutex.h>
#include <uthread.h>

int
uthread_create(uthread_t ut)
{
	ut->ut_flag = TSWAIT | TSREADY;
}

int
uthread_join(uthread_t ut)
{

}

int
uthread_cancel(uthread_t ut)
{

}

int
uthread_exit(uthread_t ut)
{

}

int
uthread_detach(uthread_t ut)
{

}

int
uthread_equal(uthread_t ut1, uthread_t ut2)
{
	if(ut1 > ut2) {
		return (1);
	} else if(ut1 < ut2) {
		return (-1);
	}
	return (0);
}

int
uthread_kill(uthread_t ut)
{

}

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
uthread_mutexmgr(mtx, flags, ut)
    mutex_t mtx;
    unsigned int flags;
    uthread_t ut;
{
    int error = 0;
    tid_t tid;
    if (ut) {
        tid = ut->ut_tid;
    } else {
        tid = MTX_THREAD;
    }
    return mutexmgr(mtx, flags, tid);
}
