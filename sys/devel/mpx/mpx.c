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
/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (c) 1996 John S. Dyson
 * Copyright (c) 2012 Giovanni Trematerra
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Absolutely no warranty of function or purpose is made by the author
 *    John S. Dyson.
 * 4. Modifications may be freely made to this file if the above conditions
 *    are met.
 */

#include <sys/cdefs.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/lock.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/null.h>

#include <devel/mpx/mpx.h>
#include <devel/sys/malloctypes.h>

struct fileops mpxops = {
		.fo_rw = mpx_rw,
		.fo_read = mpx_read,
		.fo_write = mpx_write,
		.fo_ioctl = mpx_ioctl,
		.fo_poll = mpx_poll,
		.fo_close = mpx_close,
		.fo_kqfilter = mpx_kqfilter
};

struct grouplist   	mpx_groups[NGROUPS];
struct channellist  mpx_channels[NCHANS];

static long amountmpxkva;
static int mpxfragretry;
static int mpxallocfail;
static int mpxresizefail;
static int mpxresizeallowed = 1;
static long mpx_mindirect = MPX_MINDIRECT;

static int mpx_paircreate(struct proc *, struct mpxpair **);
static int mpxspace_new(struct mpx *, int);
static int mpxspace(struct mpx *, int);
static int mpx_create(struct mpx *, bool_t);

void
mpx_init(void)
{
	register struct mpx *mpx;
	register int 	i, j;

	mpx = (struct mpx *)malloc(sizeof(struct mpx *), M_MPX, M_WAITOK);

	MPX_LOCK_INIT(mpx->mpx_pair);

    for(i = 0; i <= NGROUPS; i++) {
    	LIST_INIT(&mpx_groups[i]);
    }

	for(j = 0; j <= NCHANS; j++) {
		LIST_INIT(&mpx_channels[j]);
	}
}

static int
mpx_paircreate(p, p_mpp)
	struct proc 	*p;
	struct mpxpair 	**p_mpp;
{
	struct mpxpair *mpp;
	struct mpx *wmpx, *rmpx;
	int error;

	*p_mpp = mpp = (struct mpxpair *)malloc(sizeof(struct mpxpair *), M_MPX, M_WAITOK);

	rmpx = &mpp->mpp_rmpx;
	wmpx = &mpp->mpp_wmpx;

	error = mpx_create(rmpx, TRUE);
	if (error != 0) {
		goto fail;
	}

	error = mpx_create(wmpx, FALSE);
	if (error != 0) {
		goto fail;
	}

	rmpx->mpx_state |= MPX_DIRECTOK;
	wmpx->mpx_state |= MPX_DIRECTOK;
	return (0);

fail:

	free(mpp, M_MPX);
	return (error);
}

static int
mpxspace_new(cmpx, size)
	struct mpx *cmpx;
	int 		size;
{
	caddr_t buffer;
	int error, cnt, firstseg;
	static int curfail = 0;
	static struct timeval lastfail;

retry:
	cnt = cmpx->mpx_buffer.cnt;
	if (cnt > size) {
		size = cnt;
	}

	size = round_page(size);
	buffer = (caddr_t) vm_map_min(mpx_map);

	error = vm_map_find(mpx_map, NULL, 0, (vm_offset_t *)&buffer, size, 0, FALSE);
	if (error != KERN_SUCCESS) {
		if (cmpx->mpx_buffer.buffer == NULL && size > SMALL_MPX_SIZE) {
			size = SMALL_MPX_SIZE;
			mpxfragretry++;
			goto retry;
		}
		if (cmpx->mpx_buffer.buffer == NULL) {
			mpxallocfail++;
		} else {
			mpxresizefail++;
		}
		return (ENOMEM);
	}

	/* copy data, then free old resources if we're resizing */
	if (cnt > 0) {
		if (cmpx->mpx_buffer.in <= cmpx->mpx_buffer.out) {
			firstseg = cmpx->mpx_buffer.size - cmpx->mpx_buffer.out;
			bcopy(&cmpx->mpx_buffer.buffer[cmpx->mpx_buffer.out], buffer, firstseg);
			if ((cnt - firstseg) > 0) {
				bcopy(cmpx->mpx_buffer.buffer, &buffer[firstseg],
						cmpx->mpx_buffer.in);
			}
		} else {
			bcopy(&cmpx->mpx_buffer.buffer[cmpx->mpx_buffer.out], buffer, cnt);
		}
	}

	cmpx->mpx_buffer.buffer = buffer;
	cmpx->mpx_buffer.size = size;
	cmpx->mpx_buffer.in = cnt;
	cmpx->mpx_buffer.out = 0;
	cmpx->mpx_buffer.cnt = cnt;

	return (0);
}

