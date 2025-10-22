/*
 * Copyright (c) 1991, 1993
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
 *	@(#)lfs_segment.c	8.10 (Berkeley) 6/10/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/kernel.h>
#include <sys/resourcevar.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/user.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>
#include <miscfs/fifofs/fifo.h>
#include <miscfs/specfs/specdev.h>

extern int count_lock_queue (void);

#define MAX_ACTIVE	10
/*
 * Determine if it's OK to start a partial in this segment, or if we need
 * to go on to a new segment.
 */
#define	LFS_PARTIAL_FITS(fs) 									\
	((fs)->lfs_dbpseg - ((fs)->lfs_offset - (fs)->lfs_curseg) > \
	1 << (fs)->lfs_fsbtodb)

void	lfs_callback(struct buf *);
void	lfs_gather(struct lfs *, struct segment *, struct vnode *, int (*) (struct lfs *, struct buf *));
//void	lfs_iset(struct inode *, ufs1_daddr_t, time_t);
int	 	lfs_match_data(struct lfs *, struct buf *);
int	 	lfs_match_dindir(struct lfs *, struct buf *);
int	 	lfs_match_indir(struct lfs *, struct buf *);
int	 	lfs_match_tindir(struct lfs *, struct buf *);
void	lfs_newseg(struct lfs *);
void	lfs_shellsort(struct buf **, ufs1_daddr_t *, register int);
void	lfs_supercallback(struct buf *);
int	 	lfs_vref(struct vnode *);
void	lfs_vunref(struct vnode *);
void	lfs_writefile(struct lfs *, struct segment *, struct vnode *);
void	lfs_writevnodes(struct lfs *, struct mount *, struct segment *, int);

//int	lfs_allclean_wakeup;		/* Cleaner wakeup address. */

/* Statistics Counters */
#define DOSTATS
struct lfs_stats lfs_stats;

/* op values to lfs_writevnodes */
#define	VN_REG	0
#define	VN_DIROP	1
#define	VN_EMPTY	2

/*
 * Ifile and meta data blocks are not marked busy, so segment writes MUST be
 * single threaded.  Currently, there are two paths into lfs_segwrite, sync()
 * and getnewbuf().  They both mark the file system busy.  Lfs_vflush()
 * explicitly marks the file system busy.  So lfs_segwrite is safe.  I think.
 */

int
lfs_vflush(vp)
	struct vnode *vp;
{
	struct inode *ip;
	struct lfs *fs;
	struct segment *sp;

	fs = VFSTOUFS(vp->v_mount)->um_lfs;
	if (fs->lfs_nactive > MAX_ACTIVE)
		return(lfs_segwrite(vp->v_mount, SEGM_SYNC|SEGM_CKP));
	lfs_seglock(fs, SEGM_SYNC);
	sp = fs->lfs_sp;

	ip = VTOI(vp);
	if (LIST_FIRST(&vp->v_dirtyblkhd) == NULL)
		lfs_writevnodes(fs, vp->v_mount, sp, VN_EMPTY);

	do {
		do {
			if (LIST_FIRST(&vp->v_dirtyblkhd) != NULL)
				lfs_writefile(fs, sp, vp);
		} while (lfs_writeinode(fs, sp, ip));

	} while (lfs_writeseg(fs, sp) && ip->i_number == LFS_IFILE_INUM);

#ifdef DOSTATS
	++lfs_stats.nwrites;
	if (sp->seg_flags & SEGM_SYNC)
		++lfs_stats.nsync_writes;
	if (sp->seg_flags & SEGM_CKP)
		++lfs_stats.ncheckpoints;
#endif
	lfs_segunlock(fs);
	return (0);
}

void
lfs_writevnodes(fs, mp, sp, op)
	struct lfs *fs;
	struct mount *mp;
	struct segment *sp;
	int op;
{
	struct inode *ip;
	struct vnode *vp;

/* BEGIN HACK */
#define	VN_OFFSET		(((void *)LIST_NEXT(vp, v_mntvnodes)) - (void *)vp)
#define	BACK_VP(VP)		((struct vnode *)(((void *)LIST_PREV(VP, v_mntvnodes)) - VN_OFFSET))
#define	BEG_OF_VLIST	((struct vnode *)(((void *)LIST_FIRST(&mp->mnt_vnodelist)) - VN_OFFSET))

/* Find last vnode. */
loop:   
	for (vp = LIST_FIRST(&mp->mnt_vnodelist);
	     vp && LIST_NEXT(vp, v_mntvnodes) != NULL;
	     vp = LIST_NEXT(vp, v_mntvnodes));
	for (; vp && vp != BEG_OF_VLIST; vp = BACK_VP(vp)) {
/* END HACK */
/*
loop:
	for (vp = mp->mnt_vnodelist.lh_first;
	     vp != NULL;
	     vp = vp->v_mntvnodes.le_next) {
*/
		/*
		 * If the vnode that we are about to sync is no longer
		 * associated with this mount point, start over.
		 */
		if (vp->v_mount != mp)
			goto loop;

		/* XXX ignore dirops for now
		if (op == VN_DIROP && !(vp->v_flag & VDIROP) ||
		    op != VN_DIROP && (vp->v_flag & VDIROP))
			continue;
		*/

		if (op == VN_EMPTY && LIST_FIRST(&vp->v_dirtyblkhd))
			continue;

		if (vp->v_type == VNON)
			continue;

		if (lfs_vref(vp))
			continue;

		/*
		 * Write the inode/file if dirty and it's not the
		 * the IFILE.
		 */
		ip = VTOI(vp);
		if (((ip->i_flag &
		    (IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE)) ||
		    LIST_FIRST(&vp->v_dirtyblkhd) != NULL) &&
		    ip->i_number != LFS_IFILE_INUM) {
			if (LIST_FIRST(&vp->v_dirtyblkhd) != NULL)
				lfs_writefile(fs, sp, vp);
			(void) lfs_writeinode(fs, sp, ip);
		}
		vp->v_flag &= ~VDIROP;
		lfs_vunref(vp);
	}
}

