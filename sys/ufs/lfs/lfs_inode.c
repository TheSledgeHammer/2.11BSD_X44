/*
 * Copyright (c) 1986, 1989, 1991, 1993
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
 *	@(#)lfs_inode.c	8.9 (Berkeley) 5/8/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <vm/include/vm.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

/* Search a block for a specific dinode. */
struct ufs1_dinode *
lfs1_ifind(fs, ino, bp)
	struct lfs *fs;
	ino_t ino;
	struct buf *bp;
{
	struct ufs1_dinode *dip;
	struct ufs1_dinode *ldip, *fin;

	dip = (struct ufs1_dinode *)bp->b_data;
	fin = dip + INOPB(fs);
	for (ldip = fin - 1; ldip >= dip; --ldip) {
		if (ldip->di_inumber == ino) {
			return (ldip);
		}
	}
	panic("lfs1_ifind: ufs1_dinode %u not found", ino);
	return (NULL);
}

struct ufs2_dinode *
lfs2_ifind(fs, ino, bp)
	struct lfs *fs;
	ino_t ino;
	struct buf *bp;
{
	struct ufs2_dinode *dip;
	struct ufs2_dinode *ldip, *fin;

	dip = (struct ufs2_dinode *)bp->b_data;
	fin = dip + INOPB(fs);
	for (ldip = fin - 1; ldip >= dip; --ldip) {
		if (ldip->di_inumber == ino) {
			return (ldip);
		}
	}
	panic("lfs2_ifind: ufs2_dinode %u not found", ino);
	return (NULL);
}

void *
lfs_ifind(fs, ino, bp)
	struct lfs *fs;
	ino_t ino;
	struct buf *bp;
{
	struct ufs1_dinode *din1;
	struct ufs2_dinode *din2;
	void 			   *data;

	if(UFS1) {
		din1 = lfs1_ifind(fs, ino, bp);
		data = din1;
	} else {
		din2 = lfs2_ifind(fs, ino, bp);
		data = din2;
	}
	return (data);
}

int
lfs_update(ap)
	struct vop_update_args /* {
		struct vnode *a_vp;
		struct timeval *a_access;
		struct timeval *a_modify;
		int a_waitfor;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct inode *ip;

	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		return (0);
	ip = VTOI(vp);
	if ((ip->i_flag &
	    (IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE)) == 0)
		return (0);
	if (ip->i_flag & IN_ACCESS)
		DIP_SET(ip, atime, ap->a_access->tv_sec);
	if (ip->i_flag & IN_UPDATE) {
		DIP_SET(ip, mtime, ap->a_modify->tv_sec);
		(ip)->i_modrev++;
	}
	if (ip->i_flag & IN_CHANGE)
		DIP_SET(ip, ctime, time.tv_sec);
	ip->i_flag &= ~(IN_ACCESS | IN_CHANGE | IN_UPDATE);

	if (!(ip->i_flag & IN_MODIFIED))
		++(VFSTOUFS(vp->v_mount)->um_lfs->lfs_uinodes);
	ip->i_flag |= IN_MODIFIED;

	/* If sync, push back the vnode and any dirty blocks it may have. */
	return (ap->a_waitfor & LFS_SYNC ? lfs_vflush(vp) : 0);
}

/* Update segment usage information when removing a block. */
#define UPDATE_SEGUSE 												\
	if (lastseg != -1) { 											\
		LFS_SEGENTRY(sup, fs, lastseg, sup_bp); 					\
		if (num > sup->su_nbytes) 									\
			panic("lfs_truncate: negative bytes in segment %d\n", 	\
			    lastseg); 											\
		sup->su_nbytes -= num; 										\
		e1 = VOP_BWRITE(sup_bp); 									\
		fragsreleased += numfrags(fs, num); 						\
	}

#define SEGDEC(S) { 												\
	if (daddr != 0) { 												\
		if (lastseg != (seg = datosn(fs, daddr))) { 				\
			UPDATE_SEGUSE; 											\
			num = (S); 												\
			lastseg = seg; 											\
		} else 														\
			num += (S); 											\
	} 																\
}

/*
 * Truncate the inode ip to at most length size.  Update segment usage
 * table information.
 */
