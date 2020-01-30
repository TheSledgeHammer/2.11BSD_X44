#include <sys/param.h>
#include <sys/stdint.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/bio.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>

#include <ufs/ufs/extattr.h>
#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>


/* Missing: BLK_SNAP, BLK_NOCOPY, KERNCRED, BA_METAONLY, UFS_BALLOC(),
 * SF_SNAPSHOT, P_COWINPROGRESS */

static int expunge_lfs(struct vnode *, struct inode *, struct lfs *,
    int (*)(struct vnode *, ufs_daddr_t *, ufs_daddr_t *, struct lfs *,
    ufs_lbn_t, int), int);
static int indiracct_lfs(struct vnode *, struct vnode *, int,
    ufs_daddr_t, ufs_lbn_t, ufs_lbn_t, ufs_lbn_t, ufs_lbn_t, struct lfs *,
    int (*)(struct vnode *, ufs_daddr_t *, ufs_daddr_t *, struct lfs *,
    ufs_lbn_t, int), int);
static int fullacct_lfs(struct vnode *, ufs_daddr_t *, ufs_daddr_t *,
    struct lfs *, ufs_lbn_t, int);
static int snapacct_lfs(struct vnode *, ufs_daddr_t *, ufs_daddr_t *,
    struct lfs *, ufs_lbn_t, int);
static int mapacct_lfs(struct vnode *, ufs_daddr_t *, ufs_daddr_t *,
    struct lfs *, ufs_lbn_t, int);
static int lfs_copyonwrite(struct vnode *, struct buf *);
static int readblock(struct buf *, ufs_daddr_t);

static int
expunge_lfs(snapvp, cancelip, fs, acctfunc, expungetype)
	struct vnode *snapvp;
	struct inode *cancelip;
	struct lfs *fs;
	int (*acctfunc)(struct vnode *, ufs_daddr_t *, ufs_daddr_t *,
	    struct lfs *, ufs_lbn_t, int);
	int expungetype;
{
	int i, error, indiroff;
	ufs_lbn_t lbn, rlbn;
	ufs_daddr_t len, blkno, numblks, blksperindir;
	struct dinode *dip;
	struct thread *td = curthread;
	struct buf *bp;

	/*
	 * Prepare to expunge the inode. If its inode block has not
	 * yet been copied, then allocate and fill the copy.
	 */
	lbn = fragstoblks(fs, ino_to_fsba(fs, cancelip->i_number));
	blkno = 0;
	if (lbn < NDADDR) {
		blkno = VTOI(snapvp)->i_din->di_db[lbn];
	} else {
		//td->td_proc->p_flag |= P_COWINPROGRESS;
		error = UFS_BALLOC(snapvp, lblktosize(fs, (off_t)lbn),
		   fs->lfs_bsize, KERNCRED, BA_METAONLY, &bp);
		//td->td_proc->p_flag &= ~P_COWINPROGRESS;
		if (error)
			return (error);
		indiroff = (lbn - NDADDR) % NINDIR(fs);
		blkno = ((ufs_daddr_t *)(bp->b_data))[indiroff];
		bqrelse(bp);
	}
	if (blkno != 0) {
		if ((error = bread(snapvp, lbn, fs->lfs_bsize, KERNCRED, &bp)))
			return (error);
	} else {
		error = UFS_BALLOC(snapvp, lblktosize(fs, (off_t)lbn),
		    fs->lfs_bsize, KERNCRED, 0, &bp);
		if (error)
			return (error);
		if ((error = readblock(bp, lbn)) != 0)
			return (error);
	}
	/*
	 * Set a snapshot inode to be a zero length file, regular files
	 * to be completely unallocated.
	 */
	dip = (struct dinode *)bp->b_data +
	    ino_to_fsbo(fs, cancelip->i_number);
	if (expungetype == BLK_NOCOPY)
		dip->di_mode = 0;
	dip->di_size = 0;
	dip->di_blocks = 0;
	dip->di_flags &= ~SF_SNAPSHOT;
	bzero(&dip->di_db[0], (NDADDR + NIADDR) * sizeof(ufs_daddr_t));
	bdwrite(bp);
	/*
	 * Now go through and expunge all the blocks in the file
	 * using the function requested.
	 */
	numblks = howmany(cancelip->i_size, fs->lfs_bsize);
	if ((error = (*acctfunc)(snapvp, &cancelip->i_din->di_db[0],
	    &cancelip->i_din->di_db[NDADDR], fs, 0, expungetype)))
		return (error);
	if ((error = (*acctfunc)(snapvp, &cancelip->i_din->di_ib[0],
	    &cancelip->i_din->di_ib[NIADDR], fs, -1, expungetype)))
		return (error);
	blksperindir = 1;
	lbn = -NDADDR;
	len = numblks - NDADDR;
	rlbn = NDADDR;
	for (i = 0; len > 0 && i < NIADDR; i++) {
		error = indiracct_lfs(snapvp, ITOV(cancelip), i,
		    cancelip->i_din->di_ib[i], lbn, rlbn, len,
		    blksperindir, fs, acctfunc, expungetype);
		if (error)
			return (error);
		blksperindir *= NINDIR(fs);
		lbn -= blksperindir + 1;
		len -= blksperindir;
		rlbn += blksperindir;
	}

	return (0);
}

