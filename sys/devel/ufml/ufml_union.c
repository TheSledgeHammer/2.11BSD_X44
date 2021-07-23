/*
 * Copyright (c) 1994 Jan-Simon Pendry
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)union_subr.c	8.20 (Berkeley) 5/20/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/queue.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/user.h>

#include <vm/include/vm.h>		/* for vnode_pager_setsize */

#include <devel/ufml/ufml.h>
#include <devel/ufml/ufml_union.h>

/* must be power of two, otherwise change UFML_HASH() */
#define NHASH 			32
#define LOG2_SIZEVNODE 	8		/* log2(sizeof struct vnode) */

/* unsigned int ... */
#define UFML_HASH(u, l) \
	(((((unsigned long) (u)) + ((unsigned long) l)) >> LOG2_SIZEVNODE) & (NHASH-1))

static LIST_HEAD(ufml_head, ufml_node) ufml_head[NHASH];
static int unvplock[NHASH];

ufml_init()
{
	int i;
	for (i = 0; i < NHASH; i++) {
		LIST_INIT(&ufml_head[i]);
	}
	bzero((caddr_t) unvplock, sizeof(unvplock));
	uops_init(); 									/* start ufml vector operations */
}

static int
ufml_list_lock(ix)
	int ix;
{
	if (unvplock[ix] & UFML_LOCKED) {
		unvplock[ix] |= UFML_WANT;
		sleep((caddr_t) &unvplock[ix], PINOD);
		return (1);
	}

	unvplock[ix] |= UFML_LOCKED;

	return (0);
}

static void
ufml_list_unlock(ix)
	int ix;
{
	unvplock[ix] &= ~UFML_LOCKED;

	if (unvplock[ix] & UFML_WANT) {
		unvplock[ix] &= ~UFML_WANT;
		wakeup((caddr_t) &unvplock[ix]);
	}
}

void
ufml_updatevp(un, uppervp, lowervp)
	struct ufml_node 	*un;
	struct vnode 		*uppervp;
	struct vnode 		*lowervp;
{
	struct ufml_union_node *uun = UFML_UNION(un);

	int ohash = UFML_HASH(uun->uun_uppervp, uun->uun_lowervp);
	int nhash = UFML_HASH(uppervp, lowervp);
	int docache = (lowervp != NULLVP || uppervp != NULLVP);
	int lhash, hhash, uhash;

	/*
	 * Ensure locking is ordered from lower to higher
	 * to avoid deadlocks.
	 */
	if (nhash < ohash) {
		lhash = nhash;
		uhash = ohash;
	} else {
		lhash = ohash;
		uhash = nhash;
	}

	if (lhash != uhash)
		while (ufml_list_lock(lhash))
			continue;

	while (ufml_list_lock(uhash))
		continue;

	if (ohash != nhash || !docache) {
		if (uun->uun_flags & UFML_CACHED) {
			uun->uun_flags &= ~UFML_CACHED;
			LIST_REMOVE(un, ufml_cache);
		}
	}

	if (ohash != nhash)
		ufml_list_unlock(ohash);

	if (uun->uun_lowervp != lowervp) {
		if (uun->uun_lowervp) {
			vrele(uun->uun_lowervp);
			if (uun->uun_path) {
				free(uun->uun_path, M_TEMP);
				uun->uun_path = 0;
			}
			if (uun->uun_dirvp) {
				vrele(uun->uun_dirvp);
				uun->uun_dirvp = NULLVP;
			}
		}
		uun->uun_lowervp = lowervp;
		uun->uun_lowersz = VNOVAL;
	}

	if (uun->uun_uppervp != uppervp) {
		if (uun->uun_uppervp)
			vrele(uun->uun_uppervp);

		uun->uun_uppervp = uppervp;
		uun->uun_uppersz = VNOVAL;
	}

	if (docache && (ohash != nhash)) {
		LIST_INSERT_HEAD(&ufml_head[nhash], un, ufml_cache);
		uun->uun_flags |= UFML_CACHED;
	}

	ufml_list_unlock(nhash);
}

