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
#include <sys/event.h>
#include <sys/poll.h>
#include <sys/null.h>
#include <sys/user.h>

#include <devel/mpx/mpx.h>
#include <devel/sys/malloctypes.h>

struct fileops mpxops = {
		.fo_rw = mpx_rw,
		.fo_read = mpx_read,
		.fo_write = mpx_write,
		.fo_ioctl = mpx_ioctl,
		.fo_poll = mpx_poll,
		.fo_close = mpx_close,
		.fo_kqfilter = mpx_kqfilter,
};

struct grouplist   	mpx_groups[NGROUPS];
struct channellist  mpx_channels[NCHANS];

#define MPX_TIMESTAMP(tvp)	(*(tvp) = time)

/*
 * Default mpx buffer size(s), this can be kind-of large now because mpx
 * space is pageable.  The mpx code will try to maintain locality of
 * reference for performance reasons, so small amounts of outstanding I/O
 * will not wipe the cache.
 */
#define MINMPXSIZE (MPX_SIZE/3)
#define MAXMPXSIZE (2*MPX_SIZE/3)

/*
 * Maximum amount of kva for mpxs -- this is kind-of a soft limit, but
 * is there so that on large systems, we don't exhaust it.
 */
#define MAXMPXKVA (8*1024*1024)
static int maxmpxkva = MAXMPXKVA;

/*
 * Limit for direct transfers, we cannot, of course limit
 * the amount of kva for mpxs in general though.
 */
#define LIMITMPXKVA (16*1024*1024)
static int limitmpxkva = LIMITMPXKVA;

/*
 * Limit the number of "big" mpxs
 */
#define LIMITBIGMPXS  32
static int maxbigmpxs = LIMITBIGMPXS;
static int nbigmpx = 0;

/*
 * Amount of KVA consumed by mpx buffers.
 */
static int amountmpxkva = 0;

struct mpxpair *mpx_paircreate(void);
static int mpxspace_new(struct mpx *, int);
static int mpxspace(struct mpx *, int);
static void mpxclose(struct file *, struct mpx 	*);
static int mpx_create(struct mpx *, bool_t);
static void mpxselwakeup(struct mpx *, struct mpx *);
static void	mpx_free_kmem(struct mpx *);

static void	filt_mpxdetach(struct knote *);
static int	filt_mpxread(struct knote *, long);
static int	filt_mpxwrite(struct knote *, long);

static struct filterops mpx_rfiltops =
	{ 1, NULL, filt_mpxdetach, filt_mpxread };
static struct filterops mpx_wfiltops =
	{ 1, NULL, filt_mpxdetach, filt_mpxwrite };

void
mpx_init(void)
{
	register int i, j;

    for(i = 0; i <= NGROUPS; i++) {
    	LIST_INIT(&mpx_groups[i]);
    }

	for(j = 0; j <= NCHANS; j++) {
		LIST_INIT(&mpx_channels[j]);
	}
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
}

void
mpx_create_group(mpx, idx, pgrp)
	struct mpx 	*mpx;
	int idx;
	struct pgrp *pgrp;
{
	struct mpx_group *gp;

	gp = mpx_allocate_groups(mpx, NGROUPS);
	mpx_add_group(gp, idx);
	mpx_set_grouppgrp(gp, pgrp);
	mpx->mpx_pgid = gp->mpg_pgrp->pg_id;
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
}

void
mpx_create_channel(mpx, gp, idx)
	struct mpx 			*mpx;
	struct mpx_group 	*gp;
	int 				idx;
{
    struct mpx_channel *cp;

    cp = mpx_allocate_channels(mpx, NCHANS);
    mpx_add_channel(cp, idx);
    mpx_set_channelgroup(cp, gp);
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
	return (mpx_compare_index(gp1->mpg_index, gp2->mpg_index));
}

int
mpx_compare_channels(cp1, cp2)
	struct mpx_channel 	*cp1, *cp2;
{
	return (mpx_compare_index(cp1->mpc_index, cp2->mpc_index));
}

/*
 * Allocate kva for mpx circular buffer, the space is pageable
 * This routine will 'realloc' the size of a mpx safely, if it fails
 * it will retain the old buffer.
 * If it fails it will return ENOMEM.
 */