int
lfs_segwrite(mp, flags)
	struct mount *mp;
	int flags;			/* Do a checkpoint. */
{
	struct proc *p = curproc;	/* XXX */
	struct buf *bp;
	struct inode *ip;
	struct lfs *fs;
	struct segment *sp;
	struct vnode *vp;
	SEGUSE *segusep;
	ufs1_daddr_t ibno;
	CLEANERINFO *cip;
	int clean, do_ckp, error, i;

	fs = VFSTOUFS(mp)->um_lfs;

 	/*
 	 * If we have fewer than 2 clean segments, wait until cleaner
	 * writes.
 	 */
	do {
		LFS_CLEANERINFO(cip, fs, bp);
		clean = cip->clean;
		brelse(bp);
		if (clean <= 2 || fs->lfs_avail <= 0) {
			/* printf ("segs clean: %d\n", clean); */
			wakeup(&lfs_allclean_wakeup);
			wakeup(&fs->lfs_nextseg);
			if ((error = tsleep(&fs->lfs_avail, PRIBIO + 1,
			    "lfs writer", 0)))
				return (error);
		}
	} while (clean <= 2 || fs->lfs_avail <= 0);

	/*
	 * Allocate a segment structure and enough space to hold pointers to
	 * the maximum possible number of buffers which can be described in a
	 * single summary block.
	 */
	do_ckp = (flags & SEGM_CKP) || fs->lfs_nactive > MAX_ACTIVE;
	lfs_seglock(fs, flags | (do_ckp ? SEGM_CKP : 0));
	sp = fs->lfs_sp;

	lfs_writevnodes(fs, mp, sp, VN_REG);

	/* XXX ignore ordering of dirops for now */
	/* XXX
	fs->lfs_writer = 1;
	if (fs->lfs_dirops && (error =
	    tsleep(&fs->lfs_writer, PRIBIO + 1, "lfs writer", 0))) {
		free(sp->bpp, M_SEGMENT);
		free(sp, M_SEGMENT); 
		fs->lfs_writer = 0;
		return (error);
	}

	lfs_writevnodes(fs, mp, sp, VN_DIROP);
	*/

	/*
	 * If we are doing a checkpoint, mark everything since the
	 * last checkpoint as no longer ACTIVE.
	 */
	if (do_ckp)
		for (ibno = fs->lfs_cleansz + fs->lfs_segtabsz;
		     --ibno >= fs->lfs_cleansz; ) {
			if (bread(fs->lfs_ivnode, ibno, fs->lfs_bsize,
			    NOCRED, &bp))

				panic("lfs: ifile read");
			segusep = (SEGUSE *)bp->b_data;
			for (i = fs->lfs_sepb; i--; segusep++)
				segusep->su_flags &= ~SEGUSE_ACTIVE;
				
			error = VOP_BWRITE(bp);
		}

	if (do_ckp || fs->lfs_doifile) {
redo:
		vp = fs->lfs_ivnode;
		while (vget(vp, LK_EXCLUSIVE, p))
			continue;
		ip = VTOI(vp);
		if (LIST_FIRST(&vp->v_dirtyblkhd) != NULL)
			lfs_writefile(fs, sp, vp);
		(void)lfs_writeinode(fs, sp, ip);
		vput(vp);
		if (lfs_writeseg(fs, sp) && do_ckp)
			goto redo;
	} else
		(void) lfs_writeseg(fs, sp);

	/*
	 * If the I/O count is non-zero, sleep until it reaches zero.  At the
	 * moment, the user's process hangs around so we can sleep.
	 */
	/* XXX ignore dirops for now
	fs->lfs_writer = 0;
	fs->lfs_doifile = 0;
	wakeup(&fs->lfs_dirops);
	*/

#ifdef DOSTATS
	++lfs_stats.nwrites;
	if (sp->seg_flags & SEGM_SYNC)
		++lfs_stats.nsync_writes;
	if (sp->seg_flags & SEGM_CKP)
		++lfs_stats.ncheckpoints;
#endif
	lfs_segunlock(fs);
	return (0);
}

/*
 * Write the dirty blocks associated with a vnode.
 */
void
lfs_writefile(fs, sp, vp)
	struct lfs *fs;
	struct segment *sp;
	struct vnode *vp;
{
	struct inode *ip;
	struct buf *bp;
	struct finfo *fip;
	IFILE *ifp;

	if (sp->seg_bytes_left < fs->lfs_bsize ||
	    sp->sum_bytes_left < sizeof(struct finfo))
		(void) lfs_writeseg(fs, sp);