void
ufml_newlower(un, lowervp)
	struct ufml_node *un;
	struct vnode *lowervp;
{
	struct ufml_union_node *uun = UFML_UNION(un);
	ufml_updatevp(un, uun->uun_uppervp, lowervp);
}

void
ufml_newupper(un, uppervp)
	struct ufml_node *un;
	struct vnode *uppervp;
{
	struct ufml_union_node *uun = UFML_UNION(un);
	ufml_updatevp(un, uppervp, uun->uun_lowervp);
}

/*
 * Keep track of size changes in the underlying vnodes.
 * If the size changes, then callback to the vm layer
 * giving priority to the upper layer size.
 */
void
ufml_newsize(vp, uppersz, lowersz)
	struct vnode *vp;
	off_t uppersz, lowersz;
{
	struct ufml_node *un;
	off_t sz;
	struct ufml_union_node *uun = UFML_UNION(un);

	/* only interested in regular files */
	if (vp->v_type != VREG)
		return;

	un = VTOUFML(vp);
	sz = VNOVAL;

	if ((uppersz != VNOVAL) && (uun->uun_uppersz != uppersz)) {
		uun->uun_uppersz = uppersz;
		if (sz == VNOVAL)
			sz = uun->uun_uppersz;
	}

	if ((lowersz != VNOVAL) && (uun->uun_lowersz != lowersz)) {
		uun->uun_lowersz = lowersz;
		if (sz == VNOVAL)
			sz = uun->uun_lowersz;
	}

	if (sz != VNOVAL) {
#ifdef UNION_DIAGNOSTIC
		printf("ufml: %s size now %ld\n",
			uppersz != VNOVAL ? "upper" : "lower", (long) sz);
#endif
		vnode_pager_setsize(vp, sz);
	}
}

/*
 * allocate a ufml_node/vnode pair.  the vnode is
 * referenced and locked.  the new vnode is returned
 * via (vpp).  (mp) is the mountpoint of the union filesystem,
 * (dvp) is the parent directory where the upper layer object
 * should exist (but doesn't) and (cnp) is the componentname
 * information which is partially copied to allow the upper
 * layer object to be created at a later time.  (uppervp)
 * and (lowervp) reference the upper and lower layer objects
 * being mapped.  either, but not both, can be nil.
 * if supplied, (uppervp) is locked.
 * the reference is either maintained in the new ufml_node
 * object which is allocated, or they are vrele'd.
 *
 * all union_nodes are maintained on a singly-linked
 * list.  new nodes are only allocated when they cannot
 * be found on this list.  entries on the list are
 * removed when the vfs reclaim entry is called.
 *
 * a single lock is kept for the entire list.  this is
 * needed because the getnewvnode() function can block
 * waiting for a vnode to become free, in which case there
 * may be more than one process trying to get the same
 * vnode.  this lock is only taken if we are going to
 * call getnewvnode, since the kernel itself is single-threaded.
 *
 * if an entry is found on the list, then call vget() to
 * take a reference.  this is done because there may be
 * zero references to it and so it needs to removed from
 * the vnode free list.
 */
