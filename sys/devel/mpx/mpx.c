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

/* mpx groups have not been implemented */

#include <sys/cdefs.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/lock.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/event.h>
#include <sys/poll.h>
#include <sys/null.h>
#include <sys/user.h>

#include <devel/mpx/mpx.h>
#include <devel/sys/malloctypes.h>

int							mpxcall(int, int, struct mpx *, int, void *);
static int 					mpxchan(int, int, struct mpx *);
#ifdef notyet
static struct mpx_channel 	*mpx_allocate_channels(struct mpx *, int, void *);
static void					mpx_deallocate_channels(struct mpx *, struct mpx_channel *);
#endif
static void					mpx_add_channel(struct mpx *, int, void *);
static void					mpx_remove_channel(struct mpx *, int, void *);
static void                	mpx_create_channel(struct mpx *, int, void *);
static void					mpx_destroy_channel(struct mpx *, int, void *);
struct mpx_channel     		*mpx_get_channel(struct mpx *, int, void *);

/* MPX Changes:
 * - improve multi-processing support
 * - NCHANS is no longer a global parameter, but a local one.
 * 		- This means that each mpx structure allocated can set the
 * 		maximum number of channels available for that instance.
 */

int
mpx_create(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
	int error;

	error = mpxcall(MPXCREATE, mpx, idx, data);
	if (error != 0) {
		return (error);
	}
	return (0);
}

int
mpx_put(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
	int error;

	error = mpxcall(MPXPUT, mpx, idx, data);
	if (error != 0) {
		return (error);
	}
	return (0);
}

int
mpx_get(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
	int error;

	error = mpxcall(MPXGET, mpx, idx, data);
	if (error != 0) {
		return (error);
	}
	return (0);
}

int
mpx_destroy(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
	int error;

	error = mpxcall(MPXDESTROY, mpx, idx, data);
	if (error != 0) {
		return (error);
	}

	return (0);
}

int
mpx_remove(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
	int error;

	error = mpxcall(MPXREMOVE, mpx, idx, data);
	if (error != 0) {
		return (error);
	}
	return (0);
}

static void
mpx_init(mpx, nchans)
	struct mpx 	*mpx;
	int nchans;
{
	int i;

	for(i = 0; i < nchans; i++) {
		LIST_INIT(&mpx->mpx_chanlist[i]);
	}
	mpx->mpx_nentries = 0;
	mpx->mpx_refcnt = 1;
	mpx->mpx_nchans = nchans;
	simple_lock_init(&mpx->mpx_lock, "mpx_lock");
}

struct mpx *
mpx_allocate(nchans)
	int nchans;
{
	struct mpx *result;
	result = (struct mpx *)malloc(sizeof(struct mpx *), M_MPX, M_WAITOK);
	mpx_init(result, nchans);
	return (result);
}

void
mpx_deallocate(mpx)
	struct mpx *mpx;
{
	int i;

	if (mpx == NULL) {
		return;
	}
	for(i = 0; i < mpx->mpx_nchans; i++) {
		if (!LIST_EMPTY(&mpx->mpx_chanlist[i])) {
			return;
		}
	}
	free(mpx, M_MPX);
}

static int
mpx_copyin(mpx)
	struct mpx *mpx;
{
	struct mpx mx;
	int error;

	error = copyin(mpx, &mx, sizeof(struct mpx));
	return (error);
}

static int
mpx_copyout(mpx)
	struct mpx *mpx;
{
	struct mpx mx;
	int error;

	error = copyout(mpx, &mx, sizeof(struct mpx));
	return (error);
}

/* mpx callback */
int
mpxcall(cmd, mpx, idx, data)
	int cmd, idx;
	struct mpx *mpx;
	void *data;
{
	int error;

	error = mpx_copyin(mpx);
	if (error) {
		error = mpxchan(cmd, idx, data, mpx);
	}

	if (error != 0) {
		return (error);
	}

	error = mpx_copyout(mpx);
	return (error);
}

/* mpx syscall */
int
mpx()
{
	register struct mpx_args {
		syscallarg(int) cmd;
		syscallarg(struct mpx *) mpx;
		syscallarg(int) idx; /* channel or group index */
		syscallarg(void *) data;
	} *uap = (struct mpx_args *)u.u_ap;

	int error;

	error = mpxcall(SCARG(uap, cmd), SCARG(uap, mpx), SCARG(uap, idx), SCARG(uap, data));
	return (error);
}

