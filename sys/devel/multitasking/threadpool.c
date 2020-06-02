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
#include <sys/cdefs.h>
#include <sys/user.h>
#include <multitasking/threadpool.h>

void
itc_threadpool_init()
{
	TAILQ_INIT(&itc_head);
}

/* Add a thread to the itc queue */
void
itc_threadpool_enqueue(itc, tid)
	struct itc_threadpool *itc;
	tid_t tid;
{
	struct kthreadpool *ktpool;
	struct uthreadpool *utpool;
	/* check kernel threadpool is not null & has a job/task entry to send */
	if(ktpool != NULL && itc->itc_tid == tid) {
		itc->itc_ktpool = ktpool;
		ktpool->ktp_initcq = TRUE;
		TAILQ_INSERT_HEAD(itc_head, itc, itc_entry);
	}

	/* check user threadpool is not null & has a job/task entry to send */
	if(utpool != NULL && itc->itc_tid == tid) {
		itc->itc_utpool = utpool;
		utpool->utp_initcq = TRUE;
		TAILQ_INSERT_HEAD(itc_head, itc, itc_entry);
	}
}

/*
 * Remove a thread from the itc queue:
 * If threadpool entry is not null, search queue for entry & remove
 */
void
itc_threadpool_dequeue(itc, tid)
	struct itc_threadpool *itc;
	tid_t tid;
{
	struct kthreadpool *ktpool;
	struct uthreadpool 	   *utpool;

	if(ktpool != NULL) {
		TAILQ_FOREACH(itc, itc_head, itc_entry) {
			if(TAILQ_NEXT(itc, itc_entry)->itc_ktpool == ktpool) {
				if(itc->itc_tid == tid) {
					ktpool->ktp_initcq = FALSE;
					TAILQ_REMOVE(itc_head, itc, itc_entry);
				}
			}
		}
	}

	if(utpool != NULL) {
		TAILQ_FOREACH(itc, itc_head, itc_entry) {
			if(TAILQ_NEXT(itc, itc_entry)->itc_utpool == utpool) {
				if(itc->itc_tid == tid) {
					utpool->utp_initcq = FALSE;
					TAILQ_REMOVE(itc_head, itc, itc_entry);
				}
			}
		}
	}
}

/* Sender checks request from receiver: providing info */
void
check(itc, tid)
	struct itc_threadpool *itc;
	tid_t tid;
{
	struct kthreadpool *ktpool = itc->itc_ktpool;
	struct uthreadpool *utpool = itc->itc_utpool;

	if(ktpool->ktp_issender) {
		printf("kernel threadpool to send");
		if(itc->itc_tid == tid) {
			printf("kernel tid be found");
			/* check */
		} else {
			if(itc->itc_tid != tid) {
				if(ktpool->ktp_retcnt <= 5) { /* retry up to 5 times */
					if(ktpool->ktp_initcq) {
						/* exit and re-enter queue increasing retry count */
						itpc_threadpool_dequeue(itc);
						ktpool->ktp_retcnt++;
						itpc_threadpool_enqueue(itc);
					}
				} else {
					/* exit queue, reset count to 0 and panic */
					itpc_threadpool_dequeue(itc);
					ktpool->ktp_retcnt = 0;
					panic("kthreadpool x exited itc queue after 5 failed attempts");
				}
			}
		}
	}

	if(utpool->utp_issender) {
		printf("user threadpool to send");
		if(itc->itc_tid == tid) {
			printf("user tid be found");
			/* check */
		} else {
			if(itc->itc_tid != tid) {
				if(utpool->utp_retcnt <= 5) { /* retry up to 5 times */
					if(utpool->utp_initcq) {
						/* exit and re-enter queue, increasing retry count */
						itpc_threadpool_dequeue(itc);
						utpool->utp_retcnt++;
						itpc_threadpool_enqueue(itc);
					}
				} else {
					/* exit queue, reset count to 0 and panic */
					itpc_threadpool_dequeue(itc);
					utpool->utp_retcnt = 0;
					panic("kthreadpool x exited itc queue after 5 failed attempts");
				}
			}
		}
	}
}

/* Receiver verifies request to sender: providing info */
void
verify(itc, tid)
	struct itc_threadpool *itc;
	tid_t tid;
{
	struct kthreadpool *ktpool = itc->itc_ktpool;
	struct uthreadpool *utpool = itc->itc_utpool;

	if(ktpool->ktp_isreciever) {
		printf("kernel threadpool to recieve");
		if(itc->itc_tid == tid) {
			printf("kernel tid found");

		} else {
			printf("kernel tid couldn't be found");
		}
	}

	if(utpool->utp_isreciever) {
		printf("user threadpool to recieve");
		if(itc->itc_tid == tid) {
			printf("user tid found");
		} else {
			printf("user tid couldn't be found");
		}
	}
}
