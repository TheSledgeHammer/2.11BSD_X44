/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_inode.c	1.7 (2.11BSD GTE) 1997/2/7
 */

#include <sys/param.h>

#include <sys/user.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/syslog.h>

#include <ufs/ufs211/ufs211_extern.h>
#include <ufs/ufs211/ufs211_fs.h>
#include <ufs/ufs211/ufs211_inode.h>
//#include <ufs/ufs211/ufs211_quota.h>

#define	SINGLE				0	/* index of single indirect block */
#define	DOUBLE				1	/* index of double indirect block */
#define	TRIPLE				2	/* index of triple indirect block */

static void	ufs211_trsingle(struct ufs211_inode *, caddr_t, daddr_t, int);

/*
 * Check accessed and update flags on
 * an inode structure.
 * If any are on, update the inode
 * with the current time.
 * If waitfor set, then must insure
 * i/o order so wait for the write to complete.
 */
void
ufs211_updat(ip, ta, tm, waitfor)
	struct ufs211_inode *ip;
	struct timeval 		*ta, *tm;
	int 				waitfor;
{
	register struct buf *bp;
	register struct ufs211_dinode *dp;
#ifdef EXTERNALITIMES
	struct ufs211_icommon2 xic2, *xicp2;
#endif
	register struct ufs211_inode *tip = ip;

	if ((tip->i_flag & (UFS211_IUPD | UFS211_IACC | UFS211_ICHG | UFS211_IMOD))
			== 0)
		return;
	if (tip->i_fs->fs_ronly)
		return;
	if (bread(tip->i_devvp, itod(tip->i_number), tip->i_fs->fs_fsize, u->u_cred,
			&bp)) {
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return;
		}
	}
#ifdef EXTERNALITIMES
	xicp2 = &((struct icommon2 *)SEG5)[ip - inode];
	if (tip->i_flag & IACC)
		xicp2->ic_atime = ta->tv_sec;
	if (tip->i_flag & IUPD)
		xicp2->ic_mtime = tm->tv_sec;
	if (tip->i_flag & ICHG)
		xicp2->ic_ctime = time.tv_sec;
	xic2 = *xicp2;
#else
	if (tip->i_flag & UFS211_IACC)
		tip->i_atime = ta->tv_sec;
	if (tip->i_flag & UFS211_IUPD)
		tip->i_mtime = tm->tv_sec;
	if (tip->i_flag & UFS211_ICHG)
		tip->i_ctime = time.tv_sec;
#endif
	tip->i_flag &= ~(UFS211_IUPD | UFS211_IACC | UFS211_ICHG | UFS211_IMOD);
	ufs211_mapin(bp);
	dp = (struct ufs211_dinode*) bp + itoo(tip->i_number);
	dp->di_icom1 = tip->i_ic1;
	dp->di_flag = tip->i_flags;
#ifdef EXTERNALITIMES
	dp->di_ic2 = xic2;
#else
	dp->di_icom2 = tip->i_ic2;
#endif
	bcopy(ip->i_addr, dp->di_addr, NADDR * sizeof(daddr_t));
	ufs211_mapout(bp);
	if (waitfor && ((ip->i_fs->fs_flags & MNT_ASYNC) == 0)) {
		bwrite(bp);
	} else {
		bdwrite(bp);
	}
}

/*
 * Truncate the inode ip to at most
 * length size.  Free affected disk
 * blocks -- the blocks of the file
 * are removed in reverse order.
 *
 * NB: triple indirect blocks are untested.
 */