/* ARGSUSED */
int
lfs_truncate(ap)
	struct vop_truncate_args /* {
		struct vnode *a_vp;
		off_t a_length;
		int a_flags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	register struct indir *inp;
	register int i;
	register ufs2_daddr_t *daddrp;
	register struct vnode *vp = ap->a_vp;
	off_t length = ap->a_length;
	struct buf *bp, *sup_bp;
	struct timeval tv;
	struct ifile *ifp;
	struct inode *ip;
	struct lfs *fs;
	struct indir a[NIADDR + 2], a_end[NIADDR + 2];
	SEGUSE *sup;
	daddr_t lbn, daddr;
  	ufs2_daddr_t lastblock, olastblock;
	ufs2_daddr_t oldsize_lastblock, oldsize_newlast, newsize;
	long off, a_released, fragsreleased, i_released;
	int e1, e2, depth, lastseg, num, offset, seg, freesize;

	ip = VTOI(vp);
	tv = time;
	if (vp->v_type == VLNK && vp->v_mount->mnt_maxsymlinklen > 0) {
#ifdef DIAGNOSTIC
		if (length != 0)
			panic("lfs_truncate: partial truncate of symlink");
#endif
		bzero((char *)SHORTLINK(ip), (u_int)ip->i_size);
		ip->i_size = 0;
		ip->i_flag |= IN_CHANGE | IN_UPDATE;
		return (VOP_UPDATE(vp, &tv, &tv, 0));
	}
	vnode_pager_setsize(vp, (u_long)length);

	fs = ip->i_lfs;

	/* If length is larger than the file, just update the times. */
	if (ip->i_size <= length) {
		ip->i_flag |= IN_CHANGE | IN_UPDATE;
		return (VOP_UPDATE(vp, &tv, &tv, 0));
	}

	/*
	 * Calculate index into inode's block list of last direct and indirect
	 * blocks (if any) which we want to keep.  Lastblock is 0 when the
	 * file is truncated to 0.
	 */
	lastblock = lblkno(fs, length + fs->lfs_bsize - 1);
	olastblock = lblkno(fs, ip->i_size + fs->lfs_bsize - 1) - 1;

	/*
	 * Update the size of the file. If the file is not being truncated to
	 * a block boundry, the contents of the partial block following the end
	 * of the file must be zero'ed in case it ever become accessable again
	 * because of subsequent file growth.  For this part of the code,
	 * oldsize_newlast refers to the old size of the new last block in the file.
	 */
	offset = blkoff(fs, length);
	lbn = lblkno(fs, length);
	oldsize_newlast = blksize(fs, ip, lbn);

	/* Now set oldsize to the current size of the current last block */
	oldsize_lastblock = blksize(fs, ip, olastblock);
	if (offset == 0)
		ip->i_size = length;
	else {
#ifdef QUOTA
		if (e1 == getinoquota(ip))
			return (e1);
#endif	
		if (e1 == bread(vp, lbn, oldsize_newlast, NOCRED, &bp))
			return (e1);
		ip->i_size = length;
		(void)vnode_pager_uncache(vp);
		newsize = blksize(fs, ip, lbn);
		bzero((char *)bp->b_data + offset, (u_int)(newsize - offset));
		allocbuf(bp, newsize);
		if (e1 == VOP_BWRITE(bp))
			return (e1);
	}
	/*
	 * Modify sup->su_nbyte counters for each deleted block; keep track
	 * of number of blocks removed for ip->i_blocks.
	 */
	fragsreleased = 0;
	num = 0;
	lastseg = -1;

	for (lbn = olastblock; lbn >= lastblock;) {
		/* XXX use run length from bmap array to make this faster */
		ufs_bmaparray(vp, lbn, &daddr, a, &depth, NULL);
		if (lbn == olastblock) {
			for (i = NIADDR + 2; i--;)
				a_end[i] = a[i];
			freesize = oldsize_lastblock;
		} else
			freesize = fs->lfs_bsize;

		switch (depth) {
		case 0:				/* Direct block. */
			//daddr = ip->i_ffs1_db[lbn];
			daddr = DIP(ip, db[lbn]);
			SEGDEC(freesize);
			ip->i_ffs1_db[lbn] = 0;
			--lbn;
			break;
#ifdef DIAGNOSTIC
		case 1:				/* An indirect block. */
			panic("lfs_truncate: ufs_bmaparray returned depth 1");
			/* NOTREACHED */
#endif
		default:			/* Chain of indirect blocks. */
			inp = a + --depth;
			if (inp->in_off > 0 && lbn != lastblock) {
				lbn -= inp->in_off < lbn - lastblock ?
				    inp->in_off : lbn - lastblock;
				break;
			}
			for (; depth && (inp->in_off == 0 || lbn == lastblock);
			    --inp, --depth) {
				if (bread(vp,
				    inp->in_lbn, fs->lfs_bsize, NOCRED, &bp))
					panic("lfs_truncate: bread bno %d",
					    inp->in_lbn);
				daddrp = (ufs2_daddr_t *)bp->b_data +
				    inp->in_off;
				for (i = inp->in_off;
				    i++ <= a_end[depth].in_off;) {
					daddr = *daddrp++;
					SEGDEC(freesize);
				}
				a_end[depth].in_off = NINDIR(fs) - 1;
				if (inp->in_off == 0)
					brelse (bp);
				else {
					bzero((ufs2_daddr_t *)bp->b_data +
					    inp->in_off, fs->lfs_bsize - 
					    inp->in_off * sizeof(ufs2_daddr_t));
					if (e1 == VOP_BWRITE(bp))
						return (e1);
				}
			}
			if (depth == 0 && a[1].in_off == 0) {
				off = a[0].in_off;
				daddr = ip->i_ffs1_ib[off];
				SEGDEC(freesize);
				ip->i_ffs1_ib[off] = 0;
			}
			if (lbn == lastblock || lbn <= NDADDR)
				--lbn;
			else {
				lbn -= NINDIR(fs);
				if (lbn < lastblock)
					lbn = lastblock;
			}
		}
	}
	UPDATE_SEGUSE;

	/* If truncating the file to 0, update the version number. */
	if (length == 0) {
		LFS_IENTRY(ifp, fs, ip->i_number, bp);
		++ifp->if_version;
		(void) VOP_BWRITE(bp);
	}

