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

/*
 * Copyright (c) 1996 John S. Dyson
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
 *
 * $FreeBSD: src/sys/kern/sys_pipe.c,v 1.95 2002/03/09 22:06:31 alfred Exp $
 */

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
#include <sys/null.h>

#include <vm/include/vm_extern.h>

#include <devel/mpx/mpx.h>
#include <devel/sys/malloctypes.h>

/*
 * TODO: Possible idea
 * - make mpx a sys pipe extension.
 * - mpx have their own fileops and syscall.
 * 	- however they can return the pipe equivalent.
 *
 * 	example:
 * 	mpx_read() {
 * 		mpx read code function here
 *
 * 		return (pipe_read());
 * 	}
 *
 * 	- This would simplify the mpx code, but offer same functionality.
 * 	- Keeping it generic enough, but also niche capable.
 * 	- It also has the added benefit of being a plugin\module if the other BSD's wish to adopt it.
 * 	not a rewrite of pipes.
 */

/* mpx type */
#define MPXCHANNEL 	0
#define MPXGROUP 	1

/* mpx args */
#define MPXALLOCATE	0
#define MPXADD		1
#define MPXREMOVE	2
#define MPXGET		3

int mpxchan(struct mpx *, int, int, int);
int mpxgroup(struct mpx *, int, int, int);

/* mpx syscall */
int
mpx()
{
	register struct mpx_args {
		syscallarg(int) 	cmd;
		syscallarg(int) 	type; /* channel or group */
		syscallarg(int) 	idx;
	} *uap;
	struct mpxpair 	*mm;
	struct mpx 		*mpx;
	int error, nchans, ngroups;

	nchans = NCHANS;
	ngroups = NGROUPS;

	mm = mpx_paircreate();
	if(mm == NULL) {
		return (ENOMEM);
	}
	mpx = &mm->mpp_rmpx; /* uses mpx reader */

	switch(SCARG(uap, type)) {
	case MPXCHANNEL:
		error = mpxchan(mpx, SCARG(uap, cmd), SCARG(uap, idx), nchans);
		break;

	case MPXGROUP:
		error = mpxgroup(mpx, SCARG(uap, cmd), SCARG(uap, idx), ngroups);
		break;
	}
	return (error);
}

int
mpxchan(mpx, cmd, idx, nchans)
	struct mpx *mpx;
	int cmd, idx, nchans;
{
	 switch(cmd) {
	 case MPXALLOCATE:
		 mpx->mpx_channel = mpx_allocate_channels(mpx, nchans);
		 if(mpx->mpx_channel == NULL) {
			 return (ENIVAL);
		 }
		 break;

	 case MPXADD:
		 if(mpx->mpx_channel != NULL) {
			 if(channelcount < 0) {
				 idx = 0;
				 mpx_add_channel(mpx->mpx_channel, idx);
			 } else {
				 idx = channelcount+1;
				 mpx_add_channel(mpx->mpx_channel, idx);
			 }
		 } else {
			 idx = 0;
			 mpx_create_channel(mpx->mpx_channel, idx, nchans);
		 }
		 if(mpx->mpx_group != NULL) {
			 mpx_set_channelgroup(mpx->mpx_channel, mpx->mpx_group);
		 }
		 break;

	 case MPXREMOVE:
		 if(mpx->mpx_channel != NULL) {
			 mpx_remove_channel(mpx->mpx_channel, idx);
		 }
		 break;

	 case MPXGET:
		 mpx->mpx_channel = mpx_get_channel(idx);
		 if(mpx->mpx_channel == NULL) {
			 return (ENIVAL);
		 }
		 break;
	 }
	 return (0);
}

int
mpxgroup(mpx, cmd, idx, ngroups)
	struct mpx *mpx;
	int cmd, idx, ngroups;
{
	switch (cmd) {
	case MPXALLOCATE:
		mpx->mpx_group = mpx_allocate_groups(mpx, ngroups);
		if (mpx->mpx_group == NULL) {
			return (ENIVAL);
		}
		break;

	case MPXADD:
		if (mpx->mpx_group != NULL) {
			if (groupcount < 0) {
				idx = 0;
				mpx_add_group(mpx->mpx_group, idx);
			} else {
				idx = groupcount + 1;
				mpx_add_group(mpx->mpx_group, idx);
			}
		} else {
			idx = 0;
			mpx_create_group(mpx->mpx_group, idx, ngroups);
		}
		if (mpx->mpx_channel != NULL) {
			mpx_set_channelgroup(mpx->mpx_channel, mpx->mpx_group);
		}
		break;

	case MPXREMOVE:
		if (mpx->mpx_group != NULL) {
			mpx_remove_group(mpx->mpx_group, idx);
		}
		break;

	case MPXGET:
		mpx->mpx_group = mpx_get_group(idx);
		if (mpx->mpx_group == NULL) {
			return (ENIVAL);
		}
		break;
	}
	 return (0);
}

