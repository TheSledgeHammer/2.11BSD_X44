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

int
mpx()
{
	/*
	register struct mpx_args {
		syscallarg(int *) fdp;
	} *uap = (struct mpx_args *) u.u_ap;
	*/
	struct file *rf, *wf;
	struct mpx *rmpx, *wmpx;
	struct mpxpair *mm;
	int fd, error;

	mm = mpx_paircreate();
	if(mm == NULL) {
		return (ENOMEM);
	}
	rmpx = &mm->mpp_rmpx;
	wmpx = &mm->mpp_wmpx;

	rf = falloc();
	if (rf != NULL) {
		u.u_r.r_val1 = fd;
		rf->f_flag = FREAD;
		rf->f_type = DTYPE_PIPE;
		rf->f_mpx = rmpx;
		rf->f_ops = &mpxops;
		error = ufdalloc(rf);
		if(error != 0) {
			goto free2;
		}
	} else {
		u.u_error = ENFILE;
		goto free2;
	}
	wf = falloc();
	if (wf != NULL) {
		u.u_r.r_val2 = fd;
		wf->f_flag = FWRITE;
		wf->f_type = DTYPE_PIPE;
		wf->f_mpx = wmpx;
		wf->f_ops = &mpxops;
		error = ufdalloc(wf);
		if(error != 0) {
			goto free3;
		}
	} else {
		u.u_error = ENFILE;
		goto free3;
	}

	rmpx->mpx_peer = wmpx;
	wmpx->mpx_peer = rmpx;

	FILE_UNUSE(rf, u.u_procp);
	FILE_UNUSE(wf, u.u_procp);
	return (0);

free3:
	FILE_UNUSE(rf, u.u_procp);
	ffree(rf);
	fdremove(u.u_r.r_val1);

free2:
	mpxclose(NULL, rmpx);
	mpxclose(NULL, wmpx);
	return (u.u_error);
}

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

#define MPXCHAN 	0	/* create new channel */
#define MPXGROUP 	1	/* create new group */

int
mpxchan()
{
	register struct mpx_args {
		syscallarg(int) 	cmd;
		syscallarg(int *) 	arg;
	} *uap = (struct mpx_args *) u.u_ap;

	struct mpx *rmpx, *wmpx;
	struct mpxpair *mm;
	struct mpx_group 	*gp;
	struct mpx_channel 	*cp;

	mm = mpx_paircreate();
	if(mm == NULL) {
		return (ENOMEM);
	}
	rmpx = &mm->mpp_rmpx;
	wmpx = &mm->mpp_wmpx;

	switch(SCARG(uap, cmd)) {
	case MPXCHAN:
		cp = mpx_create_channel(wmpx, NCHANS);
		mpx_add_channel(cp, 0);
		return (0);

	case MPXGROUP:
		gp = mpx_create_group(wmpx, NGROUPS);
		mpx_add_group(gp, 0);
		return (0);

	case MPXIOATTACH:
	}

	return (u.u_error);
}

void
mpx_channel(mpx, cmd)
	struct mpx *mpx;
	int 	cmd;
{
	struct file *rf;
	struct mpx *rmpx, *wmpx;
    struct mpx_group *gp;
    struct mpx_channel *cp;
    int error;

    /* reader */
    if(rmpx->mpx_group != NULL){
        gp = rmpx->mpx_group;
    } else {
    	gp = NULL;
    }
    if(rmpx->mpx_channel != NULL) {
    	 cp = rmpx->mpx_channel;
    } else {
    	cp = NULL;
    }

    /* writer */
    switch(cmd) {
    case MPXCHAN:
    	if(cp != NULL) {
    		if(channelcount < 0) {
    			mpx_add_channel(cp, 0);
    		} else {
    			int idx;
    			idx = channelcount+1;
    			mpx_add_channel(cp, idx);
    		}
    	} else {
    		cp = mpx_allocate_channels(wmpx, NCHANS);
    		mpx_add_channel(cp, 0);
    	}
		if(gp != NULL) {
			mpx_set_channelgroup(cp, gp);
		}
    	break;

    case MPXGROUP:
    	if(gp != NULL) {
    		if(groupcount < 0) {
    			mpx_add_group(gp, 0);
    		} else {
    			int idx;
    			idx = groupcount+1;
    			mpx_add_group(gp, idx);
    		}
    	} else {
    		gp = mpx_allocate_channels(wmpx, NGROUPS);
    		mpx_add_group(gp, 0);
    	}
    	mpx_set_grouppgrp(gp, u.u_procp->p_pgrp);
    	break;
    }
}



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
/* mpxchan ctl args */
#define MPXCHANCREATE		0
#define MPXCHANADD			1
#define MPXCHANREMOVE		2
#define MPXCHANGET			3

/* mpxgroup ctl args */
#define MPXGROUPCREATE		0
#define MPXGROUPADD			1
#define MPXGROUPREMOVE		2
#define MPXGROUPGET			3

int
mpxchan()
{
	struct mpxchannel_args {
		syscallarg(int) 	cmd;
		syscallarg(void *) 	arg;
		syscallarg(int) 	nchans;
		syscallarg(int) 	idx;
	} *uap;

	 switch(cmd) {
	 case MPXCHANCREATE:
	 case MPXCHANADD:
	 case MPXCHANREMOVE:
	 case MPXCHANGET:
	 }
	 return (error);
}

struct mpxgroup_args {
	syscallarg(int) 			cmd;
	syscallarg(void *) 			arg;
	syscallarg(struct mpx *) 	mpx;
	syscallarg(int) 			idx;
	syscallarg(int) 			ngroups;
};

int
mpxgroup()
{
	struct mpxgroup_args/* {
		syscallarg(int) 			cmd;
		syscallarg(void *) 			arg;
		syscallarg(struct mpx *) 	mpx;
		syscallarg(int) 			idx;
		syscallarg(int) 			ngroups;
	}*/ *uap;

	struct mpxpair *mm;
	struct mpx mpx;
	int error, idx;;

	if(SCARG(uap, arg) == MPXGROUPADD) {
		error = copyin(&mpx, SCARG(uap, mpx), sizof(mpx));
		if (error != 0) {
			goto out;
		}
	}

	switch (SCARG(uap, cmd)) {
	case MPXGROUPCREATE:
		if(&mpx.mpx_group == NULL) {
			&mpx.mpx_group = mpx_allocate_groups(mpx, SCARG(uap, ngroups));
		} else {
			error = ENIVAL;
		}
		break;

	case MPXGROUPADD:
		if (&mpx.mpx_group != NULL) {
			if (groupcount < 0) {
				mpx_add_group(&mpx.mpx_group, 0);
			} else {
				SCARG(uap, idx) = groupcount + 1;
				mpx_add_group(&mpx.mpx_group, SCARG(uap, idx));
			}
		} else {
			error = ENIVAL;
		}
		break;

	case MPXGROUPREMOVE:
		mpx_remove_group(&mpx.mpx_group, idx);
		break;

	case MPXGROUPGET:
		&mpx.mpx_group = mpx_get_group(SCARG(uap, idx));
		break;
	}

out:
	return (error);
}
