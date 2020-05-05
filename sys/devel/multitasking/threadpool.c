/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
