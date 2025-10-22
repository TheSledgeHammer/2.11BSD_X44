/*-
 * Copyright (c) 1991, 1993, 1994
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
 *	@(#)lfs_syscalls.c	8.10 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/sysdecl.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

#define BUMP_FIP(SP) \
	(SP)->fip = (FINFO *) (&(SP)->fip->fi_blocks[(SP)->fip->fi_nblocks])

#define INC_FINFO(SP) ++((SEGSUM *)((SP)->segsum))->ss_nfinfo
#define DEC_FINFO(SP) --((SEGSUM *)((SP)->segsum))->ss_nfinfo

/*
 * Before committing to add something to a segment summary, make sure there
 * is enough room.  S is the bytes added to the summary.
 */
#define	CHECK_SEG(sp, s)				\
 if ((sp)->sum_bytes_left < (s)) {		\
	(void) lfs_writeseg((sp)->fs, sp);	\
}

struct buf *lfs_fakebuf(struct vnode *, int, size_t, caddr_t);
int	lfs_fastvget(struct mount *, ino_t, ufs2_daddr_t, struct vnode **, void *);
int lfs1_fastvget(struct mount *, ino_t, ufs1_daddr_t, struct vnode **, struct ufs1_dinode *);
int lfs2_fastvget(struct mount *, ino_t, ufs2_daddr_t, struct vnode **, struct ufs2_dinode *);

int debug_cleaner = 0;
int clean_vnlocked = 0;
int clean_inlocked = 0;

/*
 * lfs_markv:
 *
 * This will mark inodes and blocks dirty, so they are written into the log.
 * It will block until all the blocks have been written.  The segment create
 * time passed in the block_info and inode_info structures is used to decide
 * if the data is valid for each block (in case some process dirtied a block
 * or inode that is being cleaned between the determination that a block is
 * live and the lfs_markv call).
 *
 *  0 on success
 * -1/errno is return on error.
 */
