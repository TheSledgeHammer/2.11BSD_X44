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

#include <multitasking/userthreads/uthread.h>

/* User Threadpool */

TAILQ_HEAD(uthreadpool, user_threadpool) utpool;
struct user_threadpool {
	struct uthreads						*utp_uthread;		/* user threads */
	TAILQ_ENTRY(user_threadpool) 		utp_entry;			/* list uthread entries */
    struct itc_thread					utp_itc;			/* threadpool ipc ptr */

    boolean_t							utp_issender;		/* sender */
    boolean_t							utp_isreciever;		/* reciever */

    int 								utp_refcnt;			/* total thread count in pool */
    int 								utp_active;			/* active thread count */
    int									utp_inactive;		/* inactive thread count */


    struct job_head						utp_jobs;
    //flags, refcnt, priority, lock
    //flags: send, (verify: success, fail), retry_attempts
};

extern void uthreadpool_send(struct user_threadpool *, struct itc_threadpool *);
extern void uthreadpool_receive(struct user_threadpool *, struct itc_threadpool *);