	sp->sum_bytes_left -= sizeof(struct finfo) - sizeof(ufs1_daddr_t);
	++((SEGSUM *)(sp->segsum))->ss_nfinfo;

	ip = VTOI(vp);
	fip = sp->fip;
	fip->fi_nblocks = 0;
	fip->fi_ino = ip->i_number;
	LFS_IENTRY(ifp, fs, fip->fi_ino, bp);
	fip->fi_version = ifp->if_version;
	brelse(bp);

	/*
	 * It may not be necessary to write the meta-data blocks at this point,
	 * as the roll-forward recovery code should be able to reconstruct the
	 * list.
	 */
	lfs_gather(fs, sp, vp, lfs_match_data);
	lfs_gather(fs, sp, vp, lfs_match_indir);
	lfs_gather(fs, sp, vp, lfs_match_dindir);
#ifdef TRIPLE
	lfs_gather(fs, sp, vp, lfs_match_tindir);
#endif

	fip = sp->fip;
	if (fip->fi_nblocks != 0) {
		sp->fip =
		    (struct finfo *)((caddr_t)fip + sizeof(struct finfo) +
		    sizeof(ufs1_daddr_t) * (fip->fi_nblocks - 1));
		sp->start_lbp = &sp->fip->fi_blocks[0];
	} else {
		sp->sum_bytes_left += sizeof(struct finfo) - sizeof(ufs1_daddr_t);
		--((SEGSUM *)(sp->segsum))->ss_nfinfo;
	}
}

int
lfs_writeinode(fs, sp, ip)
	struct lfs *fs;
	struct segment *sp;
	struct inode *ip;
{
	struct buf *bp, *ibp;
	struct ufs1_dinode *cdp1;
	struct ufs2_dinode *cdp2;
	IFILE *ifp;
	SEGUSE *sup;
	ufs1_daddr_t daddr;
	ino_t ino;
	int error, i, ndx;
	int redo_ifile = 0;

	if (!(ip->i_flag & (IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE)))
		return(0);

	/* Allocate a new inode block if necessary. */
	if (sp->ibp == NULL) {
		/* Allocate a new segment if necessary. */
		if (sp->seg_bytes_left < fs->lfs_bsize ||
		    sp->sum_bytes_left < sizeof(ufs1_daddr_t))
			(void) lfs_writeseg(fs, sp);

		/* Get next inode block. */
		daddr = fs->lfs_offset;
		fs->lfs_offset += fsbtodb(fs, 1);
		sp->ibp = *sp->cbpp++ =
		    lfs_newbuf(VTOI(fs->lfs_ivnode)->i_devvp, daddr,
		    fs->lfs_bsize);
		/* Zero out inode numbers */
		for (i = 0; i < INOPB(fs); ++i) {
			if (I_IS_UFS1(ip)) {
				((struct ufs1_dinode *)sp->ibp->b_data)[i].di_inumber = 0;
			} else {
				((struct ufs2_dinode *)sp->ibp->b_data)[i].di_inumber = 0;
			}
		}

		++sp->start_bpp;
		fs->lfs_avail -= fsbtodb(fs, 1);
		/* Set remaining space counters. */
		sp->seg_bytes_left -= fs->lfs_bsize;
		sp->sum_bytes_left -= sizeof(ufs1_daddr_t);
		ndx = LFS_SUMMARY_SIZE / sizeof(ufs1_daddr_t) -
		    sp->ninodes / INOPB(fs) - 1;
		((ufs1_daddr_t *)(sp->segsum))[ndx] = daddr;
	}

	/* Update the inode times and copy the inode onto the inode page. */
	if (ip->i_flag & IN_MODIFIED)
		--fs->lfs_uinodes;
	ITIMES(ip, &time, &time);
	ip->i_flag &= ~(IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE);
	bp = sp->ibp;
	if (I_IS_UFS1(ip)) {
		cdp1 = ((struct ufs1_dinode *)bp->b_data) + (sp->ninodes % INOPB(fs));
		*cdp1 = *ip->i_din.ffs1_din;
	} else {
		cdp2 = ((struct ufs2_dinode *)bp->b_data) + (sp->ninodes % INOPB(fs));
		*cdp2 = *ip->i_din.ffs2_din;
	}
	
	/* Increment inode count in segment summary block. */
	++((SEGSUM *)(sp->segsum))->ss_ninos;

	/* If this page is full, set flag to allocate a new page. */
	if (++sp->ninodes % INOPB(fs) == 0)
		sp->ibp = NULL;

	/*
	 * If updating the ifile, update the super-block.  Update the disk
	 * address and access times for this inode in the ifile.
	 */
	ino = ip->i_number;
	if (ino == LFS_IFILE_INUM) {
		daddr = fs->lfs_idaddr;
		fs->lfs_idaddr = bp->b_blkno;
	} else {
		LFS_IENTRY(ifp, fs, ino, ibp);
		daddr = ifp->if_daddr;
		ifp->if_daddr = bp->b_blkno;
		error = VOP_BWRITE(ibp);
	}

