/*
 * uthreadpool.h
 *
 *  Created on: 2 May 2020
 *      Author: marti
 */
#include <multitasking/userthreads/uthread.h>


/* User Threadpool */

TAILQ_HEAD(uthreadpool, user_threadpool) utpool;
struct user_threadpool {
	struct uthreads						*utp_uthread;		/* user threads */
	TAILQ_ENTRY(user_threadpool) 		utp_entry;			/* list uthread entries */
    struct itc_thread					utp_itc;			/* threadpool ipc ptr */

    boolean_t							utp_issender;		/* sender */
    boolean_t							utp_isreciever;		/* reciever */

    int 								utp_refcnt;			/* total thread count in pool */
    int 								utp_active;			/* active thread count */
    int									utp_inactive;		/* inactive thread count */


    struct job_head						utp_jobs;
    //flags, refcnt, priority, lock
    //flags: send, (verify: success, fail), retry_attempts
};

extern void uthreadpool_send(struct user_threadpool *, struct itc_threadpool *);
extern void uthreadpool_receive(struct user_threadpool *, struct itc_threadpool *);
