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
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/event.h>
#include <sys/null.h>
#include <sys/user.h>
#include <sys/mpx.h>
#include <sys/sysdecl.h>

int								mpxcall(int, struct mpx *, int, void *);
static int 						mpxchan(int, int, void *, struct mpx *);

/* channel functions */
static struct mpx_channel 		*mpxchan_create(int, u_long, void *, int);
static void						mpxchan_destroy(struct mpx_channel *);
static void						mpxchan_insert_with_size(struct mpx *, int, u_long, void *);
static void						mpxchan_remove(struct mpx *, int, void *);
static struct mpx_channel 		*mpxchan_lookup(struct mpx *, int, void *);

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
	MPX_LOCK_INIT(mpx, "mpx_lock");
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
    int error;

    KASSERT(mpx != NULL);
    KASSERT(data != NULL);

	switch (cmd) {
	case MPXCREATE:
		error = mpx_channel_create(mpx, idx, sizeof(data), data);
		if (error != 0) {
			return (error);
		}
		break;

	case MPXDESTROY:
		error = mpx_channel_destroy(mpx, idx, data);
		if (error != 0) {
			return (error);
		}
		break;

	case MPXPUT:
		error = mpx_channel_put(mpx, idx, sizeof(data), data);
		if (error != 0) {
			return (error);
		}
		break;

	case MPXREMOVE:
		error = mpx_channel_remove(mpx, idx, data);
		if (error != 0) {
			return (error);
		}
		break;

	case MPXGET:
		error = mpx_channel_get(mpx, idx, data);
		if (error != 0) {
			return (error);
		}
		break;
	}

	return (0);
}

int
mpx_channel_create(mpx, idx, size, data)
	struct mpx *mpx;
	int idx;
	u_long size;
	void *data;
{
	if (mpx->mpx_channel->mpc_refcnt == 0) {
		idx = 0;
	}
	mpxchan_insert_with_size(mpx, idx, size, data);
	printf("mpx_channel_create: %d\n", idx);
	return (0);
}

int
mpx_channel_destroy(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
	if (idx >= 0) {
        mpx->mpx_channel = mpxchan_lookup(mpx, idx, data);
        if (mpx->mpx_channel != NULL) {
        	mpxchan_destroy(mpx->mpx_channel);
        }
	}
	printf("mpx_channel_destroy: %d\n", idx);
	return (0);
}

int
mpx_channel_remove(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
	if (mpx->mpx_channel != NULL) {
		mpxchan_remove(mpx, idx, data);
	}
	printf("mpx_channel_remove: %d\n", idx);
	return (0);
}

int
mpx_channel_put(mpx, idx, size, data)
	struct mpx *mpx;
	int idx;
	u_long size;
	void *data;
{
	if (mpx->mpx_channel != NULL) {
		if (mpx->mpx_channel->mpc_refcnt == 0) {
			idx = 0;
		}
		if ((mpx->mpx_channel->mpc_index == idx)
				&& (mpx->mpx_channel->mpc_data == data)) {
			printf("mpx_channel_put: channel %d already exists\n", idx);
			return (EEXIST);
		}
		mpxchan_insert_with_size(mpx, idx, size, data);
	} else {
		printf("mpx_channel_put: channel %d doesn't exist\n", idx);
		return (ENOMEM);
	}
	printf("mpx_channel_put: %d\n", idx);
	return (0);
}

int
mpx_channel_get(mpx, idx, data)
	struct mpx *mpx;
	int idx;
	void *data;
{
	mpx->mpx_channel = mpxchan_lookup(mpx, idx, data);
	if (mpx->mpx_channel == NULL) {
		printf("mpx_channel_get: no channel %d found\n", idx);
		return (ENOMEM);
	}
	printf("mpx_channel_get: %d\n", idx);
	return (0);
}

/* revised mpx channel functions: */
static struct mpx_channel *
mpxchan_create(idx, size, data, nchans)
	int idx, nchans;
	u_long size;
	void *data;
{
	register struct mpx_channel *result;

	result = (struct mpx_channel *)calloc(nchans, size + sizeof(struct mpx_channel *), M_MPX, M_WAITOK);
	bzero(result, sizeof(struct result *));
	result->mpc_index = idx;
	result->mpc_refcnt = 0;
	result->mpc_data = data;
	result->mpc_size = size;
	return (result);
}

static void
mpxchan_destroy(cp)
	struct mpx_channel *cp;
{
	if (cp == NULL) {
		return;
	}
	free(cp, M_MPX);
}

static void
mpxchan_insert_with_size(mpx, idx, size, data)
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
	cp = mpxchan_create(idx, size, data, mpx->mpx_nchans);
	cp->mpc_refcnt++;
	mpx->mpx_channel = cp;
	MPX_LOCK(mpx);
	LIST_INSERT_HEAD(list, cp, mpc_node);
	mpx->mpx_nentries++;
	MPX_UNLOCK(mpx);
}

static void
mpxchan_remove(mpx, idx, data)
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

static struct mpx_channel *
mpxchan_lookup(mpx, idx, data)
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