static int
mpxspace_new(mpx, size)
	struct mpx *mpx;
	int size;
{
	caddr_t buffer;

	buffer = (caddr_t)kmem_alloc(kernel_map, round_page(size));
	if (buffer == NULL) {
		return (ENOMEM);
	}

	/* free old resources if we're resizing */
	mpx_free_kmem(mpx);
	mpx->mpx_buffer.buffer = buffer;
	mpx->mpx_buffer.size = size;
	mpx->mpx_buffer.in = 0;
	mpx->mpx_buffer.out = 0;
	mpx->mpx_buffer.cnt = 0;
	amountmpxkva += mpx->mpx_buffer.size;
	return (0);
}

/*
 * Initialize and allocate VM and memory for mpx.
 */
static int
mpxspace(cmpx, size)
	struct mpx *cmpx;
	int 		size;
{
    KASSERT(cmpx->mpx_state & MPX_LOCKFL);
    return (mpxspace_new(cmpx, size));
}

/*
 * Initialize and allocate VM and memory for mpx.
 */
static int
mpx_create(mpxp, large_backing)
	struct mpx *mpxp;
	bool_t large_backing;
{
	struct mpx *mpx;
	int error;

	mpx = *mpxp = (struct mpx *)malloc(sizeof(struct mpx *), M_MPX, M_WAITOK);
	bzero(mpx, sizeof(struct mpx));
	mpx->mpx_state = MPX_SIGNALR;

	MPX_TIMESTAMP(&mpx->mpx_ctime);
	mpx->mpx_atime = mpx->mpx_ctime;
	mpx->mpx_mtime = mpx->mpx_ctime;
	simple_lock_init(&mpx->mpx_slock, "mpx slock");
	lockinit(&mpx->mpx_lock, PSOCK | PCATCH, "mpxlk", 0, 0);

	error = mpxspace_new(mpx, !large_backing || amountmpxkva > maxmpxkva/2 ? SMALL_MPX_SIZE : MPX_SIZE);
	return (error);
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

static void
mpxselwakeup(selm, sigm)
	struct mpx 	*selm, *sigm;
{
	if (selm->mpx_state & MPX_SEL) {
		selm->mpx_state &= ~MPX_SEL;
		selwakeup(&selm->mpx_sel);
	}
	if (sigm && (sigm->mpx_state & MPX_ASYNC) && sigm->mpx_pgid != NO_PID) {
		struct proc *p;

		p = pfind(sigm->mpx_pgid);
		if(sigm->mpx_pgid > 0 && p != NULL) {
			psignal(p, SIGIO);
		}
	}
}

int
mpx_rw(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
	enum uio_rw rw = uio->uio_rw;
	int error;

	if (rw == UIO_READ) {
		error = mpx_read(fp, uio, cred);
	} else {
		error = mpx_write(fp, uio, cred);
	}
	return (error);
}

int
mpx_read(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
	struct mpx *rmpx;
	struct mpxbuf *bp;
	int error;
	size_t size, nread, ocnt;
	unsigned int wakeup_state = 0;

	rmpx = fp->f_mpx;
	bp = &rmpx->mpx_buffer;
	nread = 0;

	MPX_LOCK(rmpx);
	++rmpx->mpx_busy;
	ocnt = bp->cnt;

again:
	error = mpxlock(rmpx, 1);
	if(error) {
		goto unlocked_error;
	}
	while (uio->uio_resid) {
		/*
		 * Normal pipe buffer receive.
		 */
		if (bp->cnt > 0) {
			size = bp->size - bp->out;
			if (size > bp->cnt) {
				size = bp->cnt;
			}
			if (size > uio->uio_resid) {
				size = uio->uio_resid;
			}

			error = uiomove((char*) bp->buffer + bp->out, size, uio);
			if (error) {
				break;
			}

			bp->out += size;
			if (bp->out >= bp->size)
				bp->out = 0;

			bp->cnt -= size;

			/*
			 * If there is no more to read in the pipe, reset
			 * its pointers to the beginning.  This improves
			 * cache hit stats.
			 */
			if (bp->cnt == 0) {
				bp->in = 0;
				bp->out = 0;
			}
			nread += size;
		} else {
			/*
			 * Break if some data was read.
			 */
			if (nread > 0) {
				break;
			}

			MPX_LOCK(rmpx);

			/*
			 * Detect EOF condition.
			 * Read returns 0 on EOF, no need to set error.
			 */
			if (rmpx->mpx_state & MPX_EOF) {
				break;
			}

			/*
			 * Don't block on non-blocking I/O.
			 */
			if (fp->f_flag & FNONBLOCK) {
				error = EAGAIN;
				MPX_UNLOCK(rmpx);
				break;
			}

			/*
			 * Unlock the pipe buffer for our remaining processing.
			 * We will either break out with an error or we will
			 * sleep and relock to loop.
			 */
			mpxunlock(rmpx);

			if ((rmpx->mpx_state & MPX_DIRECTR) != 0) {
				goto again;
			}

			/*
			 * We want to read more, wake up select/poll.
			 */
			mpxselwakeup(rmpx, rmpx->mpx_peer);

			/*
			 * If the "write-side" is blocked, wake it up now.
			 */
			if (rmpx->mpx_state & MPX_WANTW) {
				rmpx->mpx_state &= ~MPX_WANTW;
				wakeup(rmpx);
			}

			/* Now wait until the pipe is filled */
			rmpx->mpx_state |= MPX_WANTR;
			error = ltsleep(rmpx, PSOCK | PCATCH, "mpxrd", 0, &rmpx->mpx_slock);
			if (error != 0) {
				goto unlocked_error;
			}
			goto again;
		}
	}
	if(error == 0) {
		MPX_TIMESTAMP(&rmpx->mpx_atime);
	}
	MPX_LOCK(rmpx);
	mpxunlock(rmpx);

unlocked_error:
	--rmpx->mpx_busy;
	if ((rmpx->mpx_busy == 0) && (rmpx->mpx_state & MPX_WANT)) {
		rmpx->mpx_state &= ~(MPX_WANT | MPX_WANTW);
		wakeup(rmpx);
	} else if (bp->cnt < MINMPXSIZE) {
		if (rmpx->mpx_state & MPX_WANTW) {
			rmpx->mpx_state &= ~MPX_WANTW;
			wakeup(rmpx);
		}
	}

	/*
	 * If anything was read off the buffer, signal to the writer it's
	 * possible to write more data. Also send signal if we are here for the
	 * first time after last write.
	 */
	if ((bp->size - bp->cnt) >= MPX_BUF
			&& (ocnt != bp->cnt || (rmpx->mpx_state & MPX_SIGNALR))) {
		mpxselwakeup(rmpx, rmpx->mpx_peer);
		rmpx->mpx_state &= ~MPX_SIGNALR;
	}

	MPX_UNLOCK(rmpx);
	return (error);
}

int
mpx_write(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
	struct mpx *wmpx, *rmpx;
	struct mpxbuf *bp;
	int error;

	/* We want to write to our peer */
	rmpx = fp->f_mpx;

retry:
	error = 0;
	MPX_LOCK(rmpx);
	wmpx = rmpx->mpx_peer;

	/*
	 * Detect loss of mpx read side, issue SIGPIPE if lost.
	 */
	if (wmpx == NULL) {
		error = EMPX;
	} else if (simple_lock_try(&wmpx->mpx_slock) == 0) {
		/* Deal with race for peer */
		MPX_UNLOCK(rmpx);
		goto retry;
	} else if ((wmpx->mpx_state & MPX_EOF) != 0) {
		MPX_UNLOCK(wmpx);
		error = EMPX;
	}

	MPX_UNLOCK(rmpx);
	if (error != 0)
		return (error);

	++wmpx->mpx_busy;

	/* Aquire the long-term mpx lock */
	if ((error = mpxlock(wmpx, 1)) != 0) {
		--wmpx->mpx_busy;
		if (wmpx->mpx_busy == 0
		    && (wmpx->mpx_state & MPX_WANT)) {
			wmpx->mpx_state &= ~(MPX_WANT | MPX_WANTR);
			wakeup(wmpx);
		}
		MPX_UNLOCK(wmpx);
		return (error);
	}

	bp = &wmpx->mpx_buffer;

	/*
	 * If it is advantageous to resize the mpx buffer, do so.
	 */
	if ((uio->uio_resid > MPX_SIZE) && (nbigmpx < maxbigmpxs)
			&& (bp->size <= MPX_SIZE) && (bp->cnt == 0)) {

		if (mpxspace(wmpx, BIG_MPX_SIZE) == 0) {
			nbigmpx++;
		}
	}

	while (uio->uio_resid) {
		size_t space;

		space = bp->size - bp->cnt;

		/* Writes of size <= PIPE_BUF must be atomic. */
		if ((space < uio->uio_resid) && (uio->uio_resid <= MPX_BUF))
			space = 0;

		if (space > 0) {
			int size;		/* Transfer size */
			int segsize;	/* first segment to transfer */

			/*
			 * Transfer size is minimum of uio transfer
			 * and free space in mpx buffer.
			 */
			if (space > uio->uio_resid) {
				size = uio->uio_resid;
			} else {
				size = space;
			}
			/*
			 * First segment to transfer is minimum of
			 * transfer size and contiguous space in
			 * mpx buffer.  If first segment to transfer
			 * is less than the transfer size, we've got
			 * a wraparound in the buffer.
			 */
			segsize = bp->size - bp->in;
			if (segsize > size) {
				segsize = size;
			}

			/* Transfer first segment */
			error = uiomove(&bp->buffer[bp->in], segsize, uio);

			if (error == 0 && segsize < size) {
				/*
				 * Transfer remaining part now, to
				 * support atomic writes.  Wraparound
				 * happened.
				 */
#ifdef DEBUG
				if (bp->in + segsize != bp->size) {
					panic("Expected mpx buffer wraparound disappeared");
				}
#endif

				error = uiomove(&bp->buffer[0],
						size - segsize, uio);
			}
			if (error) {
				break;
			}

			bp->in += size;
			if (bp->in >= bp->size) {
#ifdef DEBUG
				if (bp->in != size - segsize + bp->size) {
					panic("Expected wraparound bad");
				}
#endif
				bp->in = size - segsize;
			}

			bp->cnt += size;
#ifdef DEBUG
			if (bp->cnt > bp->size) {
				panic("Mpx buffer overflow");
			}
#endif
		} else {
			/*
			 * If the "read-side" has been blocked, wake it up now.
			 */
			MPX_LOCK(wmpx);
			if (wmpx->mpx_state & MPX_WANTR) {
				wmpx->mpx_state &= ~MPX_WANTR;
				wakeup(wmpx);
			}
			MPX_UNLOCK(wmpx);

			/*
			 * don't block on non-blocking I/O
			 */
			if (fp->f_flag & FNONBLOCK) {
				error = EAGAIN;
				break;
			}

			/*
			 * We have no more space and have something to offer,
			 * wake up select/poll.
			 */
			if (bp->cnt) {
				mpxselwakeup(wmpx, wmpx);
			}

			MPX_LOCK(wmpx);
			mpxunlock(wmpx);
			wmpx->mpx_state |= MPX_WANTW;
			error = ltsleep(wmpx, PSOCK | PCATCH, "mpxwr", 0,
					&wmpx->mpx_slock);
			(void)mpxlock(wmpx, 0);
			if (error != 0)
				break;
			/*
			 * If read side wants to go away, we just issue a signal
			 * to ourselves.
			 */
			if (wmpx->mpx_state & MPX_EOF) {
				error = EMPX;
				break;
			}
		}
	}

	MPX_LOCK(wmpx);
	--wmpx->mpx_busy;
	if ((wmpx->mpx_busy == 0) && (wmpx->mpx_state & MPX_WANT)) {
		wmpx->mpx_state &= ~(MPX_WANT | MPX_WANTR);
		wakeup(wmpx);
	} else if (bp->cnt > 0) {
		/*
		 * If we have put any characters in the buffer, we wake up
		 * the reader.
		 */
		if (wmpx->mpx_state & MPX_WANTR) {
			wmpx->mpx_state &= ~MPX_WANTR;
			wakeup(wmpx);
		}
	}

	/*
	 * Don't return EMPX if I/O was successful
	 */
	if (error == EMPX && bp->cnt == 0 && uio->uio_resid == 0) {
		error = 0;
	}

	if (error == 0) {
		MPX_TIMESTAMP(&wmpx->mpx_mtime);
	}

	/*
	 * We have something to offer, wake up select/poll.
	 * wmpx->mpx_map.cnt is always 0 in this point (direct write
	 * is only done synchronously), so check only wmpx->mpx_buffer.cnt
	 */
	if (bp->cnt) {
		mpxselwakeup(wmpx, wmpx);
	}

	/*
	 * Arrange for next read(2) to do a signal.
	 */
	wmpx->mpx_state |= MPX_SIGNALR;

	mpxunlock(wmpx);
	MPX_UNLOCK(wmpx);
	return (error);
}

int
mpx_ioctl(fp, cmd, data, p)
	struct file *fp;
	u_long cmd;
	caddr_t data;
	struct proc *p;
{
	struct mpx 			*mpx;
	struct mpx_group 	*grp;
	struct mpx_channel 	*chan;

	mpx = fp->f_mpx;
	if(mpx->mpx_file != fp) {
		mpx->mpx_file = fp;
	}

	switch (cmd) {
	case FIONBIO:
		return (0);

	case FIOASYNC:
	case TIOCSPGRP:
	case FIOSETOWN:
		return fsetown(mpx->mpx_file, cmd, data);

	case TIOCGPGRP:
	case FIOGETOWN:
		return fgetown(mpx->mpx_file, cmd, data);
	}
	return (0);
}

int
mpx_poll(fp, event, p)
	struct file *fp;
	int event;
	struct proc *p;
{
	struct mpx *rmpx, *wmpx;
	int eof, revents;

	rmpx = (struct mpx *)kn->kn_fp->f_data;
	eof = 0;
	revents = 0;

retry:
	MPX_LOCK(rmpx);
	wmpx = rmpx->mpx_peer;
	if (wmpx != NULL && simple_lock_try(&wmpx->mpx_slock) == 0) {
		/* Deal with race for peer */
		MPX_UNLOCK(rmpx);
		goto retry;
	}

	if (event & (POLLIN | POLLRDNORM)) {
		if ((rmpx->mpx_buffer.cnt > 0) || (rmpx->mpx_state & MPX_EOF)) {
			revents |= event & (POLLIN | POLLRDNORM);
		}
	}

	eof |= (rmpx->mpx_state & MPX_EOF);
	MPX_UNLOCK(rmpx);

	if (wmpx == NULL) {
		revents |= event & (POLLOUT | POLLWRNORM);
	} else {
		if (event & (POLLOUT | POLLWRNORM)) {
			if ((wmpx->mpx_state & MPX_EOF)
					|| ((wmpx->mpx_buffer.size - wmpx->mpx_buffer.cnt)
							>= MPX_BUF)) {
				revents |= event & (POLLOUT | POLLWRNORM);
			}
		}

		eof |= (wmpx->mpx_state & MPX_EOF);
		MPX_UNLOCK(wmpx);
	}

	if (wmpx == NULL || eof)
		revents |= POLLHUP;

	if (revents == 0) {
		if (event & (POLLIN | POLLRDNORM)) {
			selrecord(p, &rmpx->mpx_sel);
		}

		if (event & (POLLOUT | POLLWRNORM)) {
			selrecord(p, &wmpx->mpx_sel);
		}
	}

	return (revents);
}

int
mpx_close(fp, p)
	struct file *fp;
	struct proc *p;
{
	struct mpx 	*mpx;
	mpx = fp->f_mpx;

	fp->f_mpx = NULL;
	mpxclose(fp, mpx);
	return (0);
}

static void
mpxclose(fp, mpx)
	struct file *fp;
	struct mpx 	*mpx;
{
	struct mpx 	*mmpx;

	if (mpx == NULL) {
		return;
	}

retry:
	MPX_LOCK(mpx);

	mpxselwakeup(mpx, mpx);

	/*
	 * If the other side is blocked, wake it up saying that
	 * we want to close it down.
	 */
	mpx->mpx_state |= MPX_EOF;
	while (mpx->mpx_busy) {
		wakeup(mpx);
		mpx->mpx_state |= MPX_WANT;
		ltsleep(mpx, PSOCK, "mpxcl", 0, &mpx->mpx_slock);
	}

	/*
	 * Disconnect from peer
	 */
	if ((mmpx = mpx->mpx_peer) != NULL) {
		/* Deal with race for peer */
		if (simple_lock_try(&mmpx->mpx_slock) == 0) {
			MPX_UNLOCK(mpx);
			goto retry;
		}
		mpxselwakeup(mmpx, mmpx);

		mmpx->mpx_state |= MPX_EOF;
		wakeup(mmpx);
		mmpx->mpx_peer = NULL;
		MPX_UNLOCK(mmpx);
	}

	(void) lockmgr(&mpx->mpx_lock, LK_DRAIN | LK_INTERLOCK, &mpx->mpx_slock, LOCKHOLDER_PID(&mpx->mpx_lock.lk_lockholder));

	/*
	 * free resources
	 */
	mpx_free_kmem(mpx);
	free(mpx, M_MPX);
}

int
mpx_kqfilter(fp, kn)
	struct file *fp;
	struct knote *kn;
{
	struct mpx 	*mpx;

	mpx = (struct mpx *)kn->kn_fp->f_data;
	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &mpx_rfiltops;
		break;
	case EVFILT_WRITE:
		kn->kn_fop = &mpx_wfiltops;
		mpx = mpx->mpx_peer;
		break;
	default:
		return (1);
	}
	kn->kn_hook = mpx;

	MPX_LOCK(mpx);
	SIMPLEQ_INSERT_HEAD(&mpx->mpx_sel.si_klist, kn, kn_selnext);
	MPX_UNLOCK(mpx);
	return (0);
}