	/*
	 * No need to update segment usage if there was no former inode address
	 * or if the last inode address is in the current partial segment.
	 */
	if (daddr != LFS_UNUSED_DADDR && 
	    !(daddr >= fs->lfs_lastpseg && daddr <= bp->b_blkno)) {
		LFS_SEGENTRY(sup, fs, datosn(fs, daddr), bp);
#ifdef DIAGNOSTIC
		if (I_IS_UFS1(ip)) {
			if (sup->su_nbytes < sizeof(struct ufs1_dinode)) {
				/* XXX -- Change to a panic. */
				printf("lfs: negative bytes (segment %d)\n",
				    datosn(fs, daddr));
				panic("negative bytes");
			}
		} else {
			if (sup->su_nbytes < sizeof(struct ufs2_dinode)) {
				/* XXX -- Change to a panic. */
				printf("lfs: negative bytes (segment %d)\n",
				    datosn(fs, daddr));
				panic("negative bytes");
			}
		}

#endif
		if (I_IS_UFS1(ip)) {
			sup->su_nbytes -= sizeof(struct ufs1_dinode);
		} else {
			sup->su_nbytes -= sizeof(struct ufs2_dinode);
		}

		redo_ifile =
		    (ino == LFS_IFILE_INUM && !(bp->b_flags & B_GATHERED));
		error = VOP_BWRITE(bp);
	}
	return (redo_ifile);
}

int
lfs_gatherblock(sp, bp, sptr)
	struct segment *sp;
	struct buf *bp;
	int *sptr;
{
	struct lfs *fs;
	struct inode *ip;
	int version;

	/*
	 * If full, finish this segment.  We may be doing I/O, so
	 * release and reacquire the splbio().
	 */
#ifdef DIAGNOSTIC
	if (sp->vp == NULL)
		panic ("lfs_gatherblock: Null vp in segment");
#endif
	fs = sp->fs;
	if (sp->sum_bytes_left < sizeof(ufs1_daddr_t) ||
	    sp->seg_bytes_left < bp->b_bcount) {
		if (sptr)
			splx(*sptr);
		lfs_updatemeta(sp);

		version = sp->fip->fi_version;
		(void) lfs_writeseg(fs, sp);

		ip = VTOI(sp->vp);
		sp->fip->fi_version = version;
		sp->fip->fi_ino = ip->i_number;
		/* Add the current file to the segment summary. */
		++((SEGSUM *)(sp->segsum))->ss_nfinfo;
		sp->sum_bytes_left -= 
		    sizeof(struct finfo) - sizeof(ufs1_daddr_t);

		if (sptr)
			*sptr = splbio();
		return (1);
	}

	/* Insert into the buffer list, update the FINFO block. */
	bp->b_flags |= B_GATHERED;
	*sp->cbpp++ = bp;
	sp->fip->fi_blocks[sp->fip->fi_nblocks++] = bp->b_lblkno;

	sp->sum_bytes_left -= sizeof(ufs1_daddr_t);
	sp->seg_bytes_left -= bp->b_bcount;
	return (0);
}

void
lfs_gather(fs, sp, vp, match)
	struct lfs *fs;
	struct segment *sp;
	struct vnode *vp;
	int (*match)(struct lfs *, struct buf *);
{
	struct buf *bp;
	int s;

	sp->vp = vp;
	s = splbio();
/* This is a hack to see if ordering the blocks in LFS makes a difference. */
/* BEGIN HACK */
#define	BUF_OFFSET		(((void *)LIST_NEXT(bp, b_vnbufs)) - (void *)bp)
#define	BACK_BUF(BP)	((struct buf *)(((void *)LIST_PREV(BP, b_vnbufs)) - BUF_OFFSET))
#define	BEG_OF_LIST		((struct buf *)(((void *)LIST_FIRST(&vp->v_dirtyblkhd)) - BUF_OFFSET))


/*loop:	for (bp = vp->v_dirtyblkhd.lh_first; bp; bp = bp->b_vnbufs.le_next) {*/
/* Find last buffer. */
loop:
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd);
			bp && LIST_NEXT(bp, b_vnbufs) != NULL; bp = LIST_NEXT(bp, b_vnbufs))
		;
	for (; bp && bp != BEG_OF_LIST; bp = BACK_BUF(bp)) {
		/* END HACK */
		if ((bp->b_flags & B_BUSY) || !match(fs, bp)
				|| (bp->b_flags & B_GATHERED))
			continue;
#ifdef DIAGNOSTIC
		if (!(bp->b_flags & B_DELWRI))
			panic("lfs_gather: bp not B_DELWRI");
		if (!(bp->b_flags & B_LOCKED))
			panic("lfs_gather: bp not B_LOCKED");
#endif
		if (lfs_gatherblock(sp, bp, &s))
			goto loop;
	}
	splx(s);
	lfs_updatemeta(sp);
	sp->vp = NULL;
}


/*
 * Update the metadata that points to the blocks listed in the FINFO
 * array.
 */
void
lfs_updatemeta(sp)
	struct segment *sp;
{
	SEGUSE *sup;
	struct buf *bp;
	struct lfs *fs;
	struct vnode *vp;
	struct indir a[NIADDR + 2], *ap;
	struct inode *ip;
	daddr_t daddr, lbn, off;
	int error, i, nblocks, num;

	vp = sp->vp;
	nblocks = &sp->fip->fi_blocks[sp->fip->fi_nblocks] - sp->start_lbp;
	if (nblocks < 0)
		panic("This is a bad thing\n");
	if (vp == NULL || nblocks == 0) 
		return;

	/* Sort the blocks. */
	if (!(sp->seg_flags & SEGM_CLEAN))
		lfs_shellsort(sp->start_bpp, sp->start_lbp, nblocks);