static int
indiracct_lfs(snapvp, cancelvp, level, blkno, lbn, rlbn, remblks,
	    blksperindir, fs, acctfunc, expungetype)
	struct vnode *snapvp;
	struct vnode *cancelvp;
	int level;
	ufs_daddr_t blkno;
	ufs_lbn_t lbn;
	ufs_lbn_t rlbn;
	ufs_lbn_t remblks;
	ufs_lbn_t blksperindir;
	struct lfs *fs;
	int (*acctfunc)(struct vnode *, ufs_daddr_t *, ufs_daddr_t *,
	    struct lfs *, ufs_lbn_t, int);
	int expungetype;
{
	int error, num, i;
	ufs_lbn_t subblksperindir;
	struct indir indirs[NIADDR + 2];
	ufs_daddr_t last, *bap;
	struct buf *bp;

	if ((error = ufs_getlbns(cancelvp, rlbn, indirs, &num)) != 0)
		return (error);
	if (lbn != indirs[num - 1 - level].in_lbn || blkno == 0 || num < 2)
		panic("indiracct: botched params");
	/*
	 * We have to expand bread here since it will deadlock looking
	 * up the block number for any blocks that are not in the cache.
	 */
	bp = getblk(cancelvp, lbn, fs->lfs_bsize, 0, 0);
	bp->b_blkno = fsbtodb(fs, blkno);
	if ((bp->b_flags & (B_DONE | B_DELWRI)) == 0 &&
	    (error = readblock(bp, fragstoblks(fs, blkno)))) {
		brelse(bp);
		return (error);
	}
	/*
	 * Account for the block pointers in this indirect block.
	 */
	last = howmany(remblks, blksperindir);
	if (last > NINDIR(fs))
		last = NINDIR(fs);
	MALLOC(bap, ufs_daddr_t *, fs->lfs_bsize, M_DEVBUF, M_WAITOK);
	bcopy(bp->b_data, (caddr_t)bap, fs->lfs_bsize);
	bqrelse(bp);
	error = (*acctfunc)(snapvp, &bap[0], &bap[last], fs,
	    level == 0 ? rlbn : -1, expungetype);
	if (error || level == 0)
		goto out;
	/*
	 * Account for the block pointers in each of the indirect blocks
	 * in the levels below us.
	 */
	subblksperindir = blksperindir / NINDIR(fs);
	for (lbn++, level--, i = 0; i < last; i++) {
		error = indiracct_ufs1(snapvp, cancelvp, level, bap[i], lbn,
		    rlbn, remblks, subblksperindir, fs, acctfunc, expungetype);
		if (error)
			goto out;
		rlbn += blksperindir;
		lbn -= blksperindir;
		remblks -= blksperindir;
	}
out:
	FREE(bap, M_DEVBUF);
	return (error);
}

/*
 * Do both snap accounting and map accounting.
 */
static int
fullacct_lfs(vp, oldblkp, lastblkp, fs, lblkno, exptype)
	struct vnode *vp;
	ufs_daddr_t *oldblkp, *lastblkp;
	struct lfs *fs;
	ufs_lbn_t lblkno;
	int exptype;	/* BLK_SNAP or BLK_NOCOPY */
{
	int error;

	if ((error = snapacct_lfs(vp, oldblkp, lastblkp, fs, lblkno, exptype)))
		return (error);
	return (mapacct_lfs(vp, oldblkp, lastblkp, fs, lblkno, exptype));
}

/*
 * Identify a set of blocks allocated in a snapshot inode.
 */
