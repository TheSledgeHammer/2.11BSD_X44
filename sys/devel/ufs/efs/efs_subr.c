/*	$NetBSD: efs_subr.c,v 1.14 2021/12/10 20:36:04 andvar Exp $	*/

/*
 * Copyright (c) 2006 Stephen M. Rumble <rumble@ephemeral.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <ufs/ufs/inode.h>
#include <ufs/ufs/dinode.h>

#include <ufs/efs/efs.h>
#include <ufs/efs/efs_extent.h>

struct ufs1_dinode *
efs1_ifind(fs, ino, bp)
	struct efs *fs;
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
	panic("efs1_ifind: ufs1_dinode %u not found", ino);
	return (NULL);
}

struct ufs2_dinode *
efs2_ifind(fs, ino, bp)
	struct efs *fs;
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
	panic("efs2_ifind: ufs2_dinode %u not found", ino);
	return (NULL);
}

void
efs_locate_inode(ino, fs, ip, bboff, index)
	ino_t ino;
	struct efs *fs;
	struct inode *ip;
	uint32_t *bboff;
	int *index;
{
	uint32_t cgfsize, firstcg;
	uint16_t cgisize;

	cgisize = be16toh(fs->efs_cgisize);
	cgfsize = be32toh(fs->efs_cgfsize);
	firstcg = be32toh(fs->efs_firstcg);

	if (I_IS_UFS1(ip)) {
		*bboff = firstcg + ((ino / (cgisize * EFS1_DINODES_PER_BB)) * cgfsize)
				+ ((ino % (cgisize * EFS1_DINODES_PER_BB)) / EFS1_DINODES_PER_BB);
		*index = ino & (EFS1_DINODES_PER_BB - 1);
	} else {
		*bboff = firstcg + ((ino / (cgisize * EFS2_DINODES_PER_BB)) * cgfsize)
				+ ((ino % (cgisize * EFS2_DINODES_PER_BB)) / EFS2_DINODES_PER_BB);
		*index = ino & (EFS2_DINODES_PER_BB - 1);
	}
}

int
efs_read_inode(ump, ino, p, ip)
	struct ufsmount *ump;
	ino_t ino;
	struct proc *p;
	struct inode *ip;
{
	struct efs *fs;
	struct buf *bp;
	int index, err;
	uint32_t bboff;

	fs = &ump->um_efs;
	efs_locate_inode(ino, fs, ip, &bboff, &index);

	err = efs_bread(ump, bboff, p, &bp);
	if (err) {
		return (err);
	}

	if (I_IS_UFS1(ip)) {
		struct ufs1_dinode *dp1 = ip->i_din.ffs1_din;
		memcpy(di, ((struct ufs1_dinode *)bp->b_data) + index, sizeof(*di));
		ip->i_din.ffs1_din = dp1;
	} else {
		struct ufs2_dinode *dp2 = ip->i_din.ffs2_din;
		memcpy(di, ((struct ufs2_dinode *)bp->b_data) + index, sizeof(*di));
		ip->i_din.ffs2_din = dp2;
	}
	brelse(bp, 0);
	return (0);
}

int
efs_bread(ump, bboff, p, bp)
	struct ufsmount *ump;
	uint32_t bboff;
	struct proc *p;
	struct buf **bp;
{
	KASSERT(bboff < EFS_SIZE_MAX);

	return (bread(ump->um_devvp, (daddr_t)bboff * (EFS_BB_SIZE / DEV_BSIZE), EFS_BB_SIZE, 0, bp));
}