	/*
	 * Record the length of the last block in case it's a fragment.
	 * If there are indirect blocks present, they sort last.  An
	 * indirect block will be lfs_bsize and its presence indicates
	 * that you cannot have fragments.
	 */
	sp->fip->fi_lastlength = sp->start_bpp[nblocks - 1]->b_bcount;

	/*
	 * Assign disk addresses, and update references to the logical
	 * block and the segment usage information.
	 */
	fs = sp->fs;
	for (i = nblocks; i--; ++sp->start_bpp) {
		lbn = *sp->start_lbp++;
		(*sp->start_bpp)->b_blkno = off = fs->lfs_offset;
		fs->lfs_offset +=
		    fragstodb(fs, numfrags(fs, (*sp->start_bpp)->b_bcount));

		if ((error = ufs_bmaparray(vp, lbn, &daddr, a, &num, NULL)))
			panic("lfs_updatemeta: ufs_bmaparray %d", error);
		ip = VTOI(vp);
		switch (num) {
		case 0:
			DIP_SET(ip, db[lbn], off);
			//ip->i_ffs1_db[lbn] = off;
			break;
		case 1:
			DIP_SET(ip, ib[a[0].in_off], off);
			//ip->i_ffs1_ib[a[0].in_off] = off;
			break;
		default:
			ap = &a[num - 1];
			if (bread(vp, ap->in_lbn, fs->lfs_bsize, NOCRED, &bp))
				panic("lfs_updatemeta: bread bno %d",
				    ap->in_lbn);
			/*
			 * Bread may create a new indirect block which needs
			 * to get counted for the inode.
			 */
			if (bp->b_blkno == -1 && !(bp->b_flags & B_CACHE)) {
				DIP_SET(ip, blocks, +fsbtodb(fs, 1));
				//ip->i_ffs1_blocks += fsbtodb(fs, 1);
				fs->lfs_bfree -= fragstodb(fs, fs->lfs_frag);
			}
			((ufs1_daddr_t *)bp->b_data)[ap->in_off] = off;
			VOP_BWRITE(bp);
		}

		/* Update segment usage information. */
		if (daddr != UNASSIGNED &&
		    !(daddr >= fs->lfs_lastpseg && daddr <= off)) {
			LFS_SEGENTRY(sup, fs, datosn(fs, daddr), bp);
#ifdef DIAGNOSTIC
			if (sup->su_nbytes < (*sp->start_bpp)->b_bcount) {
				/* XXX -- Change to a panic. */
				printf("lfs: negative bytes (segment %d)\n",
				    datosn(fs, daddr));
				printf("lfs: bp = 0x%x, addr = 0x%x\n",
						bp, bp->b_un.b_addr);
				panic ("Negative Bytes");
			}
#endif
			sup->su_nbytes -= (*sp->start_bpp)->b_bcount;
			error = VOP_BWRITE(bp);
		}
	}
}

/*
 * Start a new segment.
 */
int
lfs_initseg(fs)
	struct lfs *fs;
{
	struct segment *sp;
	SEGUSE *sup;
	SEGSUM *ssp;
	struct buf *bp;
	int repeat;

	sp = fs->lfs_sp;

	repeat = 0;
	/* Advance to the next segment. */
	if (!LFS_PARTIAL_FITS(fs)) {
		/* Wake up any cleaning procs waiting on this file system. */
		wakeup(&lfs_allclean_wakeup);
		wakeup(&fs->lfs_nextseg);

		lfs_newseg(fs);
		repeat = 1;
		fs->lfs_offset = fs->lfs_curseg;
		sp->seg_number = datosn(fs, fs->lfs_curseg);
		sp->seg_bytes_left = fs->lfs_dbpseg * DEV_BSIZE;

		/*
		 * If the segment contains a superblock, update the offset
		 * and summary address to skip over it.
		 */
		LFS_SEGENTRY(sup, fs, sp->seg_number, bp);
		if (sup->su_flags & SEGUSE_SUPERBLOCK) {
			fs->lfs_offset += LFS_SBPAD / DEV_BSIZE;
			sp->seg_bytes_left -= LFS_SBPAD;
		}
		brelse(bp);
	} else {
		sp->seg_number = datosn(fs, fs->lfs_curseg);
		sp->seg_bytes_left = (fs->lfs_dbpseg -
		    (fs->lfs_offset - fs->lfs_curseg)) * DEV_BSIZE;
	}
	fs->lfs_lastpseg = fs->lfs_offset;

	sp->fs = fs;
	sp->ibp = NULL;
	sp->ninodes = 0;

	/* Get a new buffer for SEGSUM and enter it into the buffer list. */
	sp->cbpp = sp->bpp;
	*sp->cbpp = lfs_newbuf(VTOI(fs->lfs_ivnode)->i_devvp, fs->lfs_offset,
	     LFS_SUMMARY_SIZE);
	sp->segsum = (*sp->cbpp)->b_data;
	bzero(sp->segsum, LFS_SUMMARY_SIZE);
	sp->start_bpp = ++sp->cbpp;
	fs->lfs_offset += LFS_SUMMARY_SIZE / DEV_BSIZE;

	/* Set point to SEGSUM, initialize it. */
	ssp = sp->segsum;
	ssp->ss_next = fs->lfs_nextseg;
	ssp->ss_nfinfo = ssp->ss_ninos = 0;
	ssp->ss_magic = SS_MAGIC;