static int
mpxchan(cmd, idx, data, mpx)
	int cmd, idx;
	struct mpx *mpx;
    void *data;
{
    KASSERT(mpx != NULL);
    KASSERT(data != NULL);

	switch (cmd) {
	case MPXCREATE:
	/*
		if (mpx->mpx_channel->mpc_refcnt == 0) {
			idx = 0;
		}
	*/
		mpx_create_channel(mpx, idx, data);
		printf("create channel: %d\n", idx);
		return (0);

	case MPXDESTROY:
		if (idx >= 0) {
			mpx_destroy_channel(mpx, idx, data);
		}
		printf("destroy channel: %d\n", idx);
		return (0);

	case MPXPUT:
		if (mpx->mpx_channel != NULL) {
			if (mpx->mpx_channel->mpc_refcnt == 0) {
				idx = 0;
			}
			if ((mpx->mpx_channel->mpc_index == idx) && (mpx->mpx_channel->mpc_data == data)) {
				printf("add channel: channel %d already exists\n", idx);
				return (EEXIST);
			}
			mpx_add_channel(mpx->mpx_channel, idx, data);
		} else {
			printf("add channel: channel %d doesn't exist\n", idx);
			return (ENOMEM);
		}
		printf("add channel: %d\n", idx);
		return (0);

	case MPXREMOVE:
		if (mpx->mpx_channel != NULL) {
			mpx_remove_channel(mpx->mpx_channel, idx, data);
		}
		printf("remove channel: %d\n", idx);
		return (0);

	case MPXGET:
		mpx->mpx_channel = mpx_get_channel(mpx, idx, data);
		if (mpx->mpx_channel == NULL) {
			printf("get channel: no channel %d found\n", idx);
			return (ENOMEM);
		}
		printf("get channel: %d\n", idx);
		return (0);
	}

	return (0);
}

/* revised mpx functions: */
struct mpx_channel *
mpx_channel_create(idx, size, data, nchans)
	int idx, nchans;
	u_long size;
	void *data;
{
	register struct mpx_channel *result;
	long totsize;

	totsize = (data * size);
	result = (struct mpx_channel *)calloc(nchans, totsize + sizeof(struct mpx_channel *), M_MPX, M_WAITOK);
	bzero(result, sizeof(struct result));
	result->mpc_index = idx;
	//result->mpc_refcnt = 0;
	result->mpc_data = data;
	result->mpc_size = size;
	return (result);
}

void
mpx_channel_destroy(cp)
	struct mpx_channel *cp;
{
	if (cp == NULL) {
		return;
	}
	free(cp, M_MPX);
}

void
mpx_channel_insert_with_size(mpx, idx, size, data)
	struct mpx *mpx;
	int idx;
	u_long size;
	void *data;
{
	struct channellist *list;
	struct mpx_channel *cp;

	if (mpx == NULL) {
		return;
	}

	list = &mpx->mpx_chanlist[idx];
	cp = channel_create(idx, size, data, mpx->mpx_nchans);
	cp->mpc_refcnt++;
	mpx->mpx_channel = cp;
	MPX_LOCK(mpx);
	LIST_INSERT_HEAD(list, cp, mpc_node);
	mpx->mpx_nentries++;
	MPX_UNLOCK(mpx);
}

void
mpx_channel_insert(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
	mpx_channel_insert_with_size(mpx, idx, sizeof(data), data);
}

void
mpx_channel_remove(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
	struct channellist *list;
	struct mpx_channel *cp;

	list = &mpx->mpx_chanlist[idx];
	MPX_LOCK(mpx);
	LIST_FOREACH(cp, list, mpc_node) {
		if ((cp->mpc_index == idx) && (cp->mpc_data == data)) {
			cp->mpc_refcnt--;
			LIST_REMOVE(cp, mpc_node);
			mpx->mpx_nentries--;
			MPX_UNLOCK(mpx);
		}
	}
}

struct mpx_channel *
mpx_channel_lookup(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
	struct channellist *list;
	struct mpx_channel *cp;

	list = &mpx->mpx_chanlist[idx];
	MPX_LOCK(mpx);
	LIST_FOREACH(cp, list, mpc_node) {
		if ((cp->mpc_index == idx) && (cp->mpc_data == data)) {
			MPX_UNLOCK(mpx);
			return (cp);
		}
	}
	MPX_UNLOCK(mpx);
	return (NULL);
}

/* old functions */
#ifdef notyet
static struct mpx_channel *
mpx_allocate_channels(mpx, idx, data)
	struct mpx 	*mpx;
	int idx;
	void *data;
{
	return (mpx_channel_create(idx, sizeof(data), data), mpx->mpx_nchans);
}

static void
mpx_deallocate_channels(mpx, cp)
	struct mpx 	*mpx;
	struct mpx_channel *cp;
{
	mpx_destroy_channel(mpx, cp->mpc_index, cp->mpc_data);
}
#endif

static struct mpx_channel *
mpx_get_channel(mpx, idx, data)
	struct mpx 	*mpx;
    int idx;
    void *data;
{
    return (mpx_channel_lookup(mpx, idx, data));
}

static void
mpx_add_channel(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
    mpx_channel_insert(mpx, idx, data);
}

static void
mpx_remove_channel(mpx, idx, data)
	struct mpx *mpx;
	int idx;
    void *data;
{
    mpx_channel_remove(mpx, idx, data);
}

static void
mpx_create_channel(mpx, idx, data)
	struct mpx 			*mpx;
	int 				idx;
    void                *data;
{
    mpx_add_channel(mpx, idx, data);
}

static void
mpx_destroy_channel(mpx, idx, data)
	struct mpx 	*mpx;
	int idx;
    void *data;
{
	struct mpx_channel *cp;

	cp = mpx_channel_lookup(mpx, idx, data);
    if (cp != NULL) {
    	mpx_channel_destroy(cp);
    }
}
