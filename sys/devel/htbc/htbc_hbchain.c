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
 *
 * @(#)htbc_hbchain.c	1.00
 */

#include <devel/htbc/htbc.h>
#include <devel/htbc/htbc_htree.h>
#include <sys/user.h>

//flush, biodone, truncate

/*
 * Convert the logical block number of a file to its physical block number
 * on the disk within ext4 extents.
 */
int
htbc_bmapext(struct vnode *vp, int32_t bn, int64_t *bnp, int *runp, int *runb)
{
	struct htbc_hi_mfs *fs;
	struct htbc_inode *ip;
	struct htbc_extent *ep;
	struct htbc_extent_path path = { .ep_bp = NULL };
	daddr_t lbn;
	int error = 0;

	ip = VTOHTI(vp);
	fs = ip->hi_mfs;
	lbn = bn;

	/* XXX: Should not initialize on error? */
	if (runp != NULL)
		*runp = 0;

	if (runb != NULL)
		*runb = 0;

	htbc_ext_find_extent(fs, ip, lbn, &path);
	if (path.ep_is_sparse) {
		*bnp = -1;
		if (runp != NULL)
			*runp = path.ep_sparse_ext.e_len - (lbn - path.ep_sparse_ext.e_blk)
					- 1;
		if (runb != NULL)
			*runb = lbn - path.ep_sparse_ext.e_blk;
	} else {
		if (path.ep_ext == NULL) {
			error = EIO;
			goto out;
		}
		ep = path.ep_ext;
		*bnp = fsbtodb(fs,
				lbn - ep->e_blk
						+ (ep->e_start_lo | (daddr_t) ep->e_start_hi << 32));

		if (*bnp == 0)
			*bnp = -1;

		if (runp != NULL)
			*runp = ep->e_len - (lbn - ep->e_blk) - 1;
		if (runb != NULL)
			*runb = lbn - ep->e_blk;
	}

out:
	if (path.ep_bp != NULL) {
		brelse(path.ep_bp, 0);
	}

	return (error);
}

static void
htbc_doio_accounting(struct vnode *devvp, int flags)
{

}

static int
htbc_doio(data, len, devvp, pbn, flags)
	void *data;
	size_t len;
	struct vnode *devvp;
	daddr_t pbn;
	int flags;
{
	struct buf *bp;
	int error;

	KASSERT(devvp->v_type == VBLK);

	htbc_doio_accounting(devvp, flags);

	bp = getiobuf(devvp, TRUE);
	bp->b_flags = flags;
	//bp->b_cflags |= BC_BUSY;	/* mandatory, asserted by biowait() */
	bp->b_dev = devvp->v_rdev;
	bp->b_data = data;
	bp->b_bufsize = bp->b_resid = bp->b_bcount = len;
	bp->b_blkno = pbn;

	VOP_STRATEGY(bp);

	error = biowait(bp);
	putiobuf(bp);

	return error;
}
