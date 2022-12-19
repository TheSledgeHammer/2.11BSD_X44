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

LIST_HEAD(, ithread) allithreads = LIST_HEAD_INITIALIZER(allithreads);
struct ithread {
	struct mpx					*impx;
	void 						*ithread;
	int 						ichannel;
	LIST_ENTRY(ithread)  		ientry;
};

LIST_HEAD(, ithreadpool) allithreadpools = LIST_HEAD_INITIALIZER(allithreadpools);
struct ithreadpool {
	struct mpx					*impx;
	void 						*ipool;
	int 						igroup;
	LIST_ENTRY(ithreadpool)  	ientry;
	struct job_head				ijobs;
};

void
ithread_init()
{
	struct ithread *ith;

	ith = (struct ithread *)malloc(sizeof(struct ithread *), M_THREAD);

	ithread_insert(ith, NULL, 0);
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

void
ithreadpool_init()
{
	struct ithreadpool *itp;

	itp = (struct ithreadpool *)malloc(sizeof(struct ithreadpool *), M_THREADPOOL);
	itp->ijobs = NULL;
	ithreadpool_insert(itp, NULL, 0);
}

struct ithreadpool *
ithreadpool_lookup(void *pool)
{
	struct ithreadpool *itp;
	LIST_FOREACH(itp, &allithreadpools, ientry) {
		if (itp->ipool == pool) {
			return (itp);
		}
	}
	return (NULL);
}

void
ithreadpool_delete(void *pool)
{
	struct ithreadpool *itp;
	itp = ithreadpool_lookup(pool);
	if (itp != NULL) {
		LIST_REMOVE(itp, ientry);
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
		itp = (struct ithreadpool *)malloc(sizeof(struct ithreadpool *), M_THREADPOOL);
		group = 0;
	}
	ithreadpool_insert(itp, NULL, group);
	return (mxthreadpool_create(itp->impx, group));
}

int
ithreadpool_destroy(struct ithreadpool *itp)
{
	int group;

	if (itp != NULL) {
		LIST_FOREACH(itp, &allithreads, ientry) {
			group = itp->igroup--;
			LIST_REMOVE(itp, ientry);
		}
	}
	return (mxthreadpool_destroy(itp->impx, group));
}

int
ithreadpool_put(struct ithreadpool *itp, void *pool)
{
	int group;

	group = itp->igroup++;
	ithreadpool_insert(itp, pool, group);
	return (mxthreadpool_put(itp->impx, group));
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
	return (mxthreadpool_get(itp->impx, group));
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
	return (mxthreadpool_remove(itp->impx, group));
}

/* ithread mpx api's */
int
ithread_create(struct ithread *ith)
{
	int channel;

	if (ith == NULL) {
		ith = (struct ithread *)malloc(sizeof(struct ithread *), M_THREAD);
		channel = 0;
	}
	ithread_insert(ith, NULL, channel);

	return (mxthread_create(ith->impx, channel));
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
	return (mxthread_destroy(ith->impx, channel));
}

int
ithread_put(struct ithread *ith, void *thread)
{
	int channel;

	channel = ith->ichannel++;
	ithread_insert(ith, thread, channel);
	return (mxthread_put(ith->impx, channel));
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
	return (mxthread_get(ith->impx, channel));
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
	return (mxthread_remove(ith->impx, channel));
}

/* mpx portion of ithread syscall */
int
mxthread_create(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXCREATE, MPXCHANNEL, mpx, idx));
}

int
mxthread_destroy(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXDESTROY, MPXCHANNEL, mpx, idx));
}

int
mxthread_put(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXPUT, MPXCHANNEL, mpx, idx));
}

int
mxthread_get(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXGET, MPXCHANNEL, mpx, idx));
}

int
mxthread_remove(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXREMOVE, MPXCHANNEL, mpx, idx));
}

/* mpx portion of ithreadpool syscall */
int
mxthreadpool_create(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXCREATE, MPXGROUP, mpx, idx));
}

int
mxthreadpool_destroy(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXDESTROY, MPXGROUP, mpx, idx));
}

int
mxthreadpool_put(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXPUT, MPXGROUP, mpx, idx));
}

int
mxthreadpool_get(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXGET, MPXGROUP, mpx, idx));
}

int
mxthreadpool_remove(mpx, idx)
	struct mpx *mpx;
	int idx;
{
	return (mpxcall(MPXREMOVE, MPXGROUP, mpx, idx));
}
