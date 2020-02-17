/*
 * threadpool.h
 *
 *  Created on: 3 Jan 2020
 *      Author: marti
 */

#ifndef SYS_THREADPOOL_H_
#define SYS_THREADPOOL_H_

#include <test/multitasking/kthreads.h>
#include <test/multitasking/uthreads.h>
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
 * IPC:
 * - Use thread pool/s as the ipc. (bi-directional fifo queue)
 * 	- i.e. all requests go through the associated thread pool.
 * 	- requests sent in order received. (no priorities)
 * 	- confirmation: to prevent thread pools requesting the same task
 * 	- Tasks added to work queue
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

TAILQ_HEAD(job_head, threadpool_job);
TAILQ_HEAD(kthread_head, threadpool_kthreads);
TAILQ_HEAD(uthread_head, threadpool_uthreads);

TAILQ_HEAD(ktpool, kern_threadpool) ktpool_head;
TAILQ_HEAD(utpool, user_threadpool) utpool_head;

/* Threadpool Jobs */
typedef void threadpool_job_fn_t(struct threadpool_job *);
struct threadpool_job {
	//job_lock
	TAILQ_ENTRY(threadpool_job)			job_entry;
	volatile unsigned int				job_refcnt;
	threadpool_job_fn_t					*job_fn;
	char								job_name[MAXCOMLEN];
};

/* Kernel Threads in Kernel Threadpool */
struct threadpool_kthreads {
	struct kthreads						*tpk_kt;
	char								*tpk_kt_name;
	struct kern_threadpool				*tpk_pool;
	struct threadpool_job				*tpk_job;
	TAILQ_ENTRY(threadpool_kthreads)	tpk_entry;
};

/* Kernel Threadpool */
struct kern_threadpool {
	TAILQ_ENTRY(kern_threadpool) 		ktp_entry;  /* TAILQ or CIRCULARQ?? */
    struct job_head						ktp_jobs;
    struct kthread_head					ktp_idle_threads;
    //flags, refcnt, priority, lock
};

/* User Threads in User Threadpool */
struct threadpool_uthreads {
	struct uthreads						*tpu_ut;
	char								*tpu_ut_name;
	struct user_threadpool				*tpu_pool;
	struct threadpool_job				*tpu_job;
	TAILQ_ENTRY(threadpool_uthreads) 	tpu_entry;
};

/* User Threadpool */
struct user_threadpool {
	TAILQ_ENTRY(user_threadpool) 		utp_entry; /* TAILQ or CIRCULARQ?? */
    struct job_head						utp_jobs;
    struct uthread_head					utp_idle_threads;
    //flags, refcnt, priority, lock
};

struct fifo_queue {
    /* thread (kernel or user), tgrp, tid, command */

};

/* fifo queue */
void kerntpool_send();
void kerntpool_receive();

void usertpool_send();
void usertpool_receive();
void check();
void verify();

#endif /* SYS_THREADPOOL_H_ */
