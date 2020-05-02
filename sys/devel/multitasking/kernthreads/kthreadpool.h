/*
 * kthreadpool.h
 *
 *  Created on: 2 May 2020
 *      Author: marti
 */

#include <multitasking/kernthreads/kthread.h>


/* Kernel Threads in Kernel Threadpool */

/* Kernel Threadpool */

TAILQ_HEAD(kthreadpool, kern_threadpool) ktpool;
struct kern_threadpool {
	struct kthreads						*ktp_kthread;		/* kernel threads */
	TAILQ_ENTRY(kern_threadpool) 		ktp_entry;			/* list kthread entries */
    struct itc_thread					ktp_itc;			/* threadpool ipc ptr */

    boolean_t							ktp_issender;		/* sender */
    boolean_t							ktp_isreciever;		/* reciever */

    int 								ktp_refcnt;			/* total thread count in pool */
    int 								ktp_active;			/* active thread count */
    int									ktp_inactive;		/* inactive thread count */


    struct job_head						ktp_jobs;

    //flags, refcnt, priority, lock, verify
    //flags: send, (verify: success, fail), retry_attempts
};


enqueue(); 		/* add to kthread threadpool */
dequeue();		/* remove from kthread threadpool */

/* ITC Functions */
extern void kthreadpool_send(struct kern_threadpool *, struct itc_threadpool *);
extern void kthreadpool_receive(struct kern_threadpool *, struct itc_threadpool *);