int
ufml_allocvp(vpp, mp, undvp, dvp, cnp, uppervp, lowervp, docache)
	struct vnode **vpp;
	struct mount *mp;
	struct vnode *undvp;		/* parent union vnode */
	struct vnode *dvp;			/* may be null */
	struct componentname *cnp;	/* may be null */
	struct vnode *uppervp;		/* may be null */
	struct vnode *lowervp;		/* may be null */
	int docache;
{
	int error;
	struct ufml_node *un;
	struct ufml_node **pp;
	struct vnode *xlowervp = NULLVP;
	struct ufml_mount *um = MOUNTTOUFMLMOUNT(mp);
	int hash;
	int vflag;
	int try;

	struct ufml_union_node *uun = UFML_UNION(un);
	struct ufml_union_mount *uum = UFML_UNIONMNT(um);

	if (uppervp == NULLVP && lowervp == NULLVP)
		panic("ufml: unidentifiable allocation");

	if (uppervp && lowervp && (uppervp->v_type != lowervp->v_type)) {
		xlowervp = lowervp;
		lowervp = NULLVP;
	}

	/* detect the root vnode (and aliases) */
	vflag = 0;
	if ((uppervp == uum->uum_uppervp) &&
	    ((lowervp == NULLVP) || lowervp == uum->uum_lowervp)) {
		if (lowervp == NULLVP) {
			lowervp = uum->uum_lowervp;
			if (lowervp != NULLVP)
				VREF(lowervp);
		}
		vflag = VROOT;
	}

loop:
	if (!docache) {
		un = 0;
	} else for (try = 0; try < 3; try++) {
		switch (try) {
		case 0:
			if (lowervp == NULLVP)
				continue;
			hash = UFML_HASH(uppervp, lowervp);
			break;

		case 1:
			if (uppervp == NULLVP)
				continue;
			hash = UFML_HASH(uppervp, NULLVP);
			break;

		case 2:
			if (lowervp == NULLVP)
				continue;
			hash = UFML_HASH(NULLVP, lowervp);
			break;
		}

		while (ufml_list_lock(hash))
			continue;

		for (un = LIST_FIRST(ufml_head[hash]); un != 0; un = LIST_NEXT(un, ufml_cache)) {
			if ((uun->uun_lowervp == lowervp ||
			     uun->uun_lowervp == NULLVP) &&
			    (uun->uun_uppervp == uppervp ||
			     uun->uun_uppervp == NULLVP) &&
			    (UFMLTOV(un)->v_mount == mp)) {
				if (vget(UFMLTOV(un), 0,
				    cnp ? cnp->cn_proc : NULL)) {
					ufml_list_unlock(hash);
					goto loop;
				}
				break;
			}
		}

		ufml_list_unlock(hash);

		if (un)
			break;
	}

	if (un) {
		/*
		 * Obtain a lock on the ufml_node.
		 * uppervp is locked, though uun->uun_uppervp
		 * may not be.  this doesn't break the locking
		 * hierarchy since in the case that uun->uun_uppervp
		 * is not yet locked it will be vrele'd and replaced
		 * with uppervp.
		 */

		if ((dvp != NULLVP) && (uppervp == dvp)) {
			/*
			 * Access ``.'', so (un) will already
			 * be locked.  Since this process has
			 * the lock on (uppervp) no other
			 * process can hold the lock on (un).
			 */
#ifdef DIAGNOSTIC
			if ((uun->uun_flags & UN_LOCKED) == 0)
				panic("ufml: . not locked");
			else if (curproc && uun->uun_pid != curproc->p_pid &&
				    uun->uun_pid > -1 && curproc->p_pid > -1)
				panic("ufml: allocvp not lock owner");
#endif
		} else {
			if (uun->uun_flags & UFML_LOCKED) {
				vrele(UFMLTOV(un));
				uun->uun_flags |= UFML_WANT;
				sleep((caddr_t) &uun->uun_flags, PINOD);
				goto loop;
			}
			uun->uun_flags |= UFML_LOCKED;

#ifdef DIAGNOSTIC
			if (curproc)
				uun->uun_pid = curproc->p_pid;
			else
				uun->uun_pid = -1;
#endif
		}

		/*
		 * At this point, the ufml_node is locked,
		 * uun->uun_uppervp may not be locked, and uppervp
		 * is locked or nil.
		 */

		/*
		 * Save information about the upper layer.
		 */
		if (uppervp != uun->uun_uppervp) {
			ufml_dircache_r(un, uppervp);
		} else if (uppervp) {
			vrele(uppervp);
		}

		if (uun->uun_uppervp) {
			uun->uun_flags |= UFML_ULOCK;
			uun->uun_flags &= ~UFML_KLOCK;
		}

		/*
		 * Save information about the lower layer.
		 * This needs to keep track of pathname
		 * and directory information which ufml_vn_create
		 * might need.
		 */
		if (lowervp != uun->uun_lowervp) {
			ufml_newlower(un, lowervp);
			if (cnp && (lowervp != NULLVP)) {
				un->ufml_cache = cnp->cn_hash;
				uun->uun_path = malloc(cnp->cn_namelen+1, M_TEMP, M_WAITOK);
				bcopy(cnp->cn_nameptr, uun->uun_path,
						cnp->cn_namelen);
				uun->uun_path[cnp->cn_namelen] = '\0';
				VREF(dvp);
				uun->uun_dirvp = dvp;
			}
		} else if (lowervp) {
			vrele(lowervp);
		}
		*vpp = UFMLTOV(un);
		return (0);
	}

	if (docache) {
		/*
		 * otherwise lock the vp list while we call getnewvnode
		 * since that can block.
		 */
		hash = UFML_HASH(uppervp, lowervp);

		if (ufml_list_lock(hash))
			goto loop;
	}

	error = getnewvnode(VT_UNION, mp, ufml_vnodeops, vpp);
	if (error) {
		if (uppervp) {
			if (dvp == uppervp)
				vrele(uppervp);
			else
				vput(uppervp);
		}
		if (lowervp)
			vrele(lowervp);

		goto out;
	}

	MALLOC((*vpp)->v_data, void *, sizeof(struct ufml_node), M_TEMP, M_WAITOK);

	(*vpp)->v_flag |= vflag;
	if (uppervp)
		(*vpp)->v_type = uppervp->v_type;
	else
		(*vpp)->v_type = lowervp->v_type;
	un = VTOUFML(*vpp);
	un->ufml_vnode = *vpp;
	uun->uun_uppervp = uppervp;
	uun->uun_uppersz = VNOVAL;
	uun->uun_lowervp = lowervp;
	uun->uun_lowersz = VNOVAL;
	uun->uun_pvp = undvp;
	if (undvp != NULLVP)
		VREF(undvp);
	uun->uun_dircache = 0;
	uun->uun_openl = 0;
	uun->uun_flags = UFML_LOCKED;
	if (uun->uun_uppervp)
		uun->uun_flags |= UFML_ULOCK;
#ifdef DIAGNOSTIC
	if (curproc)
		uun->uun_pid = curproc->p_pid;
	else
		uun->uun_pid = -1;
#endif
	if (cnp && (lowervp != NULLVP)) {
		uun->uun_hash = cnp->cn_hash;
		uun->uun_path = malloc(cnp->cn_namelen+1, M_TEMP, M_WAITOK);
		bcopy(cnp->cn_nameptr, uun->uun_path, cnp->cn_namelen);
		uun->uun_path[cnp->cn_namelen] = '\0';
		VREF(dvp);
		uun->uun_dirvp = dvp;
	} else {
		uun->uun_hash = 0;
		uun->uun_path = 0;
		uun->uun_dirvp = 0;
	}

	if (docache) {
		LIST_INSERT_HEAD(&ufml_head[hash], un, ufml_cache);
		uun->uun_flags |= UFML_CACHED;
	}

	if (xlowervp)
		vrele(xlowervp);

out:
	if (docache)
		ufml_list_unlock(hash);

	return (error);
}