void
ufs211_trunc(oip,length, ioflags)
	register struct ufs211_inode *oip;
	u_long length;
	int	ioflags;
{
	daddr_t lastblock;
	register int i;
	register struct ufs211_inode *ip;
	daddr_t bn, lastiblock[NIADDR];
	struct buf *bp;
	int offset, level;
	struct ufs211_inode tip;
	int aflags;
#ifdef QUOTA
	long bytesreleased;
#endif

	aflags = B_CLRBUF;
	if (ioflags & IO_SYNC)
		aflags |= B_SYNC;

	/*
	 * special hack for pipes, since size for them isn't the size of
	 * the file, it's the amount currently waiting for transfer.  It's
	 * unclear that this will work, though, because pipes can (although
	 * rarely do) get bigger than MAXPIPSIZ.  Don't think it worked
	 * in V7 either, I don't really understand what's going on.
	 */
	if (oip->i_flag & UFS211_IPIPE)
		oip->i_size = MAXPIPSIZ;
	else if (oip->i_size == length)
		goto updret;

	/*
	 * Lengthen the size of the file. We must ensure that the
	 * last byte of the file is allocated. Since the smallest
	 * value of osize is 0, length will be at least 1.
	 */
	if (oip->i_size < length) {
		bn = bmap(oip, lblkno(length - 1), B_WRITE, aflags);
		if (u->u_error || bn < 0)
			return;
#ifdef	QUOTA
		bytesreleased = oip->i_size - length;
#endif
		oip->i_size = length;
		goto doquotaupd;
	}

	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	lastblock = lblkno(length + DEV_BSIZE - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR;
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR * NINDIR;
	/*
	 * Update the size of the file. If the file is not being
	 * truncated to a block boundry, the contents of the
	 * partial block following the end of the file must be
	 * zero'ed in case it ever become accessable again because
	 * of subsequent file growth.
	 */
	offset = blkoff(length);
	if (offset) {
		bn = bmap(oip, lblkno(length), B_WRITE, aflags);
		if (u->u_error || bn < 0)
			return;
		if(bread(oip->i_devvp, bn, oip->i_fs->fs_fsize, u->u_ucred, &bp)){
			if (bp->b_flags & B_ERROR) {
				u->u_error = EIO;
				brelse(bp);
				return;
			}
		}
		ufs211_mapin(bp);
		bzero(bp + offset, (u_int)(DEV_BSIZE - offset));
		ufs211_mapout(bp);
		bdwrite(bp);
	}
	/*
	 * Update file and block pointers
	 * on disk before we start freeing blocks.
	 * If we crash before free'ing blocks below,
	 * the blocks will be returned to the free list.
	 * lastiblock values are also normalized to -1
	 * for calls to indirtrunc below.
	 */
	tip = *oip;
#ifdef QUOTA
	bytesreleased = oip->i_size - length;
#endif
	oip->i_size = length;
	for (level = TRIPLE; level >= SINGLE; level--)
		if (lastiblock[level] < 0) {
			oip->i_ib[level] = 0;
			lastiblock[level] = -1;
		}
	for (i = NDADDR - 1; i > lastblock; i--)
		oip->i_db[i] = 0;

	/*
	 * Indirect blocks first.
	 */
	ip = &tip;
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = ip->i_ib[level];
		if (bn != 0) {
			ufs211_indirtrunc(ip, bn, lastiblock[level], level, aflags);
			if (lastiblock[level] < 0) {
				ip->i_ib[level] = 0;
				free(ip, bn);
			}
		}
		if (lastiblock[level] >= 0)
			goto done;
	}

	/*
	 * All whole direct blocks.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		bn = ip->i_db[i];
		if (bn == 0)
			continue;
		ip->i_db[i] = 0;
		free(ip, bn);
	}
	if (lastblock < 0)
		goto done;

done:
#ifdef DIAGNOSTIC
/* BEGIN PARANOIA */
	for (level = SINGLE; level <= TRIPLE; level++)
		if (ip->i_ib[level] != oip->i_ib[level])
			panic("itrunc1");
	for (i = 0; i < NDADDR; i++)
		if (ip->i_db[i] != oip->i_db[i])
			panic("itrunc2");
/* END PARANOIA */
#endif

doquotaupd:
#ifdef QUOTA
	QUOTAMAP();
	(void)chkdq(oip, -bytesreleased, 0);
	QUOTAUNMAP();
#endif
updret:
	oip->i_flag |= UFS211_ICHG|UFS211_IUPD;
	ufs211_updat(oip, &time, &time, 1);
}