struct lfs_markv_args {
	syscallarg(fsid_t *)	fsidp;		/* file system */
	syscallarg(BLOCK_INFO *)blkiov;		/* block array */
	syscallarg(int) 		blkcnt;		/* count of block array entries */
};
int
lfs_markv()
{
	register struct proc *p;
	register struct lfs_markv_args *uap;
	int *retval;
	struct segment *sp;
	BLOCK_INFO *blkp;
	IFILE *ifp;
	struct buf *bp, **bpp;
	struct inode *ip;
	struct lfs *fs;
	struct mount *mntp;
	struct vnode *vp;
	fsid_t fsid;
	void *start;
	ino_t lastino;
	daddr_t b_daddr, v_daddr;
	u_long bsize;
	int cnt, error;

	uap = u.u_ap;
	p = u.u_procp;

	if ((error = suser1(p->p_ucred, &p->p_acflag)))
		return (error);

	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))))
		return (error);
	if ((mntp = vfs_getvfs(&fsid)) == NULL)
		return (EINVAL);

	cnt = SCARG(uap, blkcnt);
	start = calloc(cnt, sizeof(BLOCK_INFO), M_SEGMENT, M_WAITOK);
	if ((error = copyin(SCARG(uap, blkiov), start, cnt * sizeof(BLOCK_INFO))))
		goto err1;

	/* Mark blocks/inodes dirty.  */
	fs = VFSTOUFS(mntp)->um_lfs;
	bsize = fs->lfs_bsize;
	error = 0;

	lfs_seglock(fs, SEGM_SYNC | SEGM_CLEAN);
	sp = fs->lfs_sp;
	for (v_daddr = LFS_UNUSED_DADDR, lastino = LFS_UNUSED_INUM,
	    blkp = start; cnt--; ++blkp) {
		/*
		 * Get the IFILE entry (only once) and see if the file still
		 * exists.
		 */
		if (lastino != blkp->bi_inode) {
			if (lastino != LFS_UNUSED_INUM) {
				/* Finish up last file */
				if (sp->fip->fi_nblocks == 0) {
					DEC_FINFO(sp);
					sp->sum_bytes_left +=
					    sizeof(FINFO) - sizeof(ufs1_daddr_t);
				} else {
					lfs_updatemeta(sp);
					BUMP_FIP(sp);
				}

				lfs_writeinode(fs, sp, ip);
				lfs_vunref(vp);
			}

			/* Start a new file */
			CHECK_SEG(sp, sizeof(FINFO));
			sp->sum_bytes_left -= sizeof(FINFO) - sizeof(ufs1_daddr_t);
			INC_FINFO(sp);
			sp->start_lbp = &sp->fip->fi_blocks[0];
			sp->vp = NULL;
			sp->fip->fi_version = blkp->bi_version;
			sp->fip->fi_nblocks = 0;
			sp->fip->fi_ino = blkp->bi_inode;
			lastino = blkp->bi_inode;
			if (blkp->bi_inode == LFS_IFILE_INUM)
				v_daddr = fs->lfs_idaddr;
			else {
				LFS_IENTRY(ifp, fs, blkp->bi_inode, bp);
				v_daddr = ifp->if_daddr;
				brelse(bp);
			}
			if (v_daddr == LFS_UNUSED_DADDR)
				continue;

			/* Get the vnode/inode. */
			if (lfs_fastvget(mntp, blkp->bi_inode, v_daddr, &vp,
			    blkp->bi_lbn == LFS_UNUSED_LBN ? 
			    blkp->bi_bp : NULL)) {
#ifdef DIAGNOSTIC
				printf("lfs_markv: VFS_VGET failed (%d)\n",
				    blkp->bi_inode);
				panic("lfs_markv VFS_VGET FAILED");
#endif
				lastino = LFS_UNUSED_INUM;
				v_daddr = LFS_UNUSED_DADDR;
				continue;
			}
			sp->vp = vp;
			ip = VTOI(vp);
		} else if (v_daddr == LFS_UNUSED_DADDR)
			continue;

		/* If this BLOCK_INFO didn't contain a block, keep going. */
		if (blkp->bi_lbn == LFS_UNUSED_LBN)
			continue;
		if (VOP_BMAP(vp, blkp->bi_lbn, NULL, &b_daddr, NULL) ||
		    b_daddr != blkp->bi_daddr)
			continue;
		/*
		 * If we got to here, then we are keeping the block.  If it
		 * is an indirect block, we want to actually put it in the
		 * buffer cache so that it can be updated in the finish_meta
		 * section.  If it's not, we need to allocate a fake buffer
		 * so that writeseg can perform the copyin and write the buffer.
		 */
		if (blkp->bi_lbn >= 0)	/* Data Block */
			bp = lfs_fakebuf(vp, blkp->bi_lbn, bsize,
			    blkp->bi_bp);
		else {
			bp = getblk(vp, blkp->bi_lbn, bsize, 0, 0);
			if (!(bp->b_flags & (B_DELWRI | B_DONE | B_CACHE)) &&
			    (error = copyin(blkp->bi_bp, bp->b_data,
			    blkp->bi_size)))
				goto err2;
			if ((error = VOP_BWRITE(bp)))
				goto err2;
		}
		while (lfs_gatherblock(sp, bp, NULL));
	}
	if (sp->vp) {
		if (sp->fip->fi_nblocks == 0) {
			DEC_FINFO(sp);
			sp->sum_bytes_left +=
			    sizeof(FINFO) - sizeof(ufs1_daddr_t);
		} else
			lfs_updatemeta(sp);

		lfs_writeinode(fs, sp, ip);
		lfs_vunref(vp);
	}
	(void) lfs_writeseg(fs, sp);
	lfs_segunlock(fs);
	free(start, M_SEGMENT);
	return (error);

/*
 * XXX
 * If we come in to error 2, we might have indirect blocks that were
 * updated and now have bad block pointers.  I don't know what to do
 * about this.
 */