static int
mpxspace(cmpx, size)
	struct mpx *cmpx;
	int 		size;
{
    KASSERT(cmpx->mpx_state & MPX_LOCKFL);
    return (mpxspace_new(cmpx, size));
}

static int
mpx_create(mpx, large_backing)
	struct mpx 	*mpx;
	bool_t  	large_backing;
{
	int error;

	error = mpxspace_new(mpx,
			!large_backing || amountmpxkva > maxmpxkva / 2 ?
					SMALL_MPX_SIZE : MPX_SIZE);

	return (error);
}

struct mpx_group *
mpx_allocate_groups(mpx, ngroups)
	struct mpx 	*mpx;
	int 		ngroups;
{
	register struct mpx_group *result;
	result = (struct mpx_group *)calloc(ngroups, sizeof(struct mpx_group *), M_MPX, M_WAITOK);
	mpx->mpx_group = result;

	return (result);
}

void
mpx_create_group(mpx, idx)
	struct mpx 	*mpx;
	int idx;
{
	struct grouplist *group;
	struct mpx_group *gp;

	group = &mpx_groups[idx];
	gp = mpx_allocate_groups(mpx, NGROUPS);
    gp->mpg_index = idx;

	LIST_INSERT_HEAD(group, gp, mpg_node);
}

struct mpx_group *
mpx_get_group(idx)
    int idx;
{
    struct grouplist *group;
    struct mpx_group *gp;

    group = &mpx_groups[idx];
    LIST_FOREACH(gp, group, mpg_node) {
        if(gp->mpg_index == idx) {
            return (gp);
        }
    }
    return (NULL);
}

void
mpx_remove_group(gp, idx)
	struct mpx_group *gp;
	int idx;
{
	if(gp->mpg_index == idx) {
		LIST_REMOVE(gp, mpg_node);
	} else {
		gp = mpx_get_group(idx);
		LIST_REMOVE(gp, mpg_node);
	}
}

struct mpx_channel *
mpx_allocate_channels(mpx, nchans)
	struct mpx 	*mpx;
	int 		nchans;
{
	register struct mpx_channel *result;
	result = (struct mpx_channel *)calloc(nchans, sizeof(struct mpx_chan *), M_MPX, M_WAITOK);
	mpx->mpx_channel = result;

	return (result);
}

void
mpx_create_channel(mpx, gp, idx, flags)
	struct mpx 			*mpx;
	struct mpx_group 	*gp;
	int 				idx, flags;
{
    struct channellist *chan;
    struct mpx_channel *cp;

    chan = &mpx_channels[idx];
    cp = mpx_allocate_channels(mpx, NCHANS);
    cp->mpc_group = gp;
    cp->mpc_index = idx;
    cp->mpc_flags = flags;

    LIST_INSERT_HEAD(chan, cp, mpc_node);
}

struct mpx_channel *
mpx_get_channel(idx)
    int idx;
{
    struct channellist *chan;
    struct mpx_channel *cp;

    chan = &mpx_channels[idx];
    LIST_FOREACH(cp, chan, mpc_node) {
        if(cp->mpc_index == idx) {
            return (cp);
        }
    }
    return (NULL);
}

void
mpx_remove_channel(cp, idx)
	struct mpx_channel *cp;
	int idx;
{
	if (cp->mpc_index == idx) {
		LIST_REMOVE(cp, mpc_node);
	} else {
		cp = mpx_get_channel(idx);
		LIST_REMOVE(cp, mpc_node);
	}
}

/*
 * Attach an existing channel to an existing group, if channels are free
 * within that group.
 * Note: This does not add a new channel to the channels list.
 */
void
mpx_attach(cp, gp)
	struct mpx_channel 	*cp;
	struct mpx_group 	*gp;
{
	register int i;

	KASSERT(cp != NULL);
	for (i = 0; i <= NGROUPS; i++) {
		 LIST_FOREACH(gp, &mpx_groups[i], mpg_node) {
			 if (gp->mpg_channel == NULL) {
				 gp->mpg_channel = cp;
			 }
		 }
	}
}

/*
 * Detach an existing channel (if not empty) from an existing group.
 * Note: This does not remove channel from the channels list.
 */
void
mpx_detach(cp, gp)
	struct mpx_channel 	*cp;
	struct mpx_group 	*gp;
{
	register int i;

	for (i = 0; i <= NGROUPS; i++) {
		LIST_FOREACH(gp, &mpx_groups[i], mpg_node) {
			 if (gp->mpg_channel == cp && cp != NULL) {
				 gp->mpg_channel = NULL;
			 }
		}
	}
}