/* mpx file ops */
int
mpxnread(mpx, gpidx, cpidx)
	struct mpx *mpx;
	int gpidx, cpidx;
{
    struct mpx_group 	*gp;
    struct mpx_channel 	*cp;

    KASSERT(gpidx <= NGROUPS);
    KASSERT(cpidx <= NCHANS);
    gp = mpx_get_group(gpidx);
    cp = mpx_get_channel(cpidx);
    if(gp == NULL && cp == NULL) {
    	return (EMPX);
    }
    if (gp != NULL && cp == NULL) {
    	cp = mpx_get_channel_from_group(gpidx);
        if (cpidx != cp->mpc_index) {
            return (EMPX);
        }
    }
	if (gp == NULL && cp != NULL) {
		gp = mpx_get_group_from_channel(cpidx);
        if (gpidx != gp->mpg_index) {
            return (EMPX);
        }
	}

	if(gp != mpx->mpx_group || mpx->mpx_group == NULL) {
		mpx->mpx_group = gp;
	}
	if(cp != mpx->mpx_channel || mpx->mpx_channel == NULL) {
		mpx->mpx_channel = cp;
	}
    return (0);
}

int
mpxnwrite(mpx, gpidx, cpidx)
	struct mpx *mpx;
	int gpidx, cpidx;
{
    struct mpx_group *gp;
    struct mpx_channel *cp;

    KASSERT(gpidx <= NGROUPS);
    KASSERT(cpidx <= NCHANS);
    gp = mpx_allocate_groups(mpx, NGROUPS);
    cp = mpx_allocate_channels(mpx, NCHANS);
    mpx_add_group(gp, gpidx);
    mpx_add_channel(cp, cpidx);
    mpx_set_channelgroup(cp, gp);
    mpx->mpx_group = gp;
    mpx->mpx_channel = cp;

    return (0);
}

int
mpxn_read(rmpx)
	struct mpx *rmpx;
{
    struct mpx_group *gp;
    struct mpx_channel *cp;
    int error, i, j;

    gp = rmpx->mpx_group;
    cp = rmpx->mpx_channel;
	for (i = 0; i < NGROUPS; i++) {
		for (j = 0; j < NCHANS; j++) {
			 if(i == gp->mpg_index && j == cp->mpc_index) {
				 error = mpxnread(rmpx, i, j);
			 }
		}
	}
	return (error);
}

int
mpxn_write(wmpx)
	struct mpx *wmpx;
{
    struct mpx_group *gp;
    struct mpx_channel *cp;
    int error, i, j;

    gp = wmpx->mpx_group;
    cp = wmpx->mpx_channel;
	for (i = 0; i < NGROUPS; i++) {
		for (j = 0; j < NCHANS; j++) {
			error = mpxnwrite(wmpx, i, j);
		}
	}
    return (error);
}

int
mpx_ioctl_sc(mpx, cmd, data, p)
	struct mpx 	*mpx;
	u_long cmd;
	caddr_t data;
	struct proc *p;
{
	struct mpx_group 	*grp;
	struct mpx_channel 	*chan, *chan1;
	int nchans;

	grp = mpx->mpx_group;
	chan = mpx->mpx_channel;
	switch (cmd) {
	case MPXIOATTACH:
		if(grp == NULL && chan == NULL) {
			grp = mpx_allocate_groups(mpx, NGROUPS);
			chan = mpx_allocate_channels(mpx, NCHANS);
			mpx_add_group(grp, 0);
			mpx_add_channel(chan, 0);
			mpx_set_channelgroup(chan, grp);
			goto attach;
		}
		if(grp != NULL && chan == NULL) {
			chan = mpx_allocate_channels(mpx, NCHANS);
			mpx_add_channel(chan, 0);
			mpx_set_channelgroup(chan, grp);
			goto attach;
		}
		if(grp == NULL && chan != NULL) {
			grp = mpx_allocate_groups(mpx, NGROUPS);
			mpx_add_group(grp, 0);
			mpx_set_channelgroup(chan, grp);
			goto attach;
		}
attach:
		mpx_attach(chan, grp);
		return (0);

	case MPXIODETACH:
		if(grp != NULL && chan == NULL) {
			chan = mpx_get_channel_from_group(grp->mpg_index);
			goto detach;
		}
		if(grp == NULL && chan != NULL) {
			grp = mpx_get_group_from_channel(chan->mpc_index);
			goto detach;
		}
detach:
		mpx_detach(chan, grp);
		return (0);

	case MPXIOCONNECT:
		chan1 = (struct mpx_channel *)data;
		return (mpx_connect(chan, chan1));

	case MPXIODISCONNECT:
		nchans = (int)data;
		chan1 = mpx_disconnect(chan, nchans);
		return (0);
	}
	return (0);
}