err2:
	lfs_vunref(vp);
	/* Free up fakebuffers */
	for (bpp = --sp->cbpp; bpp >= sp->bpp; --bpp)
		if ((*bpp)->b_flags & B_CALL) {
			brelvp(*bpp);
			free(*bpp, M_SEGMENT);
		} else
			brelse(*bpp);
	lfs_segunlock(fs);
err1:	
	free(start, M_SEGMENT);
	return (error);
}

/*
 * lfs_bmapv:
 *
 * This will fill in the current disk address for arrays of blocks.
 *
 *  0 on success
 * -1/errno is return on error.
 */
struct lfs_bmapv_args {
	syscallarg(fsid_t *) 	fsidp;		/* file system */
	syscallarg(BLOCK_INFO *)blkiov;		/* block array */
	syscallarg(int) 		blkcnt;		/* count of block array entries */
};
int
lfs_bmapv()
{
	register struct proc *p;
	register struct lfs_bmapv_args *uap;
	int *retval;
	BLOCK_INFO *blkp;
	struct mount *mntp;
	struct ufsmount *ump;
	struct vnode *vp;
	fsid_t fsid;
	void *start;
	daddr_t daddr;
	int cnt, error, step;

	uap = u.u_ap;
	p = u.u_procp;

	if ((error = suser1(p->p_ucred, &p->p_acflag)))
		return (error);

	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))))
		return (error);
	if ((mntp = vfs_getvfs(&fsid)) == NULL)
		return (EINVAL);

	cnt = SCARG(uap, blkcnt);
	start = blkp = calloc(cnt, sizeof(BLOCK_INFO), M_SEGMENT, M_WAITOK);
	if ((error = copyin(SCARG(uap, blkiov), blkp, cnt * sizeof(BLOCK_INFO)))) {
		free(blkp, M_SEGMENT);
		return (error);
	}

	for (step = cnt; step--; ++blkp) {
		if (blkp->bi_lbn == LFS_UNUSED_LBN)
			continue;
		/*
		 * A regular call to VFS_VGET could deadlock
		 * here.  Instead, we try an unlocked access.
		 */
		ump = VFSTOUFS(mntp);
		if ((vp =
		    ufs_ihashlookup(ump->um_dev, blkp->bi_inode)) != NULL) {
			if (VOP_BMAP(vp, blkp->bi_lbn, NULL, &daddr, NULL))
				daddr = LFS_UNUSED_DADDR;
		} else if (VFS_VGET(mntp, blkp->bi_inode, &vp))
			daddr = LFS_UNUSED_DADDR;
		else  {
			if (VOP_BMAP(vp, blkp->bi_lbn, NULL, &daddr, NULL))
				daddr = LFS_UNUSED_DADDR;
			vput(vp);
		}
		blkp->bi_daddr = daddr;
        }
	copyout(start, SCARG(uap, blkiov), cnt * sizeof(BLOCK_INFO));
	free(start, M_SEGMENT);
	return (0);
}

/*
 * lfs_segclean:
 *
 * Mark the segment clean.
 *
 *  0 on success
 * -1/errno is return on error.
 */