int
ufml_freevp(vp)
	struct vnode *vp;
{
	struct ufml_node *un = VTOUFML(vp);
	struct ufml_union_node *uun = UFML_UNION(un);

	if (uun->uun_flags & UFML_CACHED) {
		uun->uun_flags &= ~UFML_CACHED;
		LIST_REMOVE(un, ufml_cache);
	}

	if (uun->uun_pvp != NULLVP)
		vrele(uun->uun_pvp);
	if (uun->uun_uppervp != NULLVP)
		vrele(uun->uun_uppervp);
	if (uun->uun_lowervp != NULLVP)
		vrele(uun->uun_lowervp);
	if (uun->uun_dirvp != NULLVP)
		vrele(uun->uun_dirvp);
	if (uun->uun_path)
		free(uun->uun_path, M_TEMP);

	FREE(vp->v_data, M_TEMP);
	vp->v_data = 0;

	return (0);
}

/*
 * copyfile.  copy the vnode (fvp) to the vnode (tvp)
 * using a sequence of reads and writes.  both (fvp)
 * and (tvp) are locked on entry and exit.
 */
int
ufml_copyfile(fvp, tvp, cred, p)
	struct vnode *fvp;
	struct vnode *tvp;
	struct ucred *cred;
	struct proc *p;
{
	char *buf;
	struct uio uio;
	struct iovec iov;
	int error = 0;

