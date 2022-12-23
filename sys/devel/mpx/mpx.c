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

int mpxchan(int, int, struct mpx *, int);
int mpxgroup(int, int, struct mpx *, int);

struct grouplist   	mpx_groups[NGROUPS];
struct channellist  mpx_channels[NCHANS];
int groupcount;
int channelcount;

void
mpx_init(void)
{
	register int i, j;

    for(i = 0; i < NGROUPS; i++) {
    	LIST_INIT(&mpx_groups[i]);
    }

	for(j = 0; j < NCHANS; j++) {
		LIST_INIT(&mpx_channels[j]);
	}
	/* A count of -1 indicates no channel or group exists */
	groupcount = -1;
	channelcount = -1;
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
mpx_set(mpx)
	struct mpx *mpx;
{
	struct mpx mx;
	int error;

	error = copyin(mpx, &mx, sizeof(struct mpx));
	return (error);
}

static int
mpx_get(mpx)
	struct mpx *mpx;
{
	struct mpx mx;
	int error;

	error = copyout(mpx, &mx, sizeof(struct mpx));
	return (error);
}

/* mpx callback */
int
mpxcall(cmd, type, mpx, idx)
	int cmd, type, idx;
	struct mpx *mpx;
{
	int error, nchans, ngroups;

	nchans = NCHANS;
	ngroups = NGROUPS;

	error = mpx_set(mpx);
	if (error) {
		switch (type) {
		case MPXCHANNEL:
			error = mpxchan(cmd, idx, mpx, nchans);
			break;

		case MPXGROUP:
			error = mpxgroup(cmd, idx, mpx, ngroups);
			break;
		}
	}

	if (error != 0) {
		return (error);
	}

	error = mpx_get(mpx);
	return (error);
}

/* mpx syscall */
int
mpx()
{
	register struct mpx_args {
		syscallarg(int) cmd;
		syscallarg(int) type; /* channel or group */
		syscallarg(struct mpx *) mpx;
		syscallarg(int) idx; /* channel or group index */
	} *uap = (struct mpx_args *)u.u_ap;

	int error;

	error = mpxcall(SCARG(uap, cmd), SCARG(uap, type), SCARG(uap, mpx), SCARG(uap, idx));
	return (error);
}

int
mpxchan(cmd, idx, mpx, nchans)
	int cmd, idx, nchans;
	struct mpx *mpx;
{
	int error;

	switch (cmd) {
	case MPXCREATE:
		if (channelcount < 0) {
			idx = 0;
		}
		mpx_create_channel(mpx, idx, nchans);
		break;

	case MPXDESTROY:
		if (channelcount <= 0) {
			idx = -1;
		}
		mpx_destroy_channel(mpx, idx);
		break;

	case MPXPUT:
		if (mpx->mpx_channel != NULL) {
			if (channelcount < 0) {
				idx = 0;
				mpx_add_channel(mpx->mpx_channel, idx);
			} else {
				idx = channelcount + 1;
				mpx_add_channel(mpx->mpx_channel, idx);
			}
		} else {
			idx = 0;
			mpx_create_channel(mpx, idx, nchans);
		}
		if (mpx->mpx_group != NULL) {
			mpx_set_channelgroup(mpx->mpx_channel, mpx->mpx_group);
		}
		break;

	case MPXREMOVE:
		if (mpx->mpx_channel != NULL) {
			mpx_remove_channel(mpx->mpx_channel, idx);
		}
		break;

	case MPXGET:
		if(idx >= 0) {
			mpx->mpx_channel = mpx_get_channel(idx);
		} else {
			return (ENOMEM);
		}
		break;
	}
	return (0);
}

int
mpxgroup(cmd, idx, mpx, ngroups)
	int cmd, idx, ngroups;
	struct mpx *mpx;
{
	switch (cmd) {
	case MPXCREATE:
		if (groupcount < 0) {
			idx = 0;
		}
		mpx_create_group(mpx, idx, ngroups);
		break;

	case MPXDESTROY:
		if (groupcount <= 0) {
			idx = -1;
		}
		mpx_destroy_group(mpx, idx);
		break;

	case MPXPUT:
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
			mpx_create_group(mpx, idx, ngroups);
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
		if(idx >= 0) {
			mpx->mpx_group = mpx_get_group(idx);
		} else {
			return (ENOMEM);
		}
		break;
	}
	return (0);
}

void
mpx_set_channelgroup(cp, gp)
	struct mpx_channel 	*cp;
	struct mpx_group 	*gp;
{
	if (cp == NULL && gp == NULL) {
		return;
	}
	cp->mpc_group = gp;
	gp->mpg_channel = cp;
}

void
mpx_set_grouppgrp(gp, pgrp)
	struct mpx_group 	*gp;
	struct pgrp 		*pgrp;
{
	gp->mpg_pgrp = pgrp;
}

struct mpx_group *
mpx_allocate_groups(mpx, ngroups)
	struct mpx 	*mpx;
	int 		ngroups;
{
	register struct mpx_group *result;

	result = (struct mpx_group *)calloc(ngroups, sizeof(struct mpx_group *), M_MPX, M_WAITOK);
	result->mpg_index = -1;
	mpx->mpx_group = result;
	return (result);
}

void
mpx_deallocate_groups(mpx, gp)
	struct mpx 		 *mpx;
	struct mpx_group *gp;
{
	if(gp == NULL) {
		return;
	}

	mpx->mpx_group = mpx_get_group(gp->mpg_index);
	if(mpx->mpx_group != gp) {
		return;
	}

	free(gp, M_MPX);
	mpx->mpx_group = gp;
}

void
mpx_add_group(gp, idx)
	struct mpx_group *gp;
	int idx;
{
	struct grouplist *group;

	if(gp == NULL) {
	        return;
	}
	group = &mpx_groups[idx];
    gp->mpg_index = idx;
    LIST_INSERT_HEAD(group, gp, mpg_node);
    groupcount++;
}

void
mpx_create_group(mpx, idx, ngroups)
	struct mpx 	*mpx;
	int idx, ngroups;
{
	struct mpx_group *gp;

	gp = mpx_allocate_groups(mpx, ngroups);
	mpx_add_group(gp, idx);
}

void
mpx_destroy_group(mpx, idx)
	struct mpx 	*mpx;
	int idx;
{
	struct mpx_group *gp;

	gp = mpx_get_group(idx);
	mpx_remove_group(gp, gp->mpg_index);
	mpx_deallocate_groups(mpx, gp);
}

struct mpx_group *
mpx_get_group(idx)
    int idx;
{
    struct grouplist *group;
    struct mpx_group *gp;

    KASSERT(idx < NGROUPS);
    group = &mpx_groups[idx];
    LIST_FOREACH(gp, group, mpg_node) {
        if(gp->mpg_index == idx) {
            return (gp);
        }
    }
    return (NULL);
}

/*
 * returns the associated group from channel index
 */
struct mpx_group *
mpx_get_group_from_channel(idx)
	int idx;
{
    struct mpx_group    *gp;
    struct mpx_channel  *cp;

    cp = mpx_get_channel(idx);
    if(cp != NULL) {
        gp = cp->mpc_group;
        if(gp != NULL) {
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
	groupcount--;
}

struct mpx_channel *
mpx_allocate_channels(mpx, nchans)
	struct mpx 	*mpx;
	int 		nchans;
{
	register struct mpx_channel *result;

	result = (struct mpx_channel *)calloc(nchans, sizeof(struct mpx_chan *), M_MPX, M_WAITOK);
	result->mpc_index = -1;
	mpx->mpx_channel = result;
	return (result);
}

void
mpx_deallocate_channels(mpx, cp)
	struct mpx 	*mpx;
	struct mpx_channel *cp;
{
	if(cp == NULL) {
		return;
	}

	mpx->mpx_channel = mpx_get_group(cp->mpc_index);
	if(mpx->mpx_channel != cp) {
		return;
	}

	free(cp, M_MPX);
	mpx->mpx_channel = cp;
}

void
mpx_add_channel(cp, idx)
	struct mpx_channel *cp;
	int idx;
{
	struct channellist *chan;
    if(cp == NULL) {
        return;
    }
    chan = &mpx_channels[idx];
    cp->mpc_index = idx;
    LIST_INSERT_HEAD(chan, cp, mpc_node);
    channelcount++;
}

void
mpx_create_channel(mpx, idx, nchans)
	struct mpx 			*mpx;
	int 				idx, nchans;
{
    struct mpx_channel *cp;

    cp = mpx_allocate_channels(mpx, nchans);
    mpx_add_channel(cp, idx);
}

void
mpx_destroy_channel(mpx, idx)
	struct mpx 	*mpx;
	int idx;
{
	struct mpx_channel *cp;

	cp = mpx_get_channel(idx);
	mpx_remove_channel(cp, cp->mpc_index);
	mpx_deallocate_channels(mpx, cp);
}

struct mpx_channel *
mpx_get_channel(idx)
    int idx;
{
    struct channellist *chan;
    struct mpx_channel *cp;

    KASSERT(idx < NCHANS);
    chan = &mpx_channels[idx];
    LIST_FOREACH(cp, chan, mpc_node) {
        if(cp->mpc_index == idx) {
            return (cp);
        }
    }
    return (NULL);
}

/*
 * returns the associated channel from group index
 */
struct mpx_channel *
mpx_get_channel_from_group(idx)
	int idx;
{
	struct mpx_group *gp;
    struct mpx_channel  *cp;

    gp = mpx_get_group(idx);
    if(gp != NULL) {
        cp = gp->mpg_channel;
        if(cp != NULL) {
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
	channelcount--;
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
	for (i = 0; i < NGROUPS; i++) {
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

	for (i = 0; i < NGROUPS; i++) {
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
mpx_compare_index(idx1, idx2)
	int idx1, idx2;
{
	if (idx1 < idx2) {
		return (-1);
	} else if (idx1 > idx2) {
		return (1);
	} else {
		return (0);
	}
}

int
mpx_compare_groups(gp1, gp2)
	struct mpx_group *gp1, *gp2;
{
	if (gp1 < gp2) {
		return (-1);
	} else if(gp1 > gp2) {
		return (1);
	} else {
		return (mpx_compare_index(gp1->mpg_index, gp2->mpg_index));
	}
}

int
mpx_compare_channels(cp1, cp2)
	struct mpx_channel 	*cp1, *cp2;
{
	if (cp1 < cp2) {
		return (-1);
	} else if(cp1 > cp2) {
		return (1);
	} else {
		return (mpx_compare_index(cp1->mpc_index, cp2->mpc_index));
	}
}

/*
 * Lock a mpx for I/O, blocking other access
 * Called with mpx spin lock held.
 * Return with mpx spin lock released on success.
 */
static int
mpxlock(mpx, catch)
	struct mpx 	*mpx;
	int catch;
{
	int error;

	while (1) {
		error = lockmgr(&mpx->mpx_lock, LK_EXCLUSIVE | LK_INTERLOCK,
				&mpx->mpx_slock, LOCKHOLDER_PID(&mpx->mpx_lock.lk_lockholder));
		if (error == 0) {
			break;
		}
		simple_lock(&mpx->mpx_slock);
		if ( catch || (error != EINTR && error != ERESTART)) {
			break;
		}
		/*
		 * XXX XXX XXX
		 * The mpx lock is initialised with PCATCH on and we cannot
		 * override this in a lockmgr() call. Thus a pending signal
		 * will cause lockmgr() to return with EINTR or ERESTART.
		 * We cannot simply re-enter lockmgr() at this point since
		 * the pending signals have not yet been posted and would
		 * cause an immediate EINTR/ERESTART return again.
		 * As a workaround we pause for a while here, giving the lock
		 * a chance to drain, before trying again.
		 * XXX XXX XXX
		 *
		 * NOTE: Consider dropping PCATCH from this lock; in practice
		 * it is never held for long enough periods for having it
		 * interruptable at the start of pipe_read/pipe_write to be
		 * beneficial.
		 */
		(void) ltsleep(&lbolt, PSOCK, "rstrtmpxlock", hz, &mpx->mpx_slock);
	}
	return (error);
}

static __inline void
mpxunlock(mpx)
	struct mpx 	*mpx;
{
	lockmgr(&mpx->mpx_lock, LK_RELEASE, NULL, LOCKHOLDER_PID(&mpx->mpx_lock.lk_lockholder));
}
