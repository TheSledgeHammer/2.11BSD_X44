/*
 * threadpool.c
 *
 *  Created on: 17 Feb 2020
 *      Author: marti
 */

#include <test/multitasking/threadpool.h>


void
init_kern_threadpool()
{
	TAILQ_INIT(&ktpool_head);
}

void
insert_kern_threads(ktpool)
    struct kern_threadpool *ktpool;
{
	TAILQ_INSERT_HEAD(&ktpool_head, ktpool, ktp_entry);
}

void
remove_kern_threads(ktpool)
    struct kern_threadpool *ktpool;
{
	TAILQ_REMOVE(&ktpool_head, ktpool, ktp_entry);
}

void
init_user_threadpool()
{
	TAILQ_INIT(&utpool_head);
}

void
insert_user_threads(utpool)
    struct user_threadpool *utpool;
{
	TAILQ_INSERT_HEAD(&utpool_head, utpool, utp_entry);
}

void
remove_user_threads(utpool)
    struct user_threadpool *utpool;
{
    TAILQ_REMOVE(&utpool_head, utpool, utp_entry);
}

/* Threadpool's FIFO Queue (IPC) */
void
kerntpool_send(ktpool, tid, tgrp)
    struct kern_threadpool *ktpool;
	tid_t tid, tgrp;
{
    /* command / action */
}

void
kerntpool_recieve(ktpool, tid, tgrp)
    struct kern_threadpool *ktpool;
	tid_t tid, tgrp;
{
    /* command / action */
}

void
usertpool_send(utpool, tid, tgrp)
    struct user_threadpool *utpool;
	tid_t tid, tgrp;
{
    /* command / action */

}

void
usertpool_recieve(utpool, tid, tgrp)
    struct user_threadpool *utpool;
	tid_t tid, tgrp;
{
    /* command / action */
}

/* Sender checks request from receiver: providing info */
void
check()
{

}

/* Receiver verifies request to sender: providing info */
void
verify()
{

}