	/*
	 * strategy:
	 * allocate a buffer of size MAXBSIZE.
	 * loop doing reads and writes, keeping track
	 * of the current uio offset.
	 * give up at the first sign of trouble.
	 */

	uio.uio_procp = p;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = 0;

	VOP_UNLOCK(fvp, 0, p);				/* XXX */
	VOP_LEASE(fvp, p, cred, LEASE_READ);
	vn_lock(fvp, LK_EXCLUSIVE | LK_RETRY, p);	/* XXX */
	VOP_UNLOCK(tvp, 0, p);				/* XXX */
	VOP_LEASE(tvp, p, cred, LEASE_WRITE);
	vn_lock(tvp, LK_EXCLUSIVE | LK_RETRY, p);	/* XXX */

	buf = malloc(MAXBSIZE, M_TEMP, M_WAITOK);

	/* ugly loop follows... */
	do {
		off_t offset = uio.uio_offset;

		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		iov.iov_base = buf;
		iov.iov_len = MAXBSIZE;
		uio.uio_resid = iov.iov_len;
		uio.uio_rw = UIO_READ;
		error = VOP_READ(fvp, &uio, 0, cred);

		if (error == 0) {
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			iov.iov_base = buf;
			iov.iov_len = MAXBSIZE - uio.uio_resid;
			uio.uio_offset = offset;
			uio.uio_rw = UIO_WRITE;
			uio.uio_resid = iov.iov_len;

			if (uio.uio_resid == 0)
				break;

			do {
				error = VOP_WRITE(tvp, &uio, 0, cred);
			} while ((uio.uio_resid > 0) && (error == 0));
		}

	} while (error == 0);

	free(buf, M_TEMP);
	return (error);
}

/*
 * (un) is assumed to be locked on entry and remains
 * locked on exit.
 */
int
ufml_copyup(un, docopy, cred, p)
	struct ufml_node *un;
	int docopy;
	struct ucred *cred;
	struct proc *p;
{
	int error;
	struct vnode *lvp, *uvp;
	struct ufml_union_node *uun = UFML_UNION(un);

	error = ufml_vn_create(&uvp, un, p);
	if (error)
		return (error);

	/* at this point, uppervp is locked */
	ufml_dircache_r(un, uvp);
	uun->uun_flags |= UFML_ULOCK;

	lvp = uun->uun_lowervp;

	if (docopy) {
		/*
		 * XX - should not ignore errors
		 * from VOP_CLOSE
		 */
		vn_lock(lvp, LK_EXCLUSIVE | LK_RETRY, p);
		error = VOP_OPEN(lvp, FREAD, cred, p);
		if (error == 0) {
			error = ufml_copyfile(lvp, uvp, cred, p);
			VOP_UNLOCK(lvp, 0, p);
			(void) VOP_CLOSE(lvp, FREAD, cred, p);
		}
#ifdef UNION_DIAGNOSTIC
		if (error == 0)
			uprintf("ufml: copied up %s\n", uun->uun_path);
#endif

	}
	uun->uun_flags &= ~UFML_ULOCK;
	VOP_UNLOCK(uvp, 0, p);
	ufml_vn_close(uvp, FWRITE, cred, p);
	vn_lock(uvp, LK_EXCLUSIVE | LK_RETRY, p);
	uun->uun_flags |= UFML_ULOCK;

	/*
	 * Subsequent IOs will go to the top layer, so
	 * call close on the lower vnode and open on the
	 * upper vnode to ensure that the filesystem keeps
	 * its references counts right.  This doesn't do
	 * the right thing with (cred) and (FREAD) though.
	 * Ignoring error returns is not right, either.
	 */
	if (error == 0) {
		int i;

		for (i = 0; i < uun->uun_openl; i++) {
			(void) VOP_CLOSE(lvp, FREAD, cred, p);
			(void) VOP_OPEN(uvp, FREAD, cred, p);
		}
		uun->uun_openl = 0;
	}

	return (error);

}