#ifdef DIAGNOSTIC
	if (DIP(ip, blocks) < fragstodb(fs, fragsreleased)) {
		printf("lfs_truncate: frag count < 0\n");
		fragsreleased = dbtofrags(fs, DIP(ip, blocks));
		panic("lfs_truncate: frag count < 0\n");
	}
#endif
	DIP_SET(ip, blocks, DIP(ip, blocks) - fragstodb(fs, fragsreleased));
	fs->lfs_bfree +=  fragstodb(fs, fragsreleased);
	ip->i_flag |= IN_CHANGE | IN_UPDATE;
	/*
	 * Traverse dirty block list counting number of dirty buffers
	 * that are being deleted out of the cache, so that the lfs_avail
	 * field can be updated.
	 */
	a_released = 0;
	i_released = 0;
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = LIST_NEXT(bp, b_vnbufs))
		if (bp->b_flags & B_LOCKED) {
			a_released += numfrags(fs, bp->b_bcount);
			/*
			 * XXX
			 * When buffers are created in the cache, their block
			 * number is set equal to their logical block number.
			 * If that is still true, we are assuming that the
			 * blocks are new (not yet on disk) and weren't
			 * counted above.  However, there is a slight chance
			 * that a block's disk address is equal to its logical
			 * block number in which case, we'll get an overcounting
			 * here.
			 */
			if (bp->b_blkno == bp->b_lblkno)
				i_released += numfrags(fs, bp->b_bcount);
		}
	fragsreleased = i_released;
#ifdef DIAGNOSTIC
	if (fragsreleased > dbtofrags(fs, DIP(ip, blocks))) {
		printf("lfs_inode: Warning! %s\n",
		    "more frags released from inode than are in inode");
		fragsreleased = dbtofrags(fs, DIP(ip, blocks));
		panic("lfs_inode: Warning.  More frags released\n");
	}
#endif
	fs->lfs_bfree += fragstodb(fs, fragsreleased);
	DIP_SET(ip, blocks, DIP(ip, blocks) - fragstodb(fs, fragsreleased));
#ifdef DIAGNOSTIC
	if (length == 0 && DIP(ip, blocks) != 0) {
		printf("lfs_inode: Warning! %s%d%s\n",
		    "Truncation to zero, but ", DIP(ip, blocks),
		    " blocks left on inode");
		panic("lfs_inode");
	}
#endif
	fs->lfs_avail += fragstodb(fs, a_released);
	e1 = vinvalbuf(vp, (length > 0) ? V_SAVE : 0, ap->a_cred, ap->a_p,
	    0, 0); 
	e2 = VOP_UPDATE(vp, &tv, &tv, 0);
	return (e1 ? e1 : e2 ? e2 : 0);
}