static int
snapacct_lfs(vp, oldblkp, lastblkp, fs, lblkno, expungetype)
	struct vnode *vp;
	ufs_daddr_t *oldblkp, *lastblkp;
	struct lfs *fs;
	ufs_lbn_t lblkno;
	int expungetype;	/* BLK_SNAP or BLK_NOCOPY */
{
	struct inode *ip = VTOI(vp);
	ufs_daddr_t blkno, *blkp;
	ufs_lbn_t lbn;
	struct buf *ibp;
	int error;

	for ( ; oldblkp < lastblkp; oldblkp++) {
		blkno = *oldblkp;
		if (blkno == 0 || blkno == BLK_NOCOPY || blkno == BLK_SNAP)
			continue;
		lbn = fragstoblks(fs, blkno);
		if (lbn < NDADDR) {
			blkp = &ip->i_din->di_db[lbn];
			ip->i_flag |= IN_CHANGE | IN_UPDATE;
		} else {
			error = UFS_BALLOC(vp, lblktosize(fs, (off_t)lbn),
			    fs->lfs_bsize, KERNCRED, BA_METAONLY, &ibp);
			if (error)
				return (error);
			blkp = &((ufs_daddr_t *)(ibp->b_data))
			    [(lbn - NDADDR) % NINDIR(fs)];
		}
		/*
		 * If we are expunging a snapshot vnode and we
		 * find a block marked BLK_NOCOPY, then it is
		 * one that has been allocated to this snapshot after
		 * we took our current snapshot and can be ignored.
		 */
		if (expungetype == BLK_SNAP && *blkp == BLK_NOCOPY) {
			if (lbn >= NDADDR)
				brelse(ibp);
		} else {
			if (*blkp != 0)
				panic("snapacct: bad block");
			*blkp = expungetype;
			if (lbn >= NDADDR)
				bdwrite(ibp);
		}
	}
	return (0);
}

/*
 * Account for a set of blocks allocated in a snapshot inode.
 */
static int
mapacct_lfs(vp, oldblkp, lastblkp, fs, lblkno, expungetype)
	struct vnode *vp;
	ufs_daddr_t *oldblkp, *lastblkp;
	struct lfs *fs;
	ufs_lbn_t lblkno;
	int expungetype;
{
	ufs_daddr_t blkno;
	struct inode *ip;
	ino_t inum;
	int acctit;

	ip = VTOI(vp);
	inum = ip->i_number;

	if (lblkno == -1)
		acctit = 0;
	else
		acctit = 1;
	for ( ; oldblkp < lastblkp; oldblkp++, lblkno++) {
		blkno = *oldblkp;
		if (blkno == 0 || blkno == BLK_NOCOPY)
			continue;
		if (acctit && expungetype == BLK_SNAP && blkno != BLK_SNAP)
			*ip->i_snapblklist++ = lblkno;
		if (blkno == BLK_SNAP)
				blkno = blkstofrags(fs, lblkno);
		lfs_vfree(ip);
	}
	return (0);
}

/*
 * Check for need to copy block that is about to be written,
 * copying the block if necessary.
 */
static int
lfs_copyonwrite(devvp, bp)
	struct vnode *devvp;
	struct buf *bp;
{
	struct snaphead *snaphead;
	struct buf *ibp, *cbp, *savedcbp = 0;
	struct thread *td = curthread;
	struct lfs *fs;
	struct inode *ip;
	struct vnode *vp = 0;
	ufs_daddr_t lbn, blkno, *snapblklist;
	int lower, upper, mid, indiroff, snapshot_locked = 0, error = 0;

