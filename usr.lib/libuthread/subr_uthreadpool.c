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

#include <sys/devel/sys/uthread.h>
#include <sys/devel/sys/threadpool.h>

/* UThreadpool ITPC */

/* Add a thread to the itc queue */
void
itpc_uthreadpool_enqueue(itpc, pid)
	struct threadpool_itpc *itpc;
	pid_t pid;
{
	struct uthreadpool *utpool;

	/* check user threadpool is not null & has a job/task entry to send */
	if(utpool != NULL && itpc->itc_tid == pid) {
		itpc->itc_utpool = utpool;
		utpool->utp_initcq = TRUE;
		itpc->itc_refcnt++;
		TAILQ_INSERT_HEAD(itpc->itc_header, itpc, itc_entry);
	}
}

/*
 * Remove a thread from the itc queue:
 * If threadpool entry is not null, search queue for entry & remove
 */
void
itpc_uthreadpool_dequeue(itpc, pid)
	struct threadpool_itpc *itpc;
	pid_t pid;
{
	struct uthreadpool *utpool;

	if(utpool != NULL) {
		TAILQ_FOREACH(itpc, itpc->itc_header, itc_entry) {
			if(TAILQ_NEXT(itpc, itc_entry)->itc_utpool == utpool) {
				if(itpc->itc_tid == pid) {
					utpool->utp_initcq = FALSE;
					itpc->itc_refcnt--;
					TAILQ_REMOVE(itpc->itc_header, itpc, itc_entry);
				}
			}
		}
	}
}

/* Sender checks request from receiver: providing info */
void
itpc_check_uthreadpool(itpc, pid)
	struct threadpool_itpc *itpc;
	pid_t pid;
{
	struct uthreadpool *utpool = itpc->itc_utpool;

	if(utpool->utp_issender) {
		printf("user threadpool to send");
		if(itpc->itc_tid == pid) {
			printf("user tid be found");
			/* check */
		} else {
			if(itpc->itc_tid != pid) {
				if(utpool->utp_retcnt <= 5) { /* retry up to 5 times */
					if(utpool->utp_initcq) {
						/* exit and re-enter queue, increasing retry count */
						itpc_threadpool_dequeue(itpc);
						utpool->utp_retcnt++;
						itpc_threadpool_enqueue(itpc);
					}
				} else {
					/* exit queue, reset count to 0 and panic */
					itpc_threadpool_dequeue(itpc);
					utpool->utp_retcnt = 0;
					panic("kthreadpool x exited itc queue after 5 failed attempts");
				}
			}
		}
	}
}

/* Receiver verifies request to sender: providing info */
void
itpc_verify_uthreadpool(itpc, pid)
	struct threadpool_itpc *itpc;
	pid_t pid;
{
	struct uthreadpool *utpool = itpc->itc_utpool;

	if(utpool->utp_isreciever) {
		printf("user threadpool to recieve");
		if(itpc->itc_tid == pid) {
			printf("user tid found");
		} else {
			printf("user tid couldn't be found");
		}
	}
}
