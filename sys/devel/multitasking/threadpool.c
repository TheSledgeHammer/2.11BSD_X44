/*
 * threadpool.c
 *
 *  Created on: 17 Feb 2020
 *      Author: marti
 */

#include <multitasking/threadpool.h>

void
threadpool_init()
{
	TAILQ_INIT(&itcp_head);
	TAILQ_INIT(&ktpool);
	TAILQ_INIT(&utpool);
}


/* Threadpool's FIFO Queue (IPC) */
void
kthreadpool_send(ktpool, itc)
    struct kern_threadpool *ktpool;
	struct itc_threadpool *itc;
{
    /* command / action */
	itc->itcp_ktpool = ktpool;
	itc->itcp_job = ktpool->ktp_jobs;  /* add/ get current job */
	/* send flagged jobs */
	ktpool->ktp_issender = TRUE;
	ktpool->ktp_isreciever = FALSE;

	/* update job pool */
}

void
kthreadpool_recieve(ktpool, itc)
    struct kern_threadpool *ktpool;
	struct itc_threadpool *itc;
{
    /* command / action */
	itc->itcp_ktpool = ktpool;
	itc->itcp_job = ktpool->ktp_jobs; /* add/ get current job */
	ktpool->ktp_issender = FALSE;
	ktpool->ktp_isreciever = TRUE;

	/* update job pool */
}

void
uthreadpool_send(utpool, itc)
    struct user_threadpool *utpool;
	struct itc_threadpool *itc;
{
    /* command / action */
	itc->itcp_utpool = utpool;
	itc->itcp_job = utpool->utp_jobs;
	/* send flagged jobs */
	utpool->utp_issender = TRUE;
	utpool->utp_isreciever = FALSE;

	/* update job pool */
}

void
uthreadpool_recieve(utpool, itc)
    struct user_threadpool *utpool;
	struct itc_threadpool *itc;
{
    /* command / action */
	itc->itcp_utpool = utpool;
	itc->itcp_job = utpool->utp_jobs;
	utpool->utp_issender = FALSE;
	utpool->utp_isreciever = TRUE;

	/* update job pool */
}


/* Sender checks request from receiver: providing info */
void
check(itc, tid)
	struct itc_threadpool *itc;
	tid_t tid;
{
	if(itc->itcp_ktpool.ktp_issender) {
		printf("kernel threadpool to send");
		if(itc->itcp_tid == tid) {

		} else {
			printf("kernel tid couldn't be found");
			goto retry;
		}
	}
	if(itc->itcp_utpool.utp_issender) {
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
	if(itc->itcp_ktpool.ktp_isreciever) {
		printf("kernel threadpool to recieve");
		if(itc->itcp_tid == tid) {
			printf("kernel tid found");

		} else {
			printf("kernel tid couldn't be found");
			goto retry;
		}
	}
	if(itc->itcp_utpool.utp_isreciever) {
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
