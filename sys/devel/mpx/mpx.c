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

void
mpx_init(void)
{
	struct mpx 		*mpx;
	register int 	i, j;

	MALLOC(mpx, struct mpx *, sizeof(struct mpx *), M_MPX, M_WAITOK);
    simple_lock_init(mpx->mpx_pair, "mpx_lock");

    for(i = 0; i <= NGROUPS; i++) {
    	LIST_INIT(&mpx_groups[i]);
    }

	for(j = 0; j <= NCHANS; j++) {
		LIST_INIT(&mpx_channels[j]);
	}
}

struct mpx_writer *
mpx_writer(mpx, size, buffer, state)
	struct mpx *mpx;
	size_t size;
	caddr_t buffer;
	int state;
{
	struct mpx_writer *wmpx;

	MPX_LOCK(mpx->mpx_pair);
	wmpx = mpx->mpx_pair->mpp_wmpx;
	if(wmpx != NULL) {
		wmpx->mpw_chan = mpx->mpx_chan;
		wmpx->mpw_size = size;
		wmpx->mpw_buffer = buffer;
		wmpx->mpw_state = state;
		MPX_UNLOCK(mpx->mpx_pair);
		return (wmpx);
	}
	MPX_UNLOCK(mpx->mpx_pair);
	return (NULL);
}

struct mpx_reader *
mpx_reader(mpx, size, buffer, state)
	struct mpx *mpx;
	size_t size;
	caddr_t buffer;
	int state;
{
	struct mpx_reader *rmpx;

	MPX_LOCK(mpx->mpx_pair);
	rmpx = mpx->mpx_pair->mpp_rmpx;
	if(rmpx != NULL) {
		rmpx->mpr_chan = mpx->mpx_chan;
		rmpx->mpr_size = size;
		rmpx->mpr_buffer = buffer;
		rmpx->mpr_state = state;
		MPX_UNLOCK(mpx->mpx_pair);
		return (rmpx);
	}
	MPX_UNLOCK(mpx->mpx_pair);
	return (NULL);
}

void
mpx_create_group(idx)
    int idx;
{
    struct grouplist *group;
    struct mpx_group *gp;

    group = &mpx_groups[idx];
    gp = (struct mpx_group *)calloc(NGROUPS, sizeof(struct mpx_group *), M_MPX, M_WAITOK);
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

void
mpx_create_channel(gp, idx, flags)
	struct mpx_group *gp;
    int idx, flags;
{
    struct channellist *chan;
    struct mpx_channel *cp;

    chan = &mpx_channels[idx];
    cp = (struct mpx_channel *)calloc(NCHANS, sizeof(struct mpx_chan *), M_MPX, M_WAITOK);
    cp->mpc_index = idx;
    cp->mpc_flags = flags;
    cp->mpc_group = gp;

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
 * creates mpxspace.
 * default: 10 mpx groups with 20 mpx channels per group
 */
void
mpxspace(flags)
	int flags;
{
    register struct mpx_group 	*gp;
    register int i, j;

    for(i = 0; i <= NGROUPS; i++) {
        mpx_create_group(i);
        LIST_FOREACH(gp, &mpx_groups[i], mpg_node) {
            for(j = 0; j <= NCHANS; j++) {
                mpx_create_channel(gp, j, flags);
                gp->mpg_channel = mpx_get_channel(j);
            }
        }
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
	struct mpx_group *gp;
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

/* TODO: add file */
int
mpx_connect(cp1, cp2)
	struct mpx_channel 	*cp1, *cp2;
{
	int i, cnt1, cnt2, total;

	/* check channels used */
	cnt1 = 0;
	cnt2 = 0;
	for(i = 0; i <= NCHANS; i++) {
		cp1 = mpx_get_channel(i);
		cp2 = mpx_get_channel(i);
		if(cp1->mpc_index == i && cp1 == NULL) {
			cnt1++;
		}
		if(cp2->mpc_index == i && cp2 == NULL) {
			cnt2++;
		}
	}
	if(cnt1 < NCHANS && cnt2 < NCHANS) {
		total = (cnt1 + cnt2);
		if (total <= NCHANS) {
			goto pass;
		} else {
			goto fail;
		}
	}

pass:
	/* TODO: merge both channels together */

	return (0);

fail:
	printf("mpx_connect: too many combined channels in use, to complete");
	return (1);
}

mpx_disconnect()
{

}

mpx_join()
{

}

mpx_leave()
{

}

/*
 * Other Routines to consider:
 * mpx_attach, mpx_detach
 */

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
    struct mpx 			*mpx;
    struct mpx_reader 	*read;

    mpx = fp->f_mpx;
    mpx->mpx_file = fp;

    return (0);
}

int
mpx_write(fp, uio, cred)
    struct file *fp;
    struct uio *uio;
    struct ucred *cred;
{
	struct mpx 			*mpx;
	struct mpx_writer 	*write;

	mpx = fp->f_mpx;
	mpx->mpx_file = fp;

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
