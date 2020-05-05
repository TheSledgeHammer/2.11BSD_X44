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
#include <multitasking/kernthreads/kthread.h>

/* Kernel Threads in Kernel Threadpool */

/* Kernel Threadpool */

TAILQ_HEAD(kthreadpool, kern_threadpool) ktpool;
struct kern_threadpool {
	struct kthreads						*ktp_kthread;		/* kernel threads */
	TAILQ_ENTRY(kern_threadpool) 		ktp_entry;			/* list kthread entries */
    struct itc_thread					ktp_itc;			/* threadpool ipc ptr */

    boolean_t							ktp_issender;		/* sender */
    boolean_t							ktp_isreciever;		/* reciever */

    int 								ktp_refcnt;			/* total thread count in pool */
    int 								ktp_active;			/* active thread count */
    int									ktp_inactive;		/* inactive thread count */


    struct job_head						ktp_jobs;

    //flags, refcnt, priority, lock, verify
    //flags: send, (verify: success, fail), retry_attempts
};


enqueue(); 		/* add to kthread threadpool */
dequeue();		/* remove from kthread threadpool */

/* ITC Functions */
extern void kthreadpool_send(struct kern_threadpool *, struct itc_threadpool *);
extern void kthreadpool_receive(struct kern_threadpool *, struct itc_threadpool *);