struct lfs_segclean_args {
	syscallarg(fsid_t *) fsidp;		/* file system */
	syscallarg(u_long) segment;		/* segment number */
}; 
int
lfs_segclean()
{
	register struct proc *p;
	register struct lfs_segclean_args *uap;
	int *retval;
	CLEANERINFO *cip;
	SEGUSE *sup;
	struct buf *bp;
	struct mount *mntp;
	struct lfs *fs;
	fsid_t fsid;
	int error;

	uap = u.u_ap;
	p = u.u_procp;

	if ((error = suser1(p->p_ucred, &p->p_acflag)))
		return (error);

	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))))
		return (error);
	if ((mntp = vfs_getvfs(&fsid)) == NULL)
		return (EINVAL);

	fs = VFSTOUFS(mntp)->um_lfs;

	if (datosn(fs, fs->lfs_curseg) == SCARG(uap, segment))
		return (EBUSY);

	LFS_SEGENTRY(sup, fs, SCARG(uap, segment), bp);
	if (sup->su_flags & SEGUSE_ACTIVE) {
		brelse(bp);
		return (EBUSY);
	}
	fs->lfs_avail += fsbtodb(fs, fs->lfs_ssize) - 1;
	fs->lfs_bfree += (sup->su_nsums * LFS_SUMMARY_SIZE / DEV_BSIZE) +
	    sup->su_ninos * btodb(fs->lfs_bsize);
	sup->su_flags &= ~SEGUSE_DIRTY;
	(void) VOP_BWRITE(bp);

	LFS_CLEANERINFO(cip, fs, bp);
	++cip->clean;
	--cip->dirty;
	(void) VOP_BWRITE(bp);
	wakeup(&fs->lfs_avail);
	return (0);
}

/*
 * lfs_segwait:
 *
 * This will block until a segment in file system fsid is written.  A timeout
 * in milliseconds may be specified which will awake the cleaner automatically.
 * An fsid of -1 means any file system, and a timeout of 0 means forever.
 *
 *  0 on success
 *  1 on timeout
 * -1/errno is return on error.
 */
struct lfs_segwait_args {
	syscallarg(fsid_t *) 		 fsidp;		/* file system */
	syscallarg(struct timeval *) tv;		/* timeout */
};
int
lfs_segwait()
{
	register struct proc *p;
	register struct lfs_segwait_args *uap;
	int *retval;
	//extern int lfs_allclean_wakeup;
	struct mount *mntp;
	struct timeval atv;
	fsid_t fsid;
	void *addr;
	u_long timeout;
	int error, s;

	uap = u.u_ap;
	p = u.u_procp;

	if ((error = suser1(p->p_ucred, &p->p_acflag))) {
		return (error);
}
#ifdef WHEN_QUADS_WORK
	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))))
		return (error);
	if (fsid == (fsid_t)-1)
		addr = &lfs_allclean_wakeup;
	else {
		if ((mntp = vfs_getvfs(&fsid)) == NULL)
			return (EINVAL);
		addr = &VFSTOUFS(mntp)->um_lfs->lfs_nextseg;
	}
#else
	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))))
		return (error);
	if ((mntp = vfs_getvfs(&fsid)) == NULL)
		addr = &lfs_allclean_wakeup;
	else
		addr = &VFSTOUFS(mntp)->um_lfs->lfs_nextseg;
#endif

	if (SCARG(uap, tv)) {
		if ((error = copyin(SCARG(uap, tv), &atv, sizeof(struct timeval))))
			return (error);
		if (itimerfix(&atv))
			return (EINVAL);
		s = splclock();
		timevaladd(&atv, (struct timeval *)&time);
		timeout = hzto(&atv);
		splx(s);
	} else
		timeout = 0;

	error = tsleep(addr, PCATCH | PUSER, "segment", timeout);
	return (error == ERESTART ? EINTR : 0);
}

struct buf *
lfs_fakebuf(vp, lbn, size, uaddr)
	struct vnode *vp;
	int lbn;
	size_t size;
	caddr_t uaddr;
{
	struct buf *bp;

	bp = lfs_newbuf(vp, lbn, 0);
	bp->b_saveaddr = uaddr;
	bp->b_bufsize = size;
	bp->b_bcount = size;
	bp->b_flags |= B_INVAL;
	return (bp);
}

int
lfs_fastvget(mp, ino, daddr, vpp, dinp)
	struct mount *mp;
	ino_t ino;
	ufs2_daddr_t daddr;
	struct vnode **vpp;
	void *dinp;
{
	struct ufs1_dinode *din1;
	struct ufs2_dinode *din2;
	if (UFS1) {
		din1 = (struct ufs1_dinode *)dinp;
		return (lfs1_fastvget(mp, ino, (ufs1_daddr_t)daddr, vpp, din1));
	} else {
		din2 = (struct ufs2_dinode *)dinp;
		return (lfs2_fastvget(mp, ino, daddr, vpp, din2));
	}
}