static int
ufml_relookup(um, dvp, vpp, cnp, cn, path, pathlen)
	struct ufml_mount *um;
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
	struct componentname *cn;
	char *path;
	int pathlen;
{
	int error;
	struct ufml_union_mount *uum = UFML_UNIONMNT(um);
	/*
	 * A new componentname structure must be faked up because
	 * there is no way to know where the upper level cnp came
	 * from or what it is being used for.  This must duplicate
	 * some of the work done by NDINIT, some of the work done
	 * by namei, some of the work done by lookup and some of
	 * the work done by VOP_LOOKUP when given a CREATE flag.
	 * Conclusion: Horrible.
	 *
	 * The pathname buffer will be FREEed by VOP_MKDIR.
	 */
	cn->cn_namelen = pathlen;
	cn->cn_pnbuf = malloc(cn->cn_namelen+1, M_NAMEI, M_WAITOK);
	bcopy(path, cn->cn_pnbuf, cn->cn_namelen);
	cn->cn_pnbuf[cn->cn_namelen] = '\0';

	cn->cn_nameiop = CREATE;
	cn->cn_flags = (LOCKPARENT|HASBUF|SAVENAME|SAVESTART|ISLASTCN);
	cn->cn_proc = cnp->cn_proc;
	if (uum->uum_op == UNMNT_ABOVE)
		cn->cn_cred = cnp->cn_cred;
	else
		cn->cn_cred = uum->uum_cred;
	cn->cn_nameptr = cn->cn_pnbuf;
	cn->cn_hash = cnp->cn_hash;
	cn->cn_consume = cnp->cn_consume;

	VREF(dvp);
	error = relookup(dvp, vpp, cn);
	if (!error)
		vrele(dvp);

	return (error);
}

/*
 * Create a shadow directory in the upper layer.
 * The new vnode is returned locked.
 *
 * (um) points to the union mount structure for access to the
 * the mounting process's credentials.
 * (dvp) is the directory in which to create the shadow directory.
 * it is unlocked on entry and exit.
 * (cnp) is the componentname to be created.
 * (vpp) is the returned newly created shadow directory, which
 * is returned locked.
 */
int
ufml_mkshadow(um, dvp, cnp, vpp)
	struct ufml_mount *um;
	struct vnode *dvp;
	struct componentname *cnp;
	struct vnode **vpp;
{
	int error;
	struct vattr va;
	struct proc *p = cnp->cn_proc;
	struct componentname cn;

	struct ufml_union_mount *uum = UFML_UNIONMNT(um);

	error = ufml_relookup(um, dvp, vpp, cnp, &cn,
			cnp->cn_nameptr, cnp->cn_namelen);
	if (error)
		return (error);

	if (*vpp) {
		VOP_ABORTOP(dvp, &cn);
		VOP_UNLOCK(dvp, 0, p);
		vrele(*vpp);
		*vpp = NULLVP;
		return (EEXIST);
	}

	/*
	 * policy: when creating the shadow directory in the
	 * upper layer, create it owned by the user who did
	 * the mount, group from parent directory, and mode
	 * 777 modified by umask (ie mostly identical to the
	 * mkdir syscall).  (jsp, kb)
	 */

	VATTR_NULL(&va);
	va.va_type = VDIR;
	va.va_mode = uum->uum_cmode;

	/* VOP_LEASE: dvp is locked */
	VOP_LEASE(dvp, p, cn.cn_cred, LEASE_WRITE);

	error = VOP_MKDIR(dvp, vpp, &cn, &va);
	return (error);
}

/*
 * Create a whiteout entry in the upper layer.
 *
 * (um) points to the union mount structure for access to the
 * the mounting process's credentials.
 * (dvp) is the directory in which to create the whiteout.
 * it is locked on entry and exit.
 * (cnp) is the componentname to be created.
 */