	if (td->td_proc->p_flag & P_COWINPROGRESS)
		panic("ffs_copyonwrite: recursive call");
	/*
	 * First check to see if it is in the preallocated list.
	 * By doing this check we avoid several potential deadlocks.
	 */
	VI_LOCK(devvp);
	snaphead = &devvp->v_rdev->si_snapshots;
	ip = TAILQ_FIRST(snaphead);
	fs = ip->i_fs;
	lbn = fragstoblks(fs, dbtofsb(fs, bp->b_blkno));
	snapblklist = devvp->v_rdev->si_snapblklist;
	upper = devvp->v_rdev->si_snaplistsize - 1;
	lower = 1;
	while (lower <= upper) {
		mid = (lower + upper) / 2;
		if (snapblklist[mid] == lbn)
			break;
		if (snapblklist[mid] < lbn)
			lower = mid + 1;
		else
			upper = mid - 1;
	}
	if (lower <= upper) {
		VI_UNLOCK(devvp);
		return (0);
	}
	/*
	 * Not in the precomputed list, so check the snapshots.
	 */

retry:
		TAILQ_FOREACH(ip, snaphead, i_nextsnap) {
		vp = ITOV(ip);
		/*
		 * We ensure that everything of our own that needs to be
		 * copied will be done at the time that ffs_snapshot is
		 * called. Thus we can skip the check here which can
		 * deadlock in doing the lookup in UFS_BALLOC.
		 */
		if (bp->b_vp == vp)
			continue;
		/*
		 * Check to see if block needs to be copied. We do not have
		 * to hold the snapshot lock while doing this lookup as it
		 * will never require any additional allocations for the
		 * snapshot inode.
		 */
		if (lbn < NDADDR) {
			blkno = DIP(ip, i_db[lbn]);
		} else {
			if (snapshot_locked == 0 &&
			    lockmgr(vp->v_vnlock,
			      LK_INTERLOCK | LK_EXCLUSIVE | LK_SLEEPFAIL,
			      VI_MTX(devvp), td) != 0) {
				VI_LOCK(devvp);
				goto retry;
			}
			snapshot_locked = 1;
			td->td_proc->p_flag |= P_COWINPROGRESS;
			error = UFS_BALLOC(vp, lblktosize(fs, (off_t)lbn),
			   fs->fs_bsize, KERNCRED, BA_METAONLY, &ibp);
			td->td_proc->p_flag &= ~P_COWINPROGRESS;
			if (error)
				break;
			indiroff = (lbn - NDADDR) % NINDIR(fs);
			if (ip->i_ump->um_fstype == UFS1)
				blkno=((ufs_daddr_t *)(ibp->b_data))[indiroff];
			else
				blkno=((ufs2_daddr_t *)(ibp->b_data))[indiroff];
			bqrelse(ibp);
		}
#ifdef DIAGNOSTIC
		if (blkno == BLK_SNAP && bp->b_lblkno >= 0)
			panic("ffs_copyonwrite: bad copy block");
#endif
		if (blkno != 0)
			continue;
		/*
		 * Allocate the block into which to do the copy. Since
		 * multiple processes may all try to copy the same block,
		 * we have to recheck our need to do a copy if we sleep
		 * waiting for the lock.
		 *
		 * Because all snapshots on a filesystem share a single
		 * lock, we ensure that we will never be in competition
		 * with another process to allocate a block.
		 */
		if (snapshot_locked == 0 && lockmgr(vp->v_vnlock, LK_INTERLOCK | LK_EXCLUSIVE | LK_SLEEPFAIL, VI_MTX(devvp), td) != 0) {
			VI_LOCK(devvp);
			goto retry;
		}
		snapshot_locked = 1;
		td->td_proc->p_flag |= P_COWINPROGRESS;
		error = UFS_BALLOC(vp, lblktosize(fs, (off_t)lbn),
		    fs->fs_bsize, KERNCRED, 0, &cbp);
		td->td_proc->p_flag &= ~P_COWINPROGRESS;
		if (error)
			break;
#ifdef DEBUG
		if (snapdebug) {
			printf("Copyonwrite: snapino %d lbn %jd for ",
			    ip->i_number, (intmax_t)lbn);
			if (bp->b_vp == devvp)
				printf("fs metadata");
			else
				printf("inum %d", VTOI(bp->b_vp)->i_number);
			printf(" lblkno %jd to blkno %jd\n",
			    (intmax_t)bp->b_lblkno, (intmax_t)cbp->b_blkno);
		}
#endif
		/*
		 * If we have already read the old block contents, then
		 * simply copy them to the new block. Note that we need
		 * to synchronously write snapshots that have not been
		 * unlinked, and hence will be visible after a crash,
		 * to ensure their integrity.
		 */
		if (savedcbp != 0) {
			bcopy(savedcbp->b_data, cbp->b_data, fs->fs_bsize);
			bawrite(cbp);
			if (dopersistence && ip->i_effnlink > 0)
				(void) VOP_FSYNC(vp, KERNCRED, MNT_WAIT, td);
			continue;
		}
		/*
		 * Otherwise, read the old block contents into the buffer.
		 */
		if ((error = readblock(cbp, lbn)) != 0) {
			bzero(cbp->b_data, fs->fs_bsize);
			bawrite(cbp);
			if (dopersistence && ip->i_effnlink > 0)
				(void) VOP_FSYNC(vp, KERNCRED, MNT_WAIT, td);
			break;
		}
		savedcbp = cbp;
	}
	/*
	 * Note that we need to synchronously write snapshots that
	 * have not been unlinked, and hence will be visible after
	 * a crash, to ensure their integrity.
	 */
	if (savedcbp) {
		vp = savedcbp->b_vp;
		bawrite(savedcbp);
		if (dopersistence && VTOI(vp)->i_effnlink > 0)
			(void) VOP_FSYNC(vp, KERNCRED, MNT_WAIT, td);
	}
	if (snapshot_locked)
		VOP_UNLOCK(vp, 0, td);
	else
		VI_UNLOCK(devvp);
	return (error);
}

/*
 * Read the specified block into the given buffer.
 * Much of this boiler-plate comes from bwrite().
 */
static int
readblock(bp, lbn)
	struct buf *bp;
	ufs_daddr_t lbn;
{
	struct uio auio;
	struct iovec aiov;
	struct thread *td = curthread;
	struct inode *ip = VTOI(bp->b_vp);

	aiov.iov_base = bp->b_data;
	aiov.iov_len = bp->b_bcount;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = dbtob(fsbtodb(ip->i_fs, blkstofrags(ip->i_fs, lbn)));
	auio.uio_resid = bp->b_bcount;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_td = td;
	return (physio(ip->i_devvp->v_rdev, &auio, 0));
}
