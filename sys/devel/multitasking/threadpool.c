/*
 * threadpool.c
 *
 *  Created on: 17 Feb 2020
 *      Author: marti
 */

#include <multitasking/threadpool.h>

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
	itc->itcp_job = ktpool->ktp_jobs;  /* add/ get current job */
	/* send flagged jobs */
	ktpool->is_sender = TRUE;
	ktpool->is_reciever = FALSE;

	/* update job pool */
}

void
kerntpool_recieve(ktpool, itc)
    struct kern_threadpool *ktpool;
	struct itc_threadpool *itc;
{
    /* command / action */
	itc->itcp_ktpool = ktpool;
	itc->itcp_job = ktpool->ktp_jobs; /* add/ get current job */
	ktpool->is_sender = FALSE;
	ktpool->is_reciever = TRUE;

	/* update job pool */
}

void
usertpool_send(utpool, itc)
    struct user_threadpool *utpool;
	struct itc_threadpool *itc;
{
    /* command / action */
	itc->itcp_utpool = utpool;
	itc->itcp_job = utpool->utp_jobs;
	/* send flagged jobs */
	utpool->is_sender = TRUE;
	utpool->is_reciever = FALSE;

	/* update job pool */
}

void
usertpool_recieve(utpool, itc)
    struct user_threadpool *utpool;
	struct itc_threadpool *itc;
{
    /* command / action */
	itc->itcp_utpool = utpool;
	itc->itcp_job = utpool->utp_jobs;
	utpool->is_sender = FALSE;
	utpool->is_reciever = TRUE;

	/* update job pool */
}


/* Sender checks request from receiver: providing info */
void
check(itc, tid)
	struct itc_threadpool *itc;
	tid_t tid;
{
	if(itc->itcp_ktpool.is_sender) {
		printf("kernel threadpool to send");
		if(itc->itcp_tid == tid) {

		} else {
			printf("kernel tid couldn't be found");
			goto retry;
		}
	}
	if(itc->itcp_utpool.is_sender) {
		printf("user threadpool to send");
		if(itc->itcp_tid == tid) {

		} else {
			printf("user tid couldn't be found");
			goto retry;
		}
	}
retry:
	"todo: retry on fail";
}

/* Receiver verifies request to sender: providing info */
void
verify(itc, tid)
	struct itc_threadpool *itc;
	tid_t tid;
{
	if(itc->itcp_ktpool.is_reciever) {
		printf("kernel threadpool to recieve");
		if(itc->itcp_tid == tid) {
			printf("kernel tid found");

		} else {
			printf("kernel tid couldn't be found");
			goto retry;
		}
	}
	if(itc->itcp_utpool.is_reciever) {
		printf("user threadpool to recieve");
		if(itc->itcp_tid == tid) {
			printf("user tid found");
		} else {
			printf("user tid couldn't be found");
			goto retry;
		}
	}

retry:
	"todo: retry on fail";
}