/*
 * VFS_VGET call specialized for the cleaner.  The cleaner already knows the
 * daddr from the ifile, so don't look it up again.  If the cleaner is
 * processing IINFO structures, it may have the ondisk inode already, so
 * don't go retrieving it again.
 */
int
lfs1_fastvget(mp, ino, daddr, vpp, dinp)
	struct mount *mp;
	ino_t ino;
	ufs1_daddr_t daddr;
	struct vnode **vpp;
	struct ufs1_dinode *dinp;
{
	register struct inode *ip;
	struct ufs1_dinode *dip;
	struct vnode *vp;
	struct ufsmount *ump;
	struct buf *bp;
	struct vnodeops *fifoops;
	dev_t dev;
	int error, retries;
#ifdef FIFO
	fifoops = &lfs_fifoops
#else
	fifoops = NULL;
#endif
	ump = VFSTOUFS(mp);
	dev = ump->um_dev;
	
	/*
	 * This is playing fast and loose.  Someone may have the inode
	 * locked, in which case they are going to be distinctly unhappy
	 * if we trash something.
	 */
	if ((*vpp = ufs_ihashlookup(dev, ino)) != NULL) {
		lfs_vref(*vpp);
		if ((*vpp)->v_flag & VXLOCK)
			clean_vnlocked++;
		ip = VTOI(*vpp);
		if (lockstatus(&ip->i_lock))
			clean_inlocked++;
		if (!(ip->i_flag & IN_MODIFIED))
			++ump->um_lfs->lfs_uinodes;
		ip->i_flag |= IN_MODIFIED;
		return (0);
	}

	/* Allocate new vnode/inode. */
	if ((error = lfs_vcreate(mp, ino, &vp))) {
		*vpp = NULL;
		return (error);
	}

	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	ip = VTOI(vp);
	ufs_ihashins(ip);

	/*
	 * XXX
	 * This may not need to be here, logically it should go down with
	 * the i_devvp initialization.
	 * Ask Kirk.
	 */
	ip->i_lfs = ump->um_lfs;

	/* Read in the disk contents for the inode, copy into the inode. */
	if (dinp) {
		if ((error = copyin(dinp, ip->i_din.ffs1_din, sizeof(struct ufs1_dinode)))) {
			return (error);
		}
	} else {
		retries = 0;
	again:
		if ((error = bread(ump->um_devvp, daddr,
		    (int)ump->um_lfs->lfs_bsize, NOCRED, &bp))) {
			/*
			 * The inode does not contain anything useful, so it
			 * would be misleading to leave it on its hash chain.
			 * Iput() will return it to the free list.
			 */
			ufs_ihashrem(ip);

			/* Unlock and discard unneeded inode. */
			lfs_vunref(vp);
			brelse(bp);
			*vpp = NULL;
			return (error);
		}
		dip = lfs1_ifind(ump->um_lfs, ino, bp);
		if(dip == NULL) {
			/* Assume write has not completed yet; try again */
			bp->b_flags |= B_INVAL;
			brelse(bp);
			++retries;
			if (retries > LFS_IFIND_RETRIES)
				panic("lfs1_fastvget: dinode not found");
			printf("lfs1_fastvget: dinode not found, retrying...\n");
			goto again;
		}

		*ip->i_din.ffs1_din = *dip;
		brelse(bp);
	}

	/*
	 * Initialize the vnode from the inode, check for aliases.  In all
	 * cases re-init ip, the underlying vnode/inode may have changed.
	 */
	if ((error = ufs_vinit(mp, &lfs_specops, fifoops, &vp))) {
		lfs_vunref(vp);
		*vpp = NULL;
		return (error);
	}
	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */
	ip->i_devvp = ump->um_devvp;
	ip->i_flag |= IN_MODIFIED;
	++ump->um_lfs->lfs_uinodes;
	VREF(ip->i_devvp);
	*vpp = vp;
	return (0);
}

