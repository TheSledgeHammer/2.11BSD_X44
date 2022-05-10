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
	register struct mpx *mpx;
	register int 	i, j;

	mpx = (struct mpx *)malloc(sizeof(struct mpx *), M_MPX, M_WAITOK);
	mpxpair(mpx) = (union mpxpair *)malloc(sizeof(union mpxpair *), M_MPX, M_WAITOK);

	MPX_LOCK_INIT(mpxpair(mpx));

    for(i = 0; i <= NGROUPS; i++) {
    	LIST_INIT(&mpx_groups[i]);
    }

	for(j = 0; j <= NCHANS; j++) {
		LIST_INIT(&mpx_channels[j]);
	}
}

int
mpx_count_groups(gp)
	struct mpx_group 	*gp;
{
	int i, count;
	count = 0;
	for(i = 0; i < NGROUPS; i++) {
		gp = mpx_get_group(i);
		if(gp->mpg_index == i && gp != NULL) {
			count++;
		}
	}
	return (count);
}

int
mpx_count_channels(cp)
	struct mpx_channel 	*cp;
{
	int i, count;
	count = 0;
	for(i = 0; i < NCHANS; i++) {
		cp = mpx_get_channel(i);
		if(cp->mpc_index == i && cp != NULL) {
			count++;
		}
	}
	return (count);
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

struct mpx_writer *
mpx_writer(mpx, state)
	struct mpx *mpx;
	int state;
{
	register struct mpx_writer 	*wmpx;

	MPX_LOCK(mpxpair(mpx));
	wmpx = mpxpair(mpx)->mpp_wmpx;
	if(wmpx != NULL) {
		wmpx->mpw_group = mpx->mpx_group;
		wmpx->mpw_channel = mpx->mpx_channel;
		wmpx->mpw_size = MPX_WRITER_SIZE;
		wmpx->mpw_buffer = MPX_BUFFER(MPX_WRITER_SIZE);
		wmpx->mpw_state = state;
		MPX_UNLOCK(mpxpair(mpx));
		return (wmpx);
	}
	MPX_UNLOCK(mpxpair(mpx));
	return (NULL);
}

struct mpx_reader *
mpx_reader(mpx, state)
	struct mpx *mpx;
	int state;
{
	register struct mpx_reader *rmpx;

	MPX_LOCK(mpxpair(mpx));
	rmpx = mpxpair(mpx)->mpp_rmpx;
	if(rmpx != NULL) {
		rmpx->mpr_group = mpx->mpx_group;
		rmpx->mpr_channel = mpx->mpx_channel;
		rmpx->mpr_size = MPX_READER_SIZE;
		rmpx->mpr_buffer = MPX_BUFFER(MPX_READER_SIZE);
		rmpx->mpr_state = state;
		MPX_UNLOCK(mpxpair(mpx));
		return (rmpx);
	}
	MPX_UNLOCK(mpxpair(mpx));
	return (NULL);
}

int
mpxread(mpx, state)
	struct mpx 		*mpx;
	int 			state;
{
	register struct mpx_reader 	*rmpx;

	rmpx = mpx_reader(mpx, state);
	if(rmpx == NULL) {
		return (1);
	}
	return (0);
}

int
mpxwrite(mpx, state)
	struct mpx 		*mpx;
	int 			state;
{
	register struct mpx_writer 	*wmpx;

	wmpx = mpx_writer(mpx, state);
	if(wmpx == NULL) {
		return (1);
	}
	return (0);
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
	int i, cnt1, cnt2, total;

	/* check channels used */
	cnt1 = 0;
	cnt2 = 0;
	for(i = 0; i < NCHANS; i++) {
		cp1 = mpx_get_channel(i);
		cp2 = mpx_get_channel(i);
		cnt1 = mpx_count_channels(cp1);
		cnt2 = mpx_count_channels(cp2);
	}
	/* merge channels */
	total = (cnt1 + cnt2);
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

/* TODO: allow multiple elements to be specified in "elems" */
mpx_disconnect(cp, idx)
	struct mpx_channel *cp;
	int idx;
{
	struct mpx_channel *cp1;
	int i;

	KASSERT(cp != NULL);

	cp1 = mpx_split_channels(cp, idx);

}

mpx_join()
{

}

mpx_leave()
{

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
    struct mpx 	*mpx;
    int error;
    mpx = (struct mpx *)fp->f_mpx;
    mpx->mpx_file = fp;

    error = mpxread(mpx, state);
    return (0);
}

int
mpx_write(fp, uio, cred)
    struct file *fp;
    struct uio *uio;
    struct ucred *cred;
{
	struct mpx 	*mpx;
	int error;

	mpx = (struct mpx *)fp->f_mpx;
	mpx->mpx_file = fp;

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
