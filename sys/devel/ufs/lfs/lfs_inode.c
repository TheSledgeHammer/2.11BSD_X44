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

#include <ufs/ufs/quota.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <devel/ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

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

int
lfs_update(ap)
	struct vop_update_args /* {
		struct vnode *a_vp;
		struct timeval *a_access;
		struct timeval *a_modify;
		int a_waitfor;
	} */ *ap;
{
	struct vnode *vp;
	struct inode *ip;

	vp = ap->a_vp;
	if (vp->v_mount->mnt_flag & MNT_RDONLY) {
		return (0);
	}
	ip = VTOI(vp);
	if ((ip->i_flag & (IN_ACCESS | IN_CHANGE | IN_MODIFIED | IN_UPDATE)) == 0) {
		return (0);
	}
	if (ip->i_flag & IN_ACCESS) {
		LFS_DIP_SET(ip->i_lfs, atime, ap->a_access->tv_sec);
	}
	if (ip->i_flag & IN_UPDATE) {
		LFS_DIP_SET(ip->i_lfs, mtime, ap->a_modify->tv_sec);
		(ip)->i_modrev++;
	}
	if (ip->i_flag & IN_CHANGE) {
		LFS_DIP_SET(ip->i_lfs, ctime, time.tv_sec);
	}
	ip->i_flag &= ~(IN_ACCESS | IN_CHANGE | IN_UPDATE);

	if (!(ip->i_flag & IN_MODIFIED))
		++(VFSTOUFS(vp->v_mount)->um_lfs->lfs_uinodes);
	ip->i_flag |= IN_MODIFIED;

	/* If sync, push back the vnode and any dirty blocks it may have. */
	return (ap->a_waitfor & LFS_SYNC ? lfs_vflush(vp) : 0);
}
