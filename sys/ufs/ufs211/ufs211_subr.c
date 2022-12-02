/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_subr.c	1.5 (2.11BSD GTE) 1996/9/13
 */

#include <sys/param.h>

#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/malloc.h>

#include <ufs/ufs211/ufs211_quota.h>
#include <ufs/ufs211/ufs211_inode.h>
#include <ufs/ufs211/ufs211_fs.h>
#include <ufs/ufs211/ufs211_extern.h>

struct ufs211_bufmap ufs211buf;

static void *bufmap_alloc(void *data, long size);
static void *bufmap_free(void *data);

/*
 * Flush all the blocks associated with an inode.
 * There are two strategies based on the size of the file;
 * large files are those with more than (nbuf / 2) blocks.
 * Large files
 * 	Walk through the buffer pool and push any dirty pages
 *	associated with the device on which the file resides.
 * Small files
 *	Look up each block in the file to see if it is in the 
 *	buffer pool writing any that are found to disk.
 *	Note that we make a more stringent check of
 *	writing out any block in the buffer pool that may
 *	overlap the inode. This brings the inode up to
 *	date with recent mods to the cooked device.
 */
void
ufs211_syncip(vp)
	struct vnode *vp;
{
	register struct ufs211_inode *ip;
	register struct buf *bp;
	register struct buf *lastbufp;
	register int s;
	long lbn, lastlbn;
	daddr_t blkno, bmap;

	ip = UFS211_VTOI(vp);
	lastlbn = howmany(ip->i_size, DEV_BSIZE);
	if (lastlbn < nbuf / 2) {
		for (lbn = 0; lbn < lastlbn; lbn++) {
			bmap = ufs211_bmap1(ip, lbn, B_READ, 0);
			blkno = fsbtodb(bmap);
			blkflush(vp, blkno, ip->i_dev);
		}
	} else {
		for (bp = LIST_FIRST(vp->v_dirtyblkhd); bp; bp = lastbufp) {
			lastbufp = LIST_NEXT(bp, b_vnbufs);
			if (bp->b_dev != ip->i_dev || (bp->b_flags & B_DELWRI) == 0) {
				continue;
			}
			s = splbio();
			if (bp->b_flags & B_BUSY) {
				bp->b_flags |= B_WANTED;
				sleep((caddr_t) bp, PRIBIO + 1);
				splx(s);
				bremfree(bp);
				continue;
			}
			splx(s);
			notavail(bp, vp->v_dirtyblkhd, b_vnbufs);
			bwrite(bp);
		}
	}
	ip->i_flag |= UFS211_ICHG;
}

/*
 * Check that a specified block number is in range.
 */
int
ufs211_badblock(fp, bn)
	register struct ufs211_fs *fp;
	daddr_t bn;
{

	if (bn < fp->fs_isize || bn >= fp->fs_fsize) {
		printf("bad block %D, ",bn);
		ufs211_fserr(fp, "bad block");
		return (1);
	}
	return (0);
}

void
ufs211_bufmap_init(void)
{
	&ufs211buf = (struct ufs211_bufmap *)malloc(sizeof(struct ufs211_bufmap *), M_UFS211, M_WAITOK);
}

/* mapin buf */
void
ufs211_mapin(bp)
    void *bp;
{
    if (bufmap_alloc(bp, sizeof(bp))) {
        printf("buf allocated\n");
    }
}

/* mapout buf */
void
ufs211_mapout(bp)
	void *bp;
{
    if (bufmap_free(bp) == NULL) {
        printf("buf freed\n");
    }
}

/*
 * allocate bufmap data
 */
static void *
bufmap_alloc(data, size)
    void  	*data;
	long 	size;
{
    struct ufs211_bufmap *bm;

    bm = &ufs211buf;
    if (bm) {
    	bm->bm_data = data;
    	bm->bm_size = size;
        printf("data allocated \n");
    }
    return (bm->bm_data);
}

/*
 * free bufmap data
 */
static void *
bufmap_free(data)
    void  *data;
{
    struct ufs211_bufmap *bm;

    bm = &ufs211buf;
    if (bm->bm_data == data) {
    	if (bm->bm_data != NULL && bm->bm_size == sizeof(data)) {
    		bm->bm_data = NULL;
    		bm->bm_size = 0;
    		printf("data freed\n");
    	}
    }
    return (bm->bm_data);
}