int
ufml_mkwhiteout(um, dvp, cnp, path)
	struct ufml_mount *um;
	struct vnode *dvp;
	struct componentname *cnp;
	char *path;
{
	int error;
	struct vattr va;
	struct proc *p = cnp->cn_proc;
	struct vnode *wvp;
	struct componentname cn;

	VOP_UNLOCK(dvp, 0, p);
	error = ufml_relookup(um, dvp, &wvp, cnp, &cn, path, strlen(path));
	if (error) {
		vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY, p);
		return (error);
	}

	if (wvp) {
		VOP_ABORTOP(dvp, &cn);
		vrele(dvp);
		vrele(wvp);
		return (EEXIST);
	}

	/* VOP_LEASE: dvp is locked */
	VOP_LEASE(dvp, p, p->p_ucred, LEASE_WRITE);

	error = VOP_WHITEOUT(dvp, &cn, CREATE);
	if (error)
		VOP_ABORTOP(dvp, &cn);

	vrele(dvp);

	return (error);
}

/*
 * ufml_vn_create: creates and opens a new shadow file
 * on the upper union layer.  this function is similar
 * in spirit to calling vn_open but it avoids calling namei().
 * the problem with calling namei is that a) it locks too many
 * things, and b) it doesn't start at the "right" directory,
 * whereas relookup is told where to start.
 */
int
ufml_vn_create(vpp, un, p)
	struct vnode **vpp;
	struct ufml_node *un;
	struct proc *p;
{
	struct vnode *vp;
	struct ucred *cred = p->p_ucred;
	struct vattr vat;
	struct vattr *vap = &vat;
	int fmode = FFLAGS(O_WRONLY|O_CREAT|O_TRUNC|O_EXCL);
	int error;
	int cmode = UN_FILEMODE & ~p->p_fd->fd_cmask;
	char *cp;
	struct componentname cn;

	struct ufml_union_node *uun = UFML_UNION(un);

	*vpp = NULLVP;

	/*
	 * Build a new componentname structure (for the same
	 * reasons outlines in union_mkshadow).
	 * The difference here is that the file is owned by
	 * the current user, rather than by the person who
	 * did the mount, since the current user needs to be
	 * able to write the file (that's why it is being
	 * copied in the first place).
	 */
	cn.cn_namelen = strlen(uun->uun_path);
	cn.cn_pnbuf = (caddr_t) malloc(cn.cn_namelen, M_NAMEI, M_WAITOK);
	bcopy(uun->uun_path, cn.cn_pnbuf, cn.cn_namelen+1);
	cn.cn_nameiop = CREATE;
	cn.cn_flags = (LOCKPARENT|HASBUF|SAVENAME|SAVESTART|ISLASTCN);
	cn.cn_proc = p;
	cn.cn_cred = p->p_ucred;
	cn.cn_nameptr = cn.cn_pnbuf;
	cn.cn_hash = uun->uun_hash;
	cn.cn_consume = 0;

	VREF(uun->uun_dirvp);
	if (error == relookup(uun->uun_dirvp, &vp, &cn))
		return (error);
	vrele(uun->uun_dirvp);

	if (vp) {
		VOP_ABORTOP(uun->uun_dirvp, &cn);
		if (uun->uun_dirvp == vp)
			vrele(uun->uun_dirvp);
		else
			vput(uun->uun_dirvp);
		vrele(vp);
		return (EEXIST);
	}

	/*
	 * Good - there was no race to create the file
	 * so go ahead and create it.  The permissions
	 * on the file will be 0666 modified by the
	 * current user's umask.  Access to the file, while
	 * it is unioned, will require access to the top *and*
	 * bottom files.  Access when not unioned will simply
	 * require access to the top-level file.
	 * TODO: confirm choice of access permissions.
	 */
	VATTR_NULL(vap);
	vap->va_type = VREG;
	vap->va_mode = cmode;
	VOP_LEASE(uun->uun_dirvp, p, cred, LEASE_WRITE);
	if (error == VOP_CREATE(uun->uun_dirvp, &vp, &cn, vap))
		return (error);

	if (error == VOP_OPEN(vp, fmode, cred, p)) {
		vput(vp);
		return (error);
	}

	vp->v_writecount++;
	*vpp = vp;
	return (0);
}

int
ufml_vn_close(vp, fmode, cred, p)
	struct vnode *vp;
	int fmode;
	struct ucred *cred;
	struct proc *p;
{
	if (fmode & FWRITE)
		--vp->v_writecount;
	return (VOP_CLOSE(vp, fmode, cred, p));
}

