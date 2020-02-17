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
 */

/* init_threadpool: setup thread id's and groups */

struct kern_threadpool {
	struct kthread *ktp_kthread;
    LIST_ENTRY(kern_threadpool) ktpool_wqueue;

};
LIST_HEAD(ktpool, kern_threadpool) ktpool_head;

struct user_threadpool {
	struct uthread *utp_uthread;
    LIST_ENTRY(user_threadpool) utpool_wqueue;
};
LIST_HEAD(utpool, user_threadpool) utpool_head;


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