/*
 * Release blocks associated with the inode ip and
 * stored in the indirect block bn.  Blocks are free'd
 * in LIFO order up to (but not including) lastbn.  If
 * level is greater than SINGLE, the block is an indirect
 * block and recursive calls to indirtrunc must be used to
 * cleanse other indirect blocks.
 *
 * NB: triple indirect blocks are untested.
 */
void
ufs211_indirtrunc(ip, bn, lastbn, level, aflags)
	struct ufs211_inode *ip;
	daddr_t bn, lastbn;
	int level;
	int aflags;
{
	register struct buf *bp;
	daddr_t nb, last;
	long factor;

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	switch(level) {
		case SINGLE:
			factor = 1;
			break;
		case DOUBLE:
			factor = NINDIR;
			break;
		case TRIPLE:
			factor = NINDIR * NINDIR;
			break;
	}
	last = lastbn;
	if (lastbn > 0)
		last = last / factor;
	/*
	 * Get buffer of block pointers, zero those
	 * entries corresponding to blocks to be free'd,
	 * and update on disk copy first.
	 */
	{
		register daddr_t *bap;
		register struct buf *cpy;

		if (bread(ip->i_devvp, bn, ip->i_fs->fs_fsize, u->u_ucred, &bp)) {
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				return;
			}
		}
		cpy = geteblk();
		copy(bftopaddr(bp), bftopaddr(cpy), btoc(DEV_BSIZE));
		ufs211_mapin(bp);
		bap = (daddr_t *) bp;
		bzero((caddr_t)&bap[last + 1], (u_int)(NINDIR - (last + 1)) * sizeof(daddr_t));
		ufs211_mapout(bp);
		if (aflags & B_SYNC)
			bwrite(bp);
		else
			bawrite(bp);
		bp = cpy;
	}

	/*
	 * Optimized for single indirect blocks, i.e. until a file is
	 * greater than 4K + 256K you don't have to do a mapin/mapout
	 * for every entry.  The mapin/mapout is required since free()
	 * may have to map an item in.  Have to use another routine
	 * since it requires 1K of kernel stack to get around the problem
	 * and that doesn't work well with recursion.
	 */
	if (level == SINGLE)
		ufs211_trsingle(ip, bp, last, aflags);
	else {
		register daddr_t *bstart, *bstop;

		ufs211_mapin(bp);
		bstart = (daddr_t *) bp;
		bstop = &bstart[last];
		bstart += NINDIR - 1;
		/*
		 * Recursively free totally unused blocks.
		 */
		for (;bstart > bstop;--bstart) {
			nb = *bstart;
			if (nb) {
				ufs211_mapout(bp);
				ufs211_indirtrunc(ip, nb, (daddr_t) - 1, level - 1, aflags);
				ufs211_bfree(ip, nb);
				ufs211_mapin(bp);
			}
		}
		ufs211_mapout(bp);

		/*
		 * Recursively free last partial block.
		 */
		if (lastbn >= 0) {

			ufs211_mapin(bp);
			nb = *bstop;
			ufs211_mapout(bp);
			last = lastbn % factor;
			if (nb != 0)
				ufs211_indirtrunc(ip, nb, last, level - 1, aflags);
		}
	}
	brelse(bp);
}

static void
ufs211_trsingle(ip, bp,last, aflags)
	register struct ufs211_inode *ip;
	caddr_t bp;
	daddr_t last;
	int aflags;
{
	register daddr_t *bstart, *bstop;
	daddr_t blarray[NINDIR];

	ufs211_mapin(bp);
	bcopy(bp, blarray, NINDIR * sizeof(daddr_t));
	ufs211_mapout(bp);
	bstart = &blarray[NINDIR - 1];
	bstop = &blarray[last];
	for (; bstart > bstop; --bstart) {
		if (*bstart) {
			ufs211_bfree(ip, *bstart);
		}
	}
}
