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

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/threadpool.h>
#include <sys/kthread.h>

#include <vm/include/vm_param.h>

/* Threadpool ITPC */
struct threadpool_itpc itpc;

void
itpc_threadpool_init(void)
{
	itpc_threadpool_setup(&itpc);
}

static void
itpc_threadpool_setup(itpc)
	struct threadpool_itpc *itpc;
{
	if(itpc == NULL) {
		MALLOC(itpc, struct threadpool_itpc *, sizeof(struct threadpool_itpc *), M_ITPC, NULL);
	}
	TAILQ_INIT(itpc->itc_header);
	itpc->itc_refcnt = 0;
	itpc->itc_jobs = NULL;
	itpc->itc_job_name = NULL;
}

/* Add kthread to itpc queue */
void
itpc_add_kthreadpool(itpc, ktpool)
    struct threadpool_itpc *itpc;
    struct kthreadpool *ktpool;
{
    struct kthread *kt = NULL;

    if(ktpool != NULL) {
        itpc->itc_ktpool = ktpool;
        kt = ktpool->ktp_overseer.ktpt_kthread;
        if(kt != NULL) {
        	itpc->itc_ktinfo->itc_kthread = kt;
        	itpc->itc_ktinfo->itc_ktid = kt->kt_tid;
        	itpc->itc_ktinfo->itc_ktgrp = kt->kt_pgrp;
        	itpc->itc_ktinfo->itc_ktjob =  ktpool->ktp_overseer.ktpt_job;
            itpc->itc_refcnt++;
            ktpool->ktp_initcq = TRUE;
            TAILQ_INSERT_HEAD(itpc->itc_header, itpc, itc_entry);
        } else {
        	printf("no kthread found in kthreadpool");
        }
    } else {
    	printf("no kthreadpool found");
    }
}

/* Remove kthread from itpc queue */
void
itpc_remove_kthreadpool(itpc, ktpool)
	struct threadpool_itpc *itpc;
	struct kthreadpool *ktpool;
{
	register struct kthread *kt;

	kt = itpc->itc_ktinfo->itc_kthread;
	if (ktpool != NULL) {
		TAILQ_FOREACH(itpc, itpc->itc_header, itc_entry) {
			if (TAILQ_NEXT(itpc, itc_entry)->itc_ktpool == ktpool &&  ktpool->ktp_overseer.ktpt_kthread == kt) {
				if(kt != NULL) {
		        	itpc->itc_ktinfo->itc_kthread = NULL;
		        	itpc->itc_ktinfo->itc_ktjob = NULL;
					itpc->itc_refcnt--;
					ktpool->ktp_initcq = FALSE;
					TAILQ_REMOVE(itpc->itc_header, itpc, itc_entry);
				} else {
					printf("cannot remove kthread. does not exist");
				}
			}
		}
	}
}

/* Sender checks request from receiver: providing info */
void
itpc_check_kthreadpool(itpc, ktpool)
	struct threadpool_itpc *itpc;
	struct kthreadpool *ktpool;
{
	register struct kthread *kt;

	kt = ktpool->ktp_overseer.ktpt_kthread;
	if(ktpool->ktp_issender) {
		printf("kernel threadpool to send");
		if(itpc->itc_ktpool == ktpool) {
			if(itpc->itc_ktinfo->itc_kthread == kt) {
				if(itpc->itc_ktinfo->itc_ktid == kt->kt_tid) {
					printf("kthread tid found");
				} else {
					printf("kthread tid not found");
					goto retry;
				}
			} else {
				printf("kthread not found");
				goto retry;
			}
		}
		panic("no kthreadpool");
	}

retry:
	if(ktpool->ktp_retcnt <= 5) { /* retry up to 5 times */
		if(ktpool->ktp_initcq) {
			/* exit and re-enter queue increasing retry count */
			itpc_remove_kthreadpool(itpc, ktpool);
			ktpool->ktp_retcnt++;
			itpc_add_kthreadpool(itpc, ktpool);
		} else {
			/* exit queue, reset count to 0 and panic */
			itpc_remove_kthreadpool(itpc, ktpool);
			ktpool->ktp_retcnt = 0;
			panic("kthreadpool x exited itc queue after 5 failed attempts");
		}
	}
}

/* Receiver verifies request to sender: providing info */
void
itpc_verify_kthreadpool(itpc, ktpool)
	struct threadpool_itpc *itpc;
	struct kthreadpool *ktpool;
{
	register struct kthread *kt;

	kt = ktpool->ktp_overseer.ktpt_kthread;
	if(ktpool->ktp_isreciever) {
		printf("kernel threadpool to recieve");
		if(itpc->itc_ktpool == ktpool) {
			if(itpc->itc_ktinfo->itc_kthread == kt) {
				if(itpc->itc_ktinfo->itc_ktid == kt->kt_tid) {
					printf("kthread tid found");
				} else {
					printf("kthread tid not found");
				}
			} else {
				printf("kthread not found");
			}
		}
		panic("no kthreadpool");
	}
}
