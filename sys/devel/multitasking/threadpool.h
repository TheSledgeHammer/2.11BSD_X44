/*
 * threadpool.h
 *
 *  Created on: 3 Jan 2020
 *      Author: marti
 */

#ifndef SYS_THREADPOOL_H_
#define SYS_THREADPOOL_H_

#include <multitasking/kernthreads/kthreadpool.h>
#include <multitasking/userthreads/uthreadpool.h>
//#include <sys/queue.h>

/*
 * Two Threadpools:
 * - Kernel Thread pool
 * - User Thread pool
 *
 * primary jobs:
 * - dispatch and hold x number of threads
 * - send & receive jobs between user & kernel threads when applicable
 *
 *
 * IPC:
 * - Use thread pool/s as the ipc. (bi-directional fifo queue)
 * 	- i.e. all requests go through the associated thread pool.
 * 	- requests sent in order received. (no priorities)
 * 	- confirmation: to prevent thread pools requesting the same task
 * 	- Tasks added to work queue
 * 	- Only jobs flagged for sending
 * 	-
 *
 * 	NetBSD:
 * 	- Threadpool_Thread: threads as they relate to lwp
 * 	- Threadpool: Actual Pool of Threads
 * 	- Threadpool_Job: Jobs to be done by threads in threadpool
 */

/*
 * TODO:
 * - Threadpool_KThreads: kernel threads as they relate to kthreads
 * - Threadpool_UThreads: user threads as they relate to uthreads/ufibres
 */

/* Threadpool Jobs */

TAILQ_HEAD(job_head, threadpool_job);
typedef void threadpool_job_fn_t(struct threadpool_job *);
struct threadpool_job {
	//job_lock
	TAILQ_ENTRY(threadpool_job)			job_entry;
	volatile unsigned int				job_refcnt;
	threadpool_job_fn_t					*job_fn;
	char								job_name[MAXCOMLEN];
};


/* Inter-Threadpool-Communication (ITPC) */

TAILQ_HEAD(itc_thread, itc_threadpool) 	itcp_head;
struct itc_threadpool {
	TAILQ_ENTRY(itc_threadpool) 		itcp_entry;		/* Threadpool IPC */
	struct kern_threadpool				itcp_ktpool;	/* Pointer to Kernel Threadpool */
	struct user_threadpool				itcp_utpool;	/* Pointer to User Threadpool */
	struct job_head						itcp_job;		/* Thread's Job */
	struct tgrp 						itcp_tgrp; 		/* Thread's Thread Group */
	tid_t								itcp_tid;		/* Thread's Thread ID */
};

/* General ITPC */
void threadpool_init();
void check(struct itc_threadpool *, tid_t);
void verify(struct itc_threadpool *, tid_t);

#endif /* SYS_THREADPOOL_H_ */