static void
filt_mpxdetach(kn)
	struct knote *kn;
{
	struct mpx *mpx = (struct mpx *)kn->kn_fp->f_data;

	MPX_LOCK(mpx);
	SIMPLEQ_REMOVE(&mpx->mpx_sel.si_klist, kn, knote, kn_selnext);
	MPX_UNLOCK(mpx);
}

/*ARGSUSED*/
static int
filt_mpxread(kn, hint)
	struct knote *kn;
	long hint;
{
	struct mpx *rmpx, *wmpx;

	rmpx = (struct mpx *)kn->kn_fp->f_data;
	wmpx = rmpx->mpx_peer;

	MPX_LOCK(rmpx);
	kn->kn_data = rmpx->mpx_buffer.cnt;
	if ((kn->kn_data == 0) && (rmpx->mpx_state & MPX_DIRECTW))
		kn->kn_data = rmpx->mpx_map.cnt;

	if ((rmpx->mpx_state & MPX_EOF) ||
	    (wmpx == NULL) || (wmpx->mpx_state & MPX_EOF)) {
		kn->kn_flags |= EV_EOF;
		MPX_UNLOCK(rmpx);
		return (1);
	}
	MPX_UNLOCK(rmpx);
	return (kn->kn_data > 0);
}

/*ARGSUSED*/
static int
filt_mpxwrite(kn, hint)
	struct knote *kn;
	long hint;
{
	struct mpx *rmpx, *wmpx;

	rmpx = (struct mpx*)kn->kn_fp->f_data;
	wmpx = rmpx->mpx_peer;

	MPX_LOCK(rmpx);
	if ((wmpx == NULL) || (wmpx->mpx_state & MPX_EOF)) {
		kn->kn_data = 0;
		kn->kn_flags |= EV_EOF;
		MPX_UNLOCK(rmpx);
		return (1);
	}
	kn->kn_data = wmpx->mpx_buffer.size - wmpx->mpx_buffer.cnt;
	if (wmpx->mpx_state & MPX_DIRECTW) {
		kn->kn_data = 0;
	}

	MPX_UNLOCK(rmpx);
	return (kn->kn_data >= MPX_BUF);
}

static void
mpx_free_kmem(mpx)
	struct mpx *mpx;
{
	if (mpx->mpx_buffer.buffer != NULL) {
		if (mpx->mpx_buffer.size > MPX_SIZE) {
			--nbigmpx;
		}
		amountmpxkva -= mpx->mpx_buffer.size;
		kmem_free(kernel_map, (caddr_t)mpx->mpx_buffer.buffer, mpx->mpx_buffer.size);
		mpx->mpx_buffer.buffer = NULL;
	}
}

struct mpxpair *
mpx_paircreate(void)
{
	struct mpxpair *mm;
	struct mpx *rmpx, *wmpx;

	mm = (struct mpxpair *)malloc(sizeof(struct mpxpair *), M_MPX, M_WAITOK);

	rmpx = &mm->mpp_rmpx;
	wmpx = &mm->mpp_wmpx;

	error = mpx_create(rmpx, TRUE);
	if(error != 0) {
		goto fail;
	}
	error = mpx_create(wmpx, FALSE);
	if(error != 0) {
		mpx_free_kmem(rmpx);
		goto fail;
	}

	return (mm);

fail:
	free(mm, M_MPX);
	return (NULL);
}