int
lfs2_fastvget(mp, ino, daddr, vpp, dinp)
	struct mount *mp;
	ino_t ino;
	ufs2_daddr_t daddr;
	struct vnode **vpp;
	struct ufs2_dinode *dinp;
{
	register struct inode *ip;
	struct ufs2_dinode *dip;
	struct vnode *vp;
	struct ufsmount *ump;
	struct buf *bp;
	struct vnodeops *fifoops;
	dev_t dev;
	int error, retries;
	
#ifdef FIFO
	fifoops = &lfs_fifoops
#else
	fifoops = NULL;
#endif

	ump = VFSTOUFS(mp);
	dev = ump->um_dev;

	/*
	 * This is playing fast and loose.  Someone may have the inode
	 * locked, in which case they are going to be distinctly unhappy
	 * if we trash something.
	 */
	if ((*vpp = ufs_ihashlookup(dev, ino)) != NULL) {
		lfs_vref(*vpp);
		if ((*vpp)->v_flag & VXLOCK)
			clean_vnlocked++;
		ip = VTOI(*vpp);
		if (lockstatus(&ip->i_lock))
			clean_inlocked++;
		if (!(ip->i_flag & IN_MODIFIED))
			++ump->um_lfs->lfs_uinodes;
		ip->i_flag |= IN_MODIFIED;
		return (0);
	}

	/* Allocate new vnode/inode. */
	if ((error = lfs_vcreate(mp, ino, &vp))) {
		*vpp = NULL;
		return (error);
	}

	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	ip = VTOI(vp);
	ufs_ihashins(ip);

	/*
	 * XXX
	 * This may not need to be here, logically it should go down with
	 * the i_devvp initialization.
	 * Ask Kirk.
	 */
	ip->i_lfs = ump->um_lfs;

	/* Read in the disk contents for the inode, copy into the inode. */
	if (dinp) {
		if ((error = copyin(dinp, ip->i_din.ffs2_din, sizeof(struct ufs2_dinode)))) {
			return (error);
		}
	} else {
		retries = 0;
	again:
		if ((error = bread(ump->um_devvp, daddr,
		    (int)ump->um_lfs->lfs_bsize, NOCRED, &bp))) {
			/*
			 * The inode does not contain anything useful, so it
			 * would be misleading to leave it on its hash chain.
			 * Iput() will return it to the free list.
			 */
			ufs_ihashrem(ip);

			/* Unlock and discard unneeded inode. */
			lfs_vunref(vp);
			brelse(bp);
			*vpp = NULL;
			return (error);
		}
		dip = lfs2_ifind(ump->um_lfs, ino, bp);
		if(dip == NULL) {
			/* Assume write has not completed yet; try again */
			bp->b_flags |= B_INVAL;
			brelse(bp);
			++retries;
			if (retries > LFS_IFIND_RETRIES)
				panic("lfs2_fastvget: dinode not found");
			printf("lfs2_fastvget: dinode not found, retrying...\n");
			goto again;
		}

		*ip->i_din.ffs2_din = *dip;
		brelse(bp);
	}

	/*
	 * Initialize the vnode from the inode, check for aliases.  In all
	 * cases re-init ip, the underlying vnode/inode may have changed.
	 */
	if ((error = ufs_vinit(mp, &lfs_specops, fifoops, &vp))) {
		lfs_vunref(vp);
		*vpp = NULL;
		return (error);
	}
	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */
	ip->i_devvp = ump->um_devvp;
	ip->i_flag |= IN_MODIFIED;
	++ump->um_lfs->lfs_uinodes;
	VREF(ip->i_devvp);
	*vpp = vp;
	return (0);
}
