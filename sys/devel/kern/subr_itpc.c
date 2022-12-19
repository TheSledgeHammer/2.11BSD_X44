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
#include <sys/percpu.h>

#include <vm/include/vm_param.h>

#include <devel/sys/itpc.h>
#include <devel/sys/malloctypes.h>

/* Threadpool ITPC */
void
itpc_threadpool_init(void)
{
	itpc_setup(&itpc);
}

static void
itpc_setup(itpc)
	struct threadpool_itpc *itpc;
{
	MALLOC(itpc, struct threadpool_itpc *, sizeof(struct threadpool_itpc *), M_ITPC, NULL);
	TAILQ_INIT(itpc->itpc_header);
	itpc->itpc_refcnt = 0;
	itpc->itpc_jobs = NULL;
	itpc->itpc_job_name = NULL;
}

struct itpc *
itpc_create(void)
{
	struct itpc *itpc;
	itpc = (struct itpc *)malloc(sizeof(struct itpc *), M_ITPC, M_WAITOK);
	return (itpc);
}

void
itpc_free(struct itpc *itpc)
{
	free(itpc, M_ITPC);
}

void
itpc_add(struct itpc *itpc)
{
	TAILQ_INSERT_HEAD(&itpc_header, itpc, itpc_entry);
}

void
itpc_remove(struct itpc *itpc)
{
	TAILQ_REMOVE(&itpc_header, itpc, itpc_entry);
}

void
itpc_job_init(itpc, job, fmt, ap)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
	const char *fmt;
	va_list ap;
{
	if (job == itpc->itpc_jobs) {
		job->job_itpc = itpc;
		threadpool_job_init(job, job->job_func, job->job_lock, job->job_name, fmt, ap);
	}
}

void
itpc_job_dead(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_dead(job);
	}
}

void
itpc_job_destroy(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_destroy(job);
	}
}

void
itpc_job_hold(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_hold(job);
	}
}

void
itpc_job_rele(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_rele(job);
	}
}

void
itpc_job_done(itpc, job)
	struct threadpool_itpc *itpc;
	struct threadpool_job *job;
{
	if (job == itpc->itpc_jobs) {
		threadpool_job_done(job);
	}
}

struct kthreadpool
itpc_kthreadpool(itpc, ktpool)
	struct threadpool_itpc 	*itpc;
	struct kthreadpool 		*ktpool;
{
	if (itpc->itpc_ktpool == ktpool) {
		return (itpc->itpc_ktpool);
	}
	return (NULL);
}

struct uthreadpool
itpc_uthreadpool(itpc, utpool)
	struct threadpool_itpc 	*itpc;
	struct uthreadpool 		*utpool;
{
	if (itpc->itpc_utpool == utpool) {
		return (itpc->itpc_utpool);
	}
	return (NULL);
}

/* kthread */
/* Add kthread to itpc queue */
void
itpc_add_kthreadpool(itpc, ktpool)
    struct threadpool_itpc *itpc;
    struct kthreadpool *ktpool;
{
    struct kthread *kt = NULL;

