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

#include <devel/sys/threadpool.h>
#include "uthread.h"

/* UThreadpool ITPC */

/* Add a thread to the itc queue */
void
itpc_add_uthreadpool(itpc, utpool)
	struct threadpool_itpc *itpc;
	struct uthreadpool *utpool;
{
	struct uthread *ut;

	/* check user threadpool is not null & has a job/task entry to send */
	if(utpool != NULL) {
		itpc->itc_utpool = utpool;
		ut = utpool->utp_overseer.utpt_uthread;
		if(ut != NULL) {
			itpc->itc_utinfo->itc_uthread = ut;
			itpc->itc_utinfo->itc_utid = ut->ut_tid;
			itpc->itc_utinfo->itc_utgrp = ut->ut_pgrp;
			itpc->itc_utinfo->itc_utjob = utpool->utp_overseer.utpt_job;
			itpc->itc_refcnt++;
			utpool->utp_initcq = TRUE;
			TAILQ_INSERT_HEAD(itpc->itc_header, itpc, itc_entry);
		} else {
			printf("no uthread found in uthreadpool");
		}
	} else {
		printf("no uthreadpool found");
	}
}

/*
 * Remove a thread from the itc queue:
 * If threadpool entry is not null, search queue for entry & remove
 */
void
itpc_remove_uthreadpool(itpc, utpool)
	struct threadpool_itpc *itpc;
	struct uthreadpool *utpool;
{
	struct uthread *ut;

	ut = itpc->itc_utinfo->itc_uthread;
	if(utpool != NULL) {
		TAILQ_FOREACH(itpc, itpc->itc_header, itc_entry) {
			if(TAILQ_NEXT(itpc, itc_entry)->itc_utpool == utpool && utpool->utp_overseer.utpt_uthread == ut) {
				if(ut != NULL) {
					itpc->itc_utinfo->itc_uthread = NULL;
					itpc->itc_utinfo->itc_utjob = NULL;
					itpc->itc_refcnt--;
					utpool->utp_initcq = FALSE;
					TAILQ_REMOVE(itpc->itc_header, itpc, itc_entry);
				} else {
					printf("cannot remove uthread. does not exist");
				}
			}
		}
	}
}

/* Sender checks request from receiver: providing info */
void
itpc_check_uthreadpool(itpc, utpool)
	struct threadpool_itpc *itpc;
	struct uthreadpool 	*utpool;
{
	register struct uthread *ut;

	ut = utpool->utp_overseer.utpt_uthread;
	if(utpool->utp_issender) {
		printf("user threadpool to send");
		if(itpc->itc_utpool == utpool) {
			if(itpc->itc_utinfo->itc_uthread == ut) {
				if(itpc->itc_utinfo->itc_utid == ut->ut_tid) {
					printf("uthread tid found");
				} else {
					printf("uthread tid not found");
					goto retry;
				}
			} else {
				printf("uthread not found");
				goto retry;
			}
		}
		panic("no uthreadpool");
	}

retry:
	if (utpool->utp_retcnt <= 5) { /* retry up to 5 times */
		if (utpool->utp_initcq) {
			/* exit and re-enter queue increasing retry count */
			itpc_remove_uthreadpool(itpc, utpool);
			utpool->utp_retcnt++;
			itpc_add_uthreadpool(itpc, utpool);
		} else {
			/* exit queue, reset count to 0 and panic */
			itpc_remove_uthreadpool(itpc, utpool);
			utpool->utp_retcnt = 0;
			panic("uthreadpool x exited itc queue after 5 failed attempts");
		}
	}
}

/* Receiver verifies request to sender: providing info */
void
itpc_verify_uthreadpool(itpc, utpool)
	struct threadpool_itpc *itpc;
	struct uthreadpool *utpool;
{
	register struct uthread *ut;

	ut = utpool->utp_overseer.utpt_uthread;
	if(utpool->utp_isreciever) {
		printf("user threadpool to recieve");
		if(itpc->itc_utpool == utpool) {
			if(itpc->itc_utinfo->itc_uthread == ut) {
				if(itpc->itc_utinfo->itc_utid == ut->ut_tid) {
					printf("uthread tid found");
				} else {
					printf("uthread tid not found");
				}
			} else {
				printf("uthread not found");
			}
		}
		panic("no uthreadpool");
	}
}