	/* Set pointer to first FINFO, initialize it. */
	sp->fip = (struct finfo *)((caddr_t)sp->segsum + sizeof(SEGSUM));
	sp->fip->fi_nblocks = 0;
	sp->start_lbp = &sp->fip->fi_blocks[0];
	sp->fip->fi_lastlength = 0;

	sp->seg_bytes_left -= LFS_SUMMARY_SIZE;
	sp->sum_bytes_left = LFS_SUMMARY_SIZE - sizeof(SEGSUM);

	return (repeat);
}

/*
 * Return the next segment to write.
 */
void
lfs_newseg(fs)
	struct lfs *fs;
{
	CLEANERINFO *cip;
	SEGUSE *sup;
	struct buf *bp;
	int curseg, isdirty, sn;

	LFS_SEGENTRY(sup, fs, datosn(fs, fs->lfs_nextseg), bp);
	sup->su_flags |= SEGUSE_DIRTY | SEGUSE_ACTIVE;
	sup->su_nbytes = 0;
	sup->su_nsums = 0;
	sup->su_ninos = 0;
	(void) VOP_BWRITE(bp);

	LFS_CLEANERINFO(cip, fs, bp);
	--cip->clean;
	++cip->dirty;
	(void) VOP_BWRITE(bp);

	fs->lfs_lastseg = fs->lfs_curseg;
	fs->lfs_curseg = fs->lfs_nextseg;
	for (sn = curseg = datosn(fs, fs->lfs_curseg);;) {
		sn = (sn + 1) % fs->lfs_nseg;
		if (sn == curseg)
			panic("lfs_nextseg: no clean segments");
		LFS_SEGENTRY(sup, fs, sn, bp);
		isdirty = sup->su_flags & SEGUSE_DIRTY;
		brelse(bp);
		if (!isdirty)
			break;
	}

	++fs->lfs_nactive;
	fs->lfs_nextseg = sntoda(fs, sn);
#ifdef DOSTATS
	++lfs_stats.segsused;
#endif
}

int
lfs_writeseg(fs, sp)
	struct lfs *fs;
	struct segment *sp;
{
	extern int locked_queue_count;
	struct buf **bpp, *bp, *cbp;
	struct vnode *vp;
	struct inode *ip;
	SEGUSE *sup;
	SEGSUM *ssp;
	dev_t i_dev;
	u_long *datap, *dp;
	int do_again, i, nblocks, s;
	u_short ninos;
	char *p;
	
	vp = fs->lfs_ivnode;
	ip = VTOI(vp);

	/*
	 * If there are no buffers other than the segment summary to write
	 * and it is not a checkpoint, don't do anything.  On a checkpoint,
	 * even if there aren't any buffers, you need to write the superblock.
	 */
	if ((nblocks = sp->cbpp - sp->bpp) == 1)
		return (0);

	/* Update the segment usage information. */
	LFS_SEGENTRY(sup, fs, sp->seg_number, bp);

	/* Loop through all blocks, except the segment summary. */
	for (bpp = sp->bpp; ++bpp < sp->cbpp; )
		sup->su_nbytes += (*bpp)->b_bcount;

	ssp = (SEGSUM *)sp->segsum;

	ninos = (ssp->ss_ninos + INOPB(fs) - 1) / INOPB(fs);
	if (I_IS_UFS1(ip)) {
		sup->su_nbytes += ssp->ss_ninos * sizeof(struct ufs1_dinode);
	} else {
		sup->su_nbytes += ssp->ss_ninos * sizeof(struct ufs2_dinode);
	}
	sup->su_nbytes += LFS_SUMMARY_SIZE;
	sup->su_lastmod = time.tv_sec;
	sup->su_ninos += ninos;
	++sup->su_nsums;
	do_again = !(bp->b_flags & B_GATHERED);
	(void)VOP_BWRITE(bp);

	/*
	 * Compute checksum across data and then across summary; the first
	 * block (the summary block) is skipped.  Set the create time here
	 * so that it's guaranteed to be later than the inode mod times.
	 *
	 * XXX
	 * Fix this to do it inline, instead of malloc/copy.
	 */
	datap = dp = malloc(nblocks * sizeof(u_long), M_SEGMENT, M_WAITOK);
	for (bpp = sp->bpp, i = nblocks - 1; i--;) {
		if ((*++bpp)->b_flags & B_INVAL) {
			if (copyin((*bpp)->b_saveaddr, dp++, sizeof(u_long)))
				panic("lfs_writeseg: copyin failed");
		} else {
			*dp++ = ((u_long *)(*bpp)->b_data)[0];
		}
	}
	ssp->ss_create = time.tv_sec;
	ssp->ss_datasum = cksum(datap, (nblocks - 1) * sizeof(u_long));
	ssp->ss_sumsum = cksum(&ssp->ss_datasum, LFS_SUMMARY_SIZE - sizeof(ssp->ss_sumsum));
	free(datap, M_SEGMENT);
#ifdef DIAGNOSTIC
	if (fs->lfs_bfree < fsbtodb(fs, ninos) + LFS_SUMMARY_SIZE / DEV_BSIZE)
		panic("lfs_writeseg: No diskspace for summary");
#endif
	fs->lfs_bfree -= (fsbtodb(fs, ninos) + LFS_SUMMARY_SIZE / DEV_BSIZE);

