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

struct grplist   mpx_grps[NGROUPS];
struct chanlist  mpx_chans[NCHANS];

void
mpx_init(void)
{
	struct mpx 			*mpx;
	register int i, j;

	MALLOC(mpx, struct mpx *, sizeof(struct mpx *), M_MPX, M_WAITOK);
    simple_lock_init(mpx->mpx_pair, "mpx_lock");

    for(i = 0; i < NGROUPS; i++) {
    	LIST_INIT(&mpx_grps[i]);
    }

	for(j = 0; j < NCHANS; j++) {
		  LIST_INIT(&mpx_chans[j]);
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
mpx_create_group(index)
    int index;
{
    struct grplist *group;
    struct mpx_group *gp;

    group = &mpx_grps[index];
    gp = (struct mpx_group *)calloc(NGROUPS, sizeof(struct mpx_group *), M_MPX, M_WAITOK);
    gp->mpg_index = index;

    LIST_INSERT_HEAD(group, gp, mpg_node);
}

struct mpx_group *
mpx_get_group(index)
    int index;
{
    struct grplist *group;
    struct mpx_group *gp;

    group = &mpx_grps[index];
    LIST_FOREACH(gp, group, mpg_node) {
        if(gp->mpg_index == index) {
            return (gp);
        }
    }
    return (NULL);
}

void
mpx_group_remove(gp, index)
	struct mpx_group *gp;
	int index;
{
	if(gp->mpg_index == index) {
		LIST_REMOVE(gp, mpg_node);
	} else {
		gp = mpx_get_group(index);
		LIST_REMOVE(gp, mpg_node);
	}
}

void
mpx_create_channel(gp, index, flags)
	struct mpx_group *gp;
    int index, flags;
{
    struct chanlist *chan;
    struct mpx_chan *cp;

    chan = &mpx_chans[index];
    cp = (struct mpx_chan *)calloc(NCHANS, sizeof(struct mpx_chan *), M_MPX, M_WAITOK);
    cp->mpc_index = index;
    cp->mpc_flags = flags;
    cp->mpc_group = gp;

    LIST_INSERT_HEAD(chan, cp, mpc_node);
}

struct mpx_chan *
mpx_get_channel(index)
    int index;
{
    struct chanlist *chan;
    struct mpx_chan *cp;

    chan = &mpx_chans[index];
    LIST_FOREACH(cp, chan, mpc_node) {
        if(cp->mpc_index == index) {
            return (cp);
        }
    }
    return (NULL);
}

void
mpx_remove_channel(cp, index)
	struct mpx_chan *cp;
	int index;
{
	if (cp->mpc_index == index) {
		LIST_REMOVE(cp, mpc_node);
	} else {
		cp = mpx_get_channel(index);
		LIST_REMOVE(cp, mpc_node);
	}
}

mpxspace(flags)
	int flags;
{
	struct mpx_group *group;
	struct mpx_chan *chan;
	int i, j;

	for(i = 0; i < NGROUPS; i++) {
		mpx_create_group(group, i);

		for(j = 0; j < NCHANS; j++) {
			mpx_create_channel(group->mpg_chan, group, j, flags);
			chan = group->mpg_chan;
		}
	}
}

mpx_connect()
{

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
    struct mpx *mpx;
    struct mpx_reader *read;

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
	struct mpx *mpx;
	struct mpx_writer *write;

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
