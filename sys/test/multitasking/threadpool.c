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
    LIST_INIT(&ktpool_head);
}

void
insert_kern_threads(ktpool)
    struct kern_threadpool *ktpool;
{
    LIST_INSERT_HEAD(&ktpool_head, ktpool, ktpool_wqueue);
}

void
remove_kern_threads(ktpool)
    struct kern_threadpool *ktpool;
{
    LIST_REMOVE(ktpool, ktpool_wqueue);
}

void
init_user_threadpool()
{
    LIST_INIT(&utpool_head);
}

void
insert_user_threads(utpool)
    struct user_threadpool *utpool;
{
    LIST_INSERT_HEAD(&utpool_head, utpool, utpool_wqueue);
}

void
remove_user_threads(utpool)
    struct user_threadpool *utpool;
{
    LIST_REMOVE(utpool, utpool_wqueue);
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
