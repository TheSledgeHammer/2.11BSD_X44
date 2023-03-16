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

/* mpx api's for threads & threadpools */
#include <devel/mpx/mpx.h>

#define M_THREAD 		83
#define M_THREADPOOL 	84

LIST_HEAD(, ithread) allithreads;
struct ithread {
	struct mpx					*impx;

	void 						*ithread;
	int 						ichannel;
	LIST_ENTRY(ithread)  		ientry;
};

LIST_HEAD(, ithreadpool) allithreadpools;
struct ithreadpool {
	struct mpx					*impx;

	void 						*ipool;
	int 						igroup;
	LIST_ENTRY(ithreadpool)  	ientry;
};


int
mxthread_create(func, arg, kt, name)
	void (*func)(void *);
	void *arg;
	struct kthread *kt;
	char *name;
{
	int error;

	error = kthread_create(func, arg, &kt->kt_procp, name);
	if (error != 0) {

	}

	return (error);
}

void
ithread_init(void *thread)
{
	struct ithread *ith;

	ith = (struct ithread *)malloc(sizeof(struct ithread *), M_THREAD, M_NOWAIT);

	LIST_INIT(&allithreads);

	ith->impx = mpx_alloc();
	ithread_insert(ith, thread, 0);
}

struct ithread *
ithread_lookup(void *thread)
{
	struct ithread *ith;
	LIST_FOREACH(ith, &allithreads, ientry) {
		if (ith->ithread == thread) {
			return (ith);
		}
	}
	return (NULL);
}

void
ithread_delete(void *thread)
{
	struct ithread *ith;
	ith = ithread_lookup(thread);
	if(ith != NULL) {
		LIST_REMOVE(ith, ientry);
	}
}

void
ithread_insert(struct ithread *ith, void *thread, int channel)
{
	ith->ithread = thread;
	ith->ichannel = channel;
	LIST_INSERT_HEAD(&allithreads, ith, ientry);
}

/* ithread mpx api's */
int
ithread_create(struct ithread *ith, void *thread)
{
	int channel;

	if (ith == NULL) {
		ith = (struct ithread *)malloc(sizeof(struct ithread *), M_THREAD, M_NOWAIT);
		channel = 0;
	} else {
		channel = ith->ichannel;
	}

	ithread_insert(ith, thread, channel);
	return (mpxthread_create(ith->impx, channel));
}

int
ithread_destroy(struct ithread *ith)
{
	int channel;
	if (ith != NULL) {
		LIST_FOREACH(ith, &allithreads, ientry) {
			channel = ith->ichannel--;
			LIST_REMOVE(ith, ientry);
		}
	}
	return (mpxthread_destroy(ith->impx, channel));
}

int
ithread_put(struct ithread *ith, void *thread)
{
	int channel;

	channel = ith->ichannel++;
	ithread_insert(ith, thread, channel);
	return (mpxthread_put(ith->impx, channel));
}

int
ithread_get(void *thread)
{
	struct ithread *ith;
	int channel;

	ith = ithread_lookup(thread);
	if (ith != NULL) {
		channel = ith->ichannel;
	}
	return (mpxthread_get(ith->impx, channel));
}

int
ithread_remove(void *thread)
{
	struct ithread *ith;
	int channel;

	ith = ithread_lookup(thread);
	if (ith != NULL) {
		channel = ith->ichannel;
		ithread_delete(thread);
	}
	return (mpxthread_remove(ith->impx, channel));
}

/* mpx portion of ithread syscall */
int
mpxthread_create(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXCREATE, MPXCHANNEL, mpx, idx));
}

int
mpxthread_destroy(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXDESTROY, MPXCHANNEL, mpx, idx));
}

int
mpxthread_put(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXPUT, MPXCHANNEL, mpx, idx));
}

int
mpxthread_get(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXGET, MPXCHANNEL, mpx, idx));
}

int
mpxthread_remove(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXREMOVE, MPXCHANNEL, mpx, idx));
}

void
ithreadpool_init()
{
	struct ithreadpool *itp;

	itp = (struct ithreadpool *)malloc(sizeof(struct ithreadpool *), M_THREADPOOL, NULL);

	LIST_INIT(&allithreadpools);
	itp->impx = mpx_alloc();
	ithreadpool_insert(itp, NULL, 0);
}

struct ithreadpool *
ithreadpool_lookup(void *pool, int group)
{
	struct ithreadpool *itp;
	LIST_FOREACH(itp, &allithreadpools, ientry) {
		if (itp->ipool == pool && itp->igroup == group) {
			return (itp);
		}
	}
	return (NULL);
}

void
ithreadpool_delete(void *pool, int group)
{
	struct ithreadpool *itp;
	LIST_FOREACH(itp, &allithreadpools, ientry) {
		if (itp->ipool == pool && itp->igroup == group) {
			LIST_REMOVE(itp, ientry);
		}
	}
}

void
ithreadpool_insert(struct ithreadpool *itp, void *pool, int group)
{
	itp->ipool = pool;
	itp->igroup = group;
	LIST_INSERT_HEAD(&allithreadpools, itp, ientry);
}

/* ithreadpool mpx api's */
int
ithreadpool_create(struct ithreadpool *itp)
{
	int group;

	if (itp == NULL) {
		itp = (struct ithreadpool *)malloc(sizeof(struct ithreadpool *), M_THREADPOOL, NULL);
		group = 0;
	}
	ithreadpool_insert(itp, NULL, group);
	return (mpxthreadpool_create(itp->impx, group));
}

int
ithreadpool_destroy(struct ithreadpool *itp)
{
	int group;

	if (itp != NULL) {
		LIST_FOREACH(itp, &allithreadpools, ientry) {
			group = itp->igroup--;
			LIST_REMOVE(itp, ientry);
		}
	}
	return (mpxthreadpool_destroy(itp->impx, group));
}

int
ithreadpool_put(struct ithreadpool *itp, void *pool)
{
	int group;

	group = itp->igroup++;
	ithreadpool_insert(itp, pool, group);
	return (mpxthreadpool_put(itp->impx, group));
}

int
ithreadpool_get(void *pool)
{
	struct ithreadpool *itp;
	int group;

	itp = ithreadpool_lookup(pool);
	if (itp != NULL) {
		group = itp->igroup;
	}
	return (mpxthreadpool_get(itp->impx, group));
}

int
ithreadpool_remove(void *pool)
{
	struct ithreadpool *itp;
	int group;

	itp = ithreadpool_lookup(pool);
	if (itp != NULL) {
		group = itp->igroup;
		ithreadpool_delete(pool);
	}
	return (mpxthreadpool_remove(itp->impx, group));
}

/* mpx portion of ithreadpool syscall */
int
mpxthreadpool_create(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXCREATE, MPXGROUP, mpx, idx));
}

int
mpxthreadpool_destroy(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXDESTROY, MPXGROUP, mpx, idx));
}

int
mpxthreadpool_put(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXPUT, MPXGROUP, mpx, idx));
}

int
mpxthreadpool_get(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXGET, MPXGROUP, mpx, idx));
}

int
mpxthreadpool_remove(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXREMOVE, MPXGROUP, mpx, idx));
}
