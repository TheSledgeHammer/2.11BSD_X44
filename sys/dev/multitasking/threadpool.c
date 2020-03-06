/*
 * threadpool.c
 *
 *  Created on: 17 Feb 2020
 *      Author: marti
 */

#include <dev/multitasking/threadpool.h>

threadinit()
{
	//TAILQ_INIT(&ktpool_head);
	//TAILQ_INIT(&utpool_head);
	TAILQ_INIT(&itcp_head);
}

void
insert_kern_threads(ktpool)
    struct kern_threadpool *ktpool;
{
	//TAILQ_INSERT_HEAD(&ktpool_head, ktpool, ktp_entry);
}

void
remove_kern_threads(ktpool)
    struct kern_threadpool *ktpool;
{
	//TAILQ_REMOVE(&ktpool_head, ktpool, ktp_entry);
}

void
insert_user_threads(utpool)
    struct user_threadpool *utpool;
{
	//TAILQ_INSERT_HEAD(&utpool_head, utpool, utp_entry);
}

void
remove_user_threads(utpool)
    struct user_threadpool *utpool;
{
   //TAILQ_REMOVE(&utpool_head, utpool, utp_entry);
}

/* Threadpool's FIFO Queue (IPC) */
void
kerntpool_send(ktpool, itc)
    struct kern_threadpool *ktpool;
	struct itc_threadpool *itc;
{
    /* command / action */
	itc->itcp_ktpool = ktpool;
	itc->itcp_job = ktpool->ktp_jobs;  /* add get current job */
}

void
kerntpool_recieve(ktpool, itc)
    struct kern_threadpool *ktpool;
	struct itc_threadpool *itc;
{
    /* command / action */

}

void
usertpool_send(utpool, itc)
    struct user_threadpool *utpool;
	struct itc_threadpool *itc;
{
    /* command / action */

}

void
usertpool_recieve(utpool, itc)
    struct user_threadpool *utpool;
	struct itc_threadpool *itc;
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
