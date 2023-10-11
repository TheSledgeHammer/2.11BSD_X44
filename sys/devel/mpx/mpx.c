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

extern int nchans = NCHANS; /* NCHANS is likely going to be placed in param.c */
struct channellist  mpx_channels[NCHANS];

int							mpxcall(int, int, struct mpx *, int, void *);
static int 					mpxchan(int, int, struct mpx *);
static struct mpx_channel 	*mpx_allocate_channels(struct mpx *);
static void					mpx_deallocate_channels(struct mpx *, struct mpx_channel *);
static void					mpx_add_channel(struct mpx_channel *, int, void *);
static void					mpx_remove_channel(struct mpx_channel *, int, void *);
static void                	mpx_create_channel(struct mpx *, int, void *);
static void					mpx_destroy_channel(struct mpx *, int, void *);
struct mpx_channel     		*mpx_get_channel(int, void *);

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

void
mpx_init(void)
{
	register int i;

	for(j = 0; j < NCHANS; j++) {
		LIST_INIT(&mpx_channels[j]);
	}
}

struct mpx *
mpx_alloc(void)
{
	struct mpx *mpx;
	mpx = (struct mpx *)malloc(sizeof(struct mpx *), M_MPX, M_WAITOK);

	return (mpx);
}

void
mpx_free(mpx)
	struct mpx *mpx;
{
	if(mpx == NULL) {
		return;
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
		create:
		if (mpx->mpx_channel->mpc_refcnt == 0) {
			idx = 0;
		}
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
			if (mpx->mpx_channel->mpc_index == idx
					&& mpx->mpx_channel->mpc_data == data) {
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
		mpx->mpx_channel = mpx_get_channel(idx, data);
		if (mpx->mpx_channel == NULL) {
			printf("get channel: no channel %d found\n", idx);
			return (ENOMEM);
		}
		printf("get channel: %d\n", idx);
		return (0);
	}

	return (0);
}

static struct mpx_channel *
mpx_allocate_channels(mpx)
	struct mpx 	*mpx;
{
	register struct mpx_channel *result;

	result = (struct mpx_channel *)calloc(nchans, sizeof(struct mpx_chan *), M_MPX, M_WAITOK);
    result->mpc_refcnt = 0;
	mpx->mpx_channel = result;
	return (result);
}

static void
mpx_deallocate_channels(mpx, cp)
	struct mpx 	*mpx;
	struct mpx_channel *cp;
{
	if(cp == NULL) {
		return;
	}

	mpx->mpx_channel = mpx_get_channel(cp->mpc_index, cp->mpc_data);
	if(mpx->mpx_channel != cp) {
		return;
	}

	free(cp, M_MPX);
	mpx->mpx_channel = cp;
}

static struct mpx_channel *
mpx_get_channel(idx, data)
    int idx;
    void *data;
{
    struct channellist *chan;
    struct mpx_channel *cp;

    KASSERT(idx < NCHANS);

    chan = &mpx_channels[idx];
    LIST_FOREACH(cp, chan, mpc_node) {
        if(cp->mpc_index == idx && cp->mpc_data == data) {
            return (cp);
        }
    }
    return (NULL);
}

static void
mpx_add_channel(cp, idx, data)
	struct mpx_channel *cp;
	int idx;
	void *data;
{
	struct channellist *chan;

    if (cp == NULL) {
        return;
    }

    chan = &mpx_channels[idx];
    cp->mpc_index = idx;
    cp->mpc_data = data;
    LIST_INSERT_HEAD(chan, cp, mpc_node);
    cp->mpc_refcnt++;
}

static void
mpx_remove_channel(cp, idx, data)
	struct mpx_channel *cp;
	int idx;
    void *data;
{
	cp = mpx_get_channel(idx, data);
    if (cp != NULL) {
        LIST_REMOVE(cp, mpc_node);
        cp->mpc_refcnt--;
    }
}

static void
mpx_create_channel(mpx, idx, data)
	struct mpx 			*mpx;
	int 				idx;
    void                *data;
{
    struct mpx_channel *cp;

    cp = mpx_allocate_channels(mpx);
    MPX_LOCK(mpx);
    mpx_add_channel(cp, idx, data);
    MPX_UNLOCK(mpx);
}

static void
mpx_destroy_channel(mpx, idx, data)
	struct mpx 	*mpx;
	int idx;
    void *data;
{
	struct mpx_channel *cp;

	cp = mpx_get_channel(idx, data);
    if (cp != NULL) {
    	MPX_LOCK(mpx);
        mpx_remove_channel(cp, cp->mpc_index, cp->mpc_data);
        mpx_deallocate_channels(mpx, cp);
        MPX_UNLOCK(mpx);
    }
}