void
ufml_removed_upper(un)
	struct ufml_node *un;
{
	struct proc *p = curproc;	/* XXX */
	struct ufml_union_node *uun = UFML_UNION(un);

	ufml_dircache_r(un, NULLVP);
	if (uun->uun_flags & UFML_CACHED) {
		uun->uun_flags &= ~UFML_CACHED;
		LIST_REMOVE(un, ufml_cache);
	}

	if (uun->uun_flags & UFML_ULOCK) {
		uun->uun_flags &= ~UFML_ULOCK;
		VOP_UNLOCK(uun->uun_uppervp, 0, p);
	}
}

#if 0
struct vnode *
ufml_lowervp(vp)
	struct vnode *vp;
{
	struct ufml_node *un = VTOUFML(vp);
	struct ufml_union_node *uun = UFML_UNION(un);

	if ((uun->uun_lowervp != NULLVP) &&
	    (vp->v_type == uun->uun_lowervp->v_type)) {
		if (vget(uun->uun_lowervp, 0) == 0)
			return (uun->uun_lowervp);
	}

	return (NULLVP);
}
#endif

/*
 * determine whether a whiteout is needed
 * during a remove/rmdir operation.
 */
int
ufml_dowhiteout(un, cred, p)
	struct ufml_node *un;
	struct ucred *cred;
	struct proc *p;
{
	struct vattr va;
	struct ufml_union_node *uun = UFML_UNION(un);

	if (uun->uun_lowervp != NULLVP)
		return (1);

	if (VOP_GETATTR(uun->uun_uppervp, &va, cred, p) == 0 &&
	    (va.va_flags & OPAQUE))
		return (1);

	return (0);
}

static void
ufml_dircache_r(vp, vppp, cntp)
	struct vnode *vp;
	struct vnode ***vppp;
	int *cntp;
{
	struct ufml_node *un;
	struct ufml_union_node *uun = UFML_UNION(un);

	if (vp->v_op != ufml_vnodeops) {
		if (vppp) {
			VREF(vp);
			*(*vppp)++ = vp;
			if (--(*cntp) == 0)
				panic("ufml: dircache table too small");
		} else {
			(*cntp)++;
		}

		return;
	}

	un = VTOUFML(vp);
	if (uun->uun_uppervp != NULLVP)
		ufml_dircache_r(uun->uun_uppervp, vppp, cntp);
	if (uun->uun_lowervp != NULLVP)
		ufml_dircache_r(uun->uun_lowervp, vppp, cntp);
}

struct vnode *
ufml_dircache(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	int cnt;
	struct vnode *nvp;
	struct vnode **vpp;
	struct vnode **dircache;
	struct ufml_node *un;
	int error;

	struct ufml_union_node *uun = UFML_UNION(un);

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	dircache = VTOUFML(vp)->ufml_union->uun_dircache;

	nvp = NULLVP;

	if (dircache == 0) {
		cnt = 0;
		ufml_dircache_r(vp, 0, &cnt);
		cnt++;
		dircache = (struct vnode **)
				malloc(cnt * sizeof(struct vnode *),
					M_TEMP, M_WAITOK);
		vpp = dircache;
		ufml_dircache_r(vp, &vpp, &cnt);
		*vpp = NULLVP;
		vpp = dircache + 1;
	} else {
		vpp = dircache;
		do {
			if (*vpp++ == VTOUFML(vp)->ufml_union->uun_uppervp)
				break;
		} while (*vpp != NULLVP);
	}

	if (*vpp == NULLVP)
		goto out;

	vn_lock(*vpp, LK_EXCLUSIVE | LK_RETRY, p);
	VREF(*vpp);
	error = ufml_allocvp(&nvp, vp->v_mount, NULLVP, NULLVP, 0, *vpp, NULLVP, 0);
	if (error)
		goto out;

	VTOUFML(vp)->ufml_union->uun_dircache = 0;
	un = VTOUFML(nvp);
	uun->uun_dircache = dircache;

out:
	VOP_UNLOCK(vp, 0, p);
	return (nvp);
}