    if(ktpool != NULL) {
        itpc->itpc_ktpool = ktpool;
        kt = ktpool->ktp_overseer.ktpt_kthread;
        if(kt != NULL) {
        	itpc->itpc_ktinfo->itpc_kthread = kt;
        	itpc->itpc_ktinfo->itpc_ktid = kt->kt_tid;
        	itpc->itpc_ktinfo->itpc_ktgrp = kt->kt_pgrp;
        	itpc->itpc_ktinfo->itpc_ktjob =  ktpool->ktp_overseer.ktpt_job;
            itpc->itpc_refcnt++;
            ktpool->ktp_initcq = TRUE;
            TAILQ_INSERT_HEAD(itpc->itpc_header, itpc, itpc_entry);
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

	kt = itpc->itpc_ktinfo->itpc_kthread;
	if (ktpool != NULL) {
		TAILQ_FOREACH(itpc, itpc->itpc_header, itpc_entry) {
			if (TAILQ_NEXT(itpc, itpc_entry)->itpc_ktpool == ktpool &&  ktpool->ktp_overseer.ktpt_kthread == kt) {
				if(kt != NULL) {
		        	itpc->itpc_ktinfo->itpc_kthread = NULL;
		        	itpc->itpc_ktinfo->itpc_ktjob = NULL;
					itpc->itpc_refcnt--;
					ktpool->ktp_initcq = FALSE;
					TAILQ_REMOVE(itpc->itpc_header, itpc, itpc_entry);
				} else {
					printf("cannot remove kthread. does not exist");
				}
			}
		}
	} else {
		printf("no kthread to remove");
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
		if(itpc->itpc_ktpool == ktpool) {
			if(itpc->itpc_ktinfo->itpc_kthread == kt) {
				if(itpc->itpc_ktinfo->itpc_ktid == kt->kt_tid) {
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
			panic("kthreadpool x exited itpc queue after 5 failed attempts");
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
		if(itpc->itpc_ktpool == ktpool) {
			if(itpc->itpc_ktinfo->itpc_kthread == kt) {
				if(itpc->itpc_ktinfo->itpc_ktid == kt->kt_tid) {
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

int
itpc_ioctl(fp, cmd, data, p)
	struct file *fp;
	u_long cmd;
	caddr_t data;
	struct proc *p;
{
	struct threadpool_itpc *itpc;
	bool_t issender, isreciever;

	itpc = (struct threadpool_itpc *)fp->f_data;

	switch (cmd) {
	case ITPC_SENDER:
		if(issender) {
			case ITPC_INIT:
			case ITPC_DEAD:
			case ITPC_DESTROY:
			case ITPC_HOLD:
			case ITPC_RELE:
			case ITPC_SCHEDULE:
			case ITPC_CANCEL:
			case ITPC_CANCEL_ASYNC:
		}
		break;
	case ITPC_RECIEVER:
		if(isreciever) {
			case ITPC_INIT:
			case ITPC_DEAD:
			case ITPC_DESTROY:
			case ITPC_HOLD:
			case ITPC_RELE:
			case ITPC_SCHEDULE:
			case ITPC_CANCEL:
			case ITPC_CANCEL_ASYNC:
		}
		break;
	}
	return (0);
}

/* Threadpool's FIFO Queue (IPC) */
void
kthreadpool_itpc_send(itpc, ktpool, pid, cmd)
	struct threadpool_itpc *itpc;
    struct kthreadpool *ktpool;
    pid_t pid;
    int cmd;
{
     /* sync itpc to threadpool */
    itpc->itpc_ktpool = ktpool;
    itpc->itpc_jobs = ktpool->ktp_jobs;  /* add/ get current job */
	/* send flagged jobs */
	ktpool->ktp_issender = TRUE;
	ktpool->ktp_isreciever = FALSE;

	/* command / action */
	switch(cmd) {
	case ITPC_INIT:
		itpc_job_init(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_DEAD:
		itpc_job_dead(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_DESTROY:
		itpc_job_destroy(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_HOLD:
		itpc_job_hold(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_RELE:
		itpc_job_rele(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_SCHEDULE:
		kthreadpool_schedule_job(ktpool, ktpool->ktp_jobs);
		break;

	case ITPC_DONE:
		kthreadpool_job_done(ktpool->ktp_jobs);
		break;

	case ITPC_CANCEL:
		kthreadpool_cancel_job(ktpool, ktpool->ktp_jobs);
		break;

	case ITPC_CANCEL_ASYNC:
		kthreadpool_cancel_job_async(ktpool, ktpool->ktp_jobs);
		break;
	}

	/* update job pool */
	itpc_check_kthreadpool(itpc, pid);
}

void
kthreadpool_itpc_recieve(itpc, ktpool, pid, cmd)
	struct threadpool_itpc *itpc;
    struct kthreadpool *ktpool;
    pid_t pid;
    int cmd;
{
    /* sync itpc to threadpool */
	itpc->itpc_ktpool = ktpool;
	itpc->itpc_jobs = ktpool->ktp_jobs; /* add/ get current job */
	ktpool->ktp_issender = FALSE;
	ktpool->ktp_isreciever = TRUE;

	/* command / action */
	switch(cmd) {
	case ITPC_INIT:
		itpc_job_init(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_DEAD:
		itpc_job_dead(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_DESTROY:
		itpc_job_destroy(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_HOLD:
		itpc_job_hold(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_RELE:
		itpc_job_rele(ktpool->ktp_itpc, ktpool->ktp_jobs);
		break;

	case ITPC_SCHEDULE:
		kthreadpool_schedule_job(ktpool, ktpool->ktp_jobs);
		break;

	case ITPC_DONE:
		kthreadpool_job_done(ktpool->ktp_jobs);
		break;

	case ITPC_CANCEL:
		kthreadpool_cancel_job(ktpool, ktpool->ktp_jobs);
		break;

	case ITPC_CANCEL_ASYNC:
		kthreadpool_cancel_job_async(ktpool, ktpool->ktp_jobs);
		break;
	}

	/* update job pool */
	itpc_verify_kthreadpool(itpc, pid);
}

#include <sys/queue.h>
#include <sys/fnv_hash.h>

struct itpc 			itpc_store;

#define ITPC_HASHMASK 	128
struct itpc_hash_head 	itpc_hashtable[ITPC_HASHMASK];

void
itpc_init(u_long size)
{
	register int	i;

	TAILQ_INIT(&itpc_header);

	for (i = 0; i < ITPC_HASHMASK; i++) {
		TAILQ_INIT(itpc_hashtable[i]);
	}

	itpc_allocate(size, &itpc_store);
}

void
itpc_allocate(size, itpc)
	u_long size;
	register struct itpc *itpc;
{
	SET_IKTHREADPOOL(itpc, NULL);
	SET_IUTHREADPOOL(itpc, NULL);
	TAILQ_INSERT_TAIL(&itpc_header, itpc, itpc_node);
}

struct itpc *
itpc_alloc(size)
	u_long size;
{
	register struct itpc *result;
	result = (struct itpc *)malloc((u_long)sizeof(*result), M_ITPC, M_WAITOK);
	itpc_allocate(size, result);

	return (result);
}

u_long
itpc_hash(itpc)
	struct itpc *itpc;
{
	Fnv32_t hash1 = fnv_32_buf(&itpc, sizeof(&itpc), FNV1_32_INIT) % ITPC_HASHMASK;
	return (hash1);
}

void
itpc_enter(itpc)
	struct itpc 		*itpc;
{
	struct itpc_hash_head 			*bucket;
	register struct itpc_hash_entry *entry;

	if (itpc == NULL) {
		return;
	}

	bucket = &itpc_hashtable[itpc_hash(itpc)];
	entry = (struct itpc_hash_entry *)malloc((u_long)sizeof(*entry), M_ITPC, M_WAITOK);
	entry->itpc = itpc;

	TAILQ_INSERT_TAIL(bucket, entry, itpc_link);
}

void
itpc_remove(itpc)
	struct itpc *itpc;
{
	struct itpc_hash_head 			*bucket;
	register struct itpc_hash_entry *entry;

	bucket = &itpc_hashtable[itpc_hash(itpc)];
	TAILQ_FOREACH(entry, bucket, itpc_link)	{
		itpc = entry->itpc;
		if(itpc != NULL) {
			TAILQ_REMOVE(bucket, entry, itpc_link);
			free((caddr_t)entry, M_ITPC);
			break;
		}
	}
}

struct itpc *
itpc_lookup(itpc)
	struct itpc 					*itpc;
{
	struct itpc_hash_head 			*bucket;
	register struct itpc_hash_entry *entry;

	bucket = &itpc_hashtable[itpc_hash(itpc)];
	TAILQ_FOREACH(entry, bucket, itpc_link)	{
		itpc = entry->itpc;
		if(itpc != NULL) {
			return (itpc);
		}
	}
	return (NULL);
}

void
ithreadpool_add_kthreadpool(itpc, ktpool)
	struct itpc 		*itpc;
	struct kthreadpool 	*ktpool;
{
	SET_IKTHREADPOOL(itpc, ktpool);
	itpc_enter(itpc);
}

void
ithreadpool_add_uthreadpool(itpc, utpool)
	struct itpc 		*itpc;
	struct uthreadpool 	*utpool;
{
	SET_UKTHREADPOOL(itpc, utpool);
	itpc_enter(itpc);
}

/* mpx api's for threads & threadpools */
#include <devel/mpx/mpx.h>

struct itpmx {
	struct mpx				*mpx;

	struct ithread 			*ithread;
	struct ithreadpool 		*ithreadpool;

	struct threadpool_job	*job;
};