	i_dev = ip->i_dev;
	VOP_STRATEGY(bp);

	/*
	 * When we simply write the blocks we lose a rotation for every block
	 * written.  To avoid this problem, we allocate memory in chunks, copy
	 * the buffers into the chunk and write the chunk.  MAXPHYS is the
	 * largest size I/O devices can handle.
	 * When the data is copied to the chunk, turn off the the B_LOCKED bit
	 * and brelse the buffer (which will move them to the LRU list).  Add
	 * the B_CALL flag to the buffer header so we can count I/O's for the
	 * checkpoints and so we can release the allocated memory.
	 *
	 * XXX
	 * This should be removed if the new virtual memory system allows us to
	 * easily make the buffers contiguous in kernel memory and if that's
	 * fast enough.
	 */
	for (bpp = sp->bpp, i = nblocks; i;) {
		cbp = lfs_newbuf(ip->i_devvp, (*bpp)->b_blkno, MAXPHYS);
		cbp->b_dev = i_dev;
		cbp->b_flags |= B_ASYNC | B_BUSY;
		cbp->b_bcount = 0;

		s = splbio();
		++fs->lfs_iocount;
		for (p = cbp->b_data; i && cbp->b_bcount < MAXPHYS; i--) {
			bp = *bpp;
			if (bp->b_bcount > (MAXPHYS - cbp->b_bcount))
				break;
			bpp++;

			/*
			 * Fake buffers from the cleaner are marked as B_INVAL.
			 * We need to copy the data from user space rather than
			 * from the buffer indicated.
			 * XXX == what do I do on an error?
			 */
			if (bp->b_flags & B_INVAL) {
				if (copyin(bp->b_saveaddr, p, bp->b_bcount))
					panic("lfs_writeseg: copyin failed");
			} else
				bcopy(bp->b_data, p, bp->b_bcount);
			p += bp->b_bcount;
			cbp->b_bcount += bp->b_bcount;
			if (bp->b_flags & B_LOCKED)
				--locked_queue_count;
			bp->b_flags &= ~(B_ERROR | B_READ | B_DELWRI |
			     B_LOCKED | B_GATHERED);
			if (bp->b_flags & B_CALL) {
				/* if B_CALL, it was created with newbuf */
				brelvp(bp);
				if (!(bp->b_flags & B_INVAL))
					free(bp->b_data, M_SEGMENT);
				free(bp, M_SEGMENT);
			} else {
				bremfree(bp);
				bp->b_flags |= B_DONE;
				reassignbuf(bp, bp->b_vp);
				brelse(bp);
			}
		}
		++cbp->b_vp->v_numoutput;
		splx(s);
		/*
		 * XXXX This is a gross and disgusting hack.  Since these
		 * buffers are physically addressed, they hang off the
		 * device vnode (devvp).  As a result, they have no way
		 * of getting to the LFS superblock or lfs structure to
		 * keep track of the number of I/O's pending.  So, I am
		 * going to stuff the fs into the saveaddr field of
		 * the buffer (yuk).
		 */
		cbp->b_saveaddr = (caddr_t)fs;
		VOP_STRATEGY(cbp);
	}
	/*
	 * XXX
	 * Vinvalbuf can move locked buffers off the locked queue
	 * and we have no way of knowing about this.  So, after
	 * doing a big write, we recalculate how many bufers are
	 * really still left on the locked queue.
	 */
	locked_queue_count = count_lock_queue();
	wakeup(&locked_queue_count);
#ifdef DOSTATS
	++lfs_stats.psegwrites;
	lfs_stats.blocktot += nblocks - 1;
	if (fs->lfs_sp->seg_flags & SEGM_SYNC)
		++lfs_stats.psyncwrites;
	if (fs->lfs_sp->seg_flags & SEGM_CLEAN) {
		++lfs_stats.pcleanwrites;
		lfs_stats.cleanblocks += nblocks - 1;
	}
#endif
	return (lfs_initseg(fs) || do_again);
}

void
lfs_writesuper(fs)
	struct lfs *fs;
{
	struct buf *bp;
	struct vnode *vp;
	struct inode *ip;
	dev_t i_dev;
	int s;

	vp = fs->lfs_ivnode;
	ip = VTOI(vp);
	i_dev = ip->i_dev;
	VOP_STRATEGY(bp);

	/* Checksum the superblock and copy it into a buffer. */
	fs->lfs_cksum = cksum(fs, sizeof(struct lfs) - sizeof(fs->lfs_cksum));
	bp = lfs_newbuf(ip->i_devvp, fs->lfs_sboffs[0],
	    LFS_SBPAD);
	*(struct lfs *)bp->b_data = *fs;

	/* XXX Toggle between first two superblocks; for now just write first */
	bp->b_dev = i_dev;
	bp->b_flags |= B_BUSY | B_CALL | B_ASYNC;
	bp->b_flags &= ~(B_DONE | B_ERROR | B_READ | B_DELWRI);
	bp->b_iodone = lfs_supercallback;
	s = splbio();
	++bp->b_vp->v_numoutput;
	splx(s);
	VOP_STRATEGY(bp);
}

/*
 * Logical block number match routines used when traversing the dirty block
 * chain.
 */
int
lfs_match_data(fs, bp)
	struct lfs *fs;
	struct buf *bp;
{
	return (bp->b_lblkno >= 0);
}

