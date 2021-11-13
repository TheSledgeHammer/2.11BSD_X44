/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_ihash.c	8.7 (Berkeley) 5/17/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <ufs/ufs211/ufs211_extern.h>
#include <ufs/ufs211/ufs211_inode.h>

LIST_HEAD(ufs211_ihashhead, ufs211_inode) *ihashtbl;	/* inode LRU cache, stolen */
u_long	ihash;											/* size of hash table - 1 */
#define	INOHSZ				16							/* must be power of two */
#define	INOHASH(dev,ino)	(&ihashtbl[((dev) + (ino)) & ihash & (INOHSZ * (dev + ihash) - 1)])
struct lock_object			ufs211_ihash;

/*
 * Initialize hash links for inodes
 * and build inode free list.
 */
void
ufs211_ihinit()
{
	ihashtbl = hashinit(desiredvnodes, M_UFS211, &ihash);
	simple_lock_init(&ufs211_ihash, "ufs211_ihash");
}

/*
 * Find an inode if it is incore.
 */
struct vnode  *
ufs211_ihashfind(dev, ino)
	register dev_t dev;
	register ino_t ino;
{
	register struct ufs211_inode *ip;

	simple_lock(&ufs211_ihash);
	for (ip = LIST_FIRST(INOHASH(dev, ino)); ip; ip = LIST_NEXT(ip, i_chain)) {
		if (ino == ip->i_number && dev == ip->i_dev) {
			break;
		}
	}
	simple_unlock(&ufs211_ihash);
	if (ip) {
		return (UFS211_ITOV(ip));
	}
	return (NULL);
}

/*
 * Use the device/inum pair to find the incore inode, and return a pointer
 * to it. If it is in core, but locked, wait for it.
 */
struct vnode *
ufs211_ihashget(dev, ino)
	dev_t dev;
	ino_t ino;
{
	struct proc *p = curproc;	/* XXX */
	struct ufs211_inode *ip;
	struct vnode *vp;

loop:
	simple_lock(&ufs211_ihash);
	for (ip = LIST_FIRST(INOHASH(dev, ino)); ip; ip = LIST_NEXT(ip, i_chain)) {
		if (ino == ip->i_number && dev == ip->i_dev) {
			vp = UFS211_ITOV(ip);
			simple_lock(&vp->v_interlock);
			simple_unlock(&ufs211_ihash);
			if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK, p))
				goto loop;
			return (vp);
		}
	}
	simple_unlock(&ufs211_ihash);
	return (NULL);
}

/*
* Insert the inode into the hash table, and return it locked.
 */
void
ufs211_ihashins(ip)
	struct ufs211_inode *ip;
{
	struct proc *p = curproc;		/* XXX */
	struct ufs211_ihashhead *ipp;

	/* lock the inode, then put it on the appropriate hash list */
	lockmgr(&ip->i_lock, LK_EXCLUSIVE, &ufs211_ihash, p);

	simple_lock(&ufs211_ihash);
	ipp = INOHASH(ip->i_dev, ip->i_number);
	LIST_INSERT_HEAD(ipp, ip, i_chain);
	simple_unlock(&ufs211_ihash);
}

/*
 * Remove the inode from the hash table.
 */
void
ufs211_ihashrem(ip)
	struct ufs211_inode *ip;
{
	struct ufs211_inode *iq;

	simple_lock(&ufs211_ihash);
	LIST_REMOVE(ip, i_chain);
#ifdef DIAGNOSTIC
	ip->i_chain.le_next = NULL;
	ip->i_chain.le_prev = NULL;
#endif
	simple_unlock(&ufs211_ihash);
}