/* merges two channels into one and removing both original channels */
struct mpx_channel *
mpx_merge_channels(cp1, cp2)
    struct mpx_channel *cp1, *cp2;
{
    struct mpx_channel *cp3;
    int i;
    for(i = 0; i < NCHANS; i++) {
        LIST_FOREACH(cp1, &mpx_channels[i], mpc_node) {
            LIST_FOREACH(cp2, &mpx_channels[i], mpc_node) {
                if(cp1 != NULL && cp2 != NULL) {
                    cp3 = mpx_get_channel(i);
                    mpx_remove_channel(cp1, i);
                    mpx_remove_channel(cp2, i);
                }
            }
        }
        if (cp3 != NULL) {
            LIST_INSERT_HEAD(&mpx_channels[i], cp3, mpc_node);
            break;
        }
    }
    return (cp3);
}

/* TODO: add file */
int
mpx_connect(cp1, cp2)
	struct mpx_channel 	*cp1, *cp2;
{
	struct mpx_channel 	*cp3;
	int i, cnt, total;

	/* check channels used */
	cnt = 0;
	for(i = 0; i < NCHANS; i++) {
		cp1 = mpx_get_channel(i);
		cp2 = mpx_get_channel(i);
		if (mpx_compare_channels(cp1, cp2) != 0) {
			if((cp1->mpc_index == i && cp1 != NULL) || (cp2->mpc_index == i && cp2 != NULL)) {
				cnt++;
			}
		} else {
			cnt++;
		}
	}

	/* merge channels */
	total = cnt;
	if (total < NCHANS) {
		cp3 = mpx_merge_channels(cp1, cp2);
		if(cp3) {
			return (0);
		}
	}
	return (1);
}

/*
 * splits a single channel into two channels, depending on the channel
 * index specified. Whilst removing the specified channel at index from the original channel.
 */
struct mpx_channel *
mpx_split_channels(cp, idx)
    struct mpx_channel *cp;
    int idx;
{
    struct mpx_channel *other;
    int i;

    for(i = 0; i < NCHANS; i++) {
        LIST_FOREACH(cp, &mpx_channels[i], mpc_node) {
        	other = mpx_get_channel(i);
        }
        if(other != NULL) {
            if(idx == i) {
                LIST_INSERT_HEAD(&mpx_channels[idx], other, mpc_node);
                mpx_remove_channel(cp, idx);
            } else {
                LIST_REMOVE(other, mpc_node);
            }
            break;
        }
    }
    return (other);
}

/*
 * Similar to mpx_split_channels but takes multiple elements to form a new channel
 * with those elements
 */
struct mpx_channel *
mpx_disconnect(cp, nchans)
	struct mpx_channel *cp;
	int nchans;
{
	struct mpx_channel *cp1;
	int i;

	KASSERT(nchans >= 1);
	for (i = 0; i < nchans; i++) {
		LIST_FOREACH(cp, &mpx_channels[i], mpc_node) {
			cp1 = mpx_split_channels(cp, i);
		}
		break;
	}
	return (cp1);
}

mpx_join()
{

}

mpx_leave()
{

}

int
mpx_rw(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
	int error;

	return (error);
}

int
mpx_read(fp, uio, cred)
    struct file *fp;
    struct uio *uio;
    struct ucred *cred;
{
    struct mpx 		*rmpx;
    struct mpxbuf 	*bp;
    int error;

    rmpx = fp->f_mpx;
    bp = &rmpx->mpx_buffer;
    rmpx->mpx_file = fp;


    error = mpxread(mpx, state);
    return (0);
}

int
mpx_write(fp, uio, cred)
    struct file *fp;
    struct uio *uio;
    struct ucred *cred;
{
	struct mpx 		*wmpx;
	struct mpxbuf 	*bp;
	int error;

	wmpx = fp->f_mpx;
	bp = &wmpx->mpx_buffer;
	wmpx->mpx_file = fp;


	error = mpxwrite(mpx, state);
	return (0);
}

int
mpx_ioctl(fp, cmd, data, p)
	struct file *fp;
	u_long cmd;
	caddr_t data;
	struct proc *p;
{
	return (0);
}

int
mpx_poll(fp, event, p)
	struct file *fp;
	int event;
	struct proc *p;
{
	return (0);
}

int
mpx_close(fp, p)
	struct file *fp;
	struct proc *p;
{
	return (0);
}

int
mpx_kqfilter(fp, kn)
	struct file *fp;
	struct knote *kn;
{
	return (0);
}

int
mpx_compare_groups(gp1, gp2)
	struct mpx_group *gp1, *gp2;
{
	if(gp1->mpg_index < gp2->mpg_index) {
		return (-1);
	} else if(gp1->mpg_index > gp2->mpg_index) {
		return (1);
	} else {
		return (0);
	}
}

int
mpx_compare_channels(cp1, cp2)
	struct mpx_channel 	*cp1, *cp2;
{
	if(cp1->mpc_index < cp2->mpc_index) {
		return (-1);
	} else if(cp1->mpc_index > cp2->mpc_index) {
		return (1);
	} else {
		return (0);
	}
}