int
lfs_match_indir(fs, bp)
	struct lfs *fs;
	struct buf *bp;
{
	int lbn;

	lbn = bp->b_lblkno;
	return (lbn < 0 && (-lbn - NDADDR) % NINDIR(fs) == 0);
}

int
lfs_match_dindir(fs, bp)
	struct lfs *fs;
	struct buf *bp;
{
	int lbn;

	lbn = bp->b_lblkno;
	return (lbn < 0 && (-lbn - NDADDR) % NINDIR(fs) == 1);
}

int
lfs_match_tindir(fs, bp)
	struct lfs *fs;
	struct buf *bp;
{
	int lbn;

	lbn = bp->b_lblkno;
	return (lbn < 0 && (-lbn - NDADDR) % NINDIR(fs) == 2);
}

/*
 * Allocate a new buffer header.
 */
struct buf *
lfs_newbuf(vp, daddr, size)
	struct vnode *vp;
	ufs1_daddr_t daddr;
	size_t size;
{
	struct buf *bp;
	size_t nbytes;

	nbytes = roundup(size, DEV_BSIZE);
	bp = malloc(sizeof(struct buf), M_SEGMENT, M_WAITOK);
	bzero(bp, sizeof(struct buf));
	if (nbytes)
		bp->b_data = malloc(nbytes, M_SEGMENT, M_WAITOK);
	bgetvp(vp, bp);
	bp->b_bufsize = size;
	bp->b_bcount = size;
	bp->b_lblkno = daddr;
	bp->b_blkno = daddr;
	bp->b_error = 0;
	bp->b_resid = 0;
	bp->b_iodone = lfs_callback;
	bp->b_flags |= B_BUSY | B_CALL | B_NOCACHE;
	return (bp);
}

void
lfs_callback(bp)
	struct buf *bp;
{
	struct lfs *fs;

	fs = (struct lfs *)bp->b_saveaddr;
#ifdef DIAGNOSTIC
	if (fs->lfs_iocount == 0)
		panic("lfs_callback: zero iocount\n");
#endif
	if (--fs->lfs_iocount == 0)
		wakeup(&fs->lfs_iocount);

	brelvp(bp);
	free(bp->b_data, M_SEGMENT);
	free(bp, M_SEGMENT);
}

void
lfs_supercallback(bp)
	struct buf *bp;
{
	brelvp(bp);
	free(bp->b_data, M_SEGMENT);
	free(bp, M_SEGMENT);
}

/*
 * Shellsort (diminishing increment sort) from Data Structures and
 * Algorithms, Aho, Hopcraft and Ullman, 1983 Edition, page 290;
 * see also Knuth Vol. 3, page 84.  The increments are selected from
 * formula (8), page 95.  Roughly O(N^3/2).
 */
/*
 * This is our own private copy of shellsort because we want to sort
 * two parallel arrays (the array of buffer pointers and the array of
 * logical block numbers) simultaneously.  Note that we cast the array
 * of logical block numbers to a unsigned in this routine so that the
 * negative block numbers (meta data blocks) sort AFTER the data blocks.
 */
void
lfs_shellsort(bp_array, lb_array, nmemb)
	struct buf **bp_array;
	ufs1_daddr_t *lb_array;
	register int nmemb;
{
	static int __rsshell_increments[] = { 4, 1, 0 };
	register int incr, *incrp, t1, t2;
	struct buf *bp_temp;
	u_long lb_temp;

	for (incrp = __rsshell_increments; incr == *incrp++;) {
		for (t1 = incr; t1 < nmemb; ++t1) {
			for (t2 = t1 - incr; t2 >= 0;) {
				if (lb_array[t2] > lb_array[t2 + incr]) {
					lb_temp = lb_array[t2];
					lb_array[t2] = lb_array[t2 + incr];
					lb_array[t2 + incr] = lb_temp;
					bp_temp = bp_array[t2];
					bp_array[t2] = bp_array[t2 + incr];
					bp_array[t2 + incr] = bp_temp;
					t2 -= incr;
				} else {
					break;
				}
			}
		}
	}
}

/*
 * Check VXLOCK.  Return 1 if the vnode is locked.  Otherwise, vget it.
 */
int
lfs_vref(vp)
	register struct vnode *vp;
{
	struct proc *p = curproc;	/* XXX */

	if (vp->v_flag & VXLOCK)	/* XXX */
		return(1);
	return (vget(vp, 0, p));
}

/*
 * This is vrele except that we do not want to VOP_INACTIVE this vnode. We
 * inline vrele here to avoid the vn_lock and VOP_INACTIVE call at the end.
 */
void
lfs_vunref(vp)
	register struct vnode *vp;
{
	struct proc *p = curproc;				/* XXX */
	extern struct lock_object vnode_free_list_slock;		/* XXX */
	extern TAILQ_HEAD(freelst, vnode) vnode_free_list;	/* XXX */

	simple_lock(&vp->v_interlock);
	vp->v_usecount--;
	if (vp->v_usecount > 0) {
		simple_unlock(&vp->v_interlock);
		return;
	}
	/*
	 * insert at tail of LRU list
	 */
	simple_lock(&vnode_free_list_slock);
	TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
	simple_unlock(&vnode_free_list_slock);
	simple_unlock(&vp->v_interlock);
}
