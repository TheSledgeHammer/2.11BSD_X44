/*	$NetBSD: msdosfs_vnops.c,v 1.9.4.1 2005/09/06 16:12:00 riz Exp $	*/

/*-
 * Copyright (C) 1994, 1995, 1997 Wolfgang Solfrank.
 * Copyright (C) 1994, 1995, 1997 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: msdosfs_vnops.c,v 1.9.4.1 2005/09/06 16:12:00 riz Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>	/* defines plimit structure in proc struct */
#include <sys/kernel.h>
#include <sys/file.h>		/* define FWRITE ... */
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/lockf.h>

#include <vm/include/vm.h>

#include <miscfs/specfs/specdev.h> /* XXX */	/* defines v_rdev */

#include <fs/msdosfs/bpb.h>
#include <fs/msdosfs/direntry.h>
#include <fs/msdosfs/denode.h>
#include <fs/msdosfs/msdosfsmount.h>
#include <fs/msdosfs/fat.h>

/*
 * Some general notes:
 *
 * In the ufs filesystem the inodes, superblocks, and indirect blocks are
 * read/written using the vnode for the filesystem. Blocks that represent
 * the contents of a file are read/written using the vnode for the file
 * (including directories when they are read/written as files). This
 * presents problems for the dos filesystem because data that should be in
 * an inode (if dos had them) resides in the directory itself.  Since we
 * must update directory entries without the benefit of having the vnode
 * for the directory we must use the vnode for the filesystem.  This means
 * that when a directory is actually read/written (via read, write, or
 * readdir, or seek) we must use the vnode for the filesystem instead of
 * the vnode for the directory as would happen in ufs. This is to insure we
 * retreive the correct block from the buffer cache since the hash value is
 * based upon the vnode address and the desired block number.
 */

/*
 * Create a regular file. On entry the directory to contain the file being
 * created is locked.  We must release before we return. We must also free
 * the pathname buffer pointed at by cnp->cn_pnbuf, always on error, or
 * only if the SAVESTART bit in cn_flags is clear on success.
 */
int
msdosfs_create(ap)
	struct vop_create_args *ap;
{
	struct componentname *cnp = ap->a_cnp;
	struct denode ndirent;
	struct denode *dep;
	struct denode *pdep = VTODE(ap->a_dvp);
	int error;
	struct timespec ts;

#ifdef MSDOSFS_DEBUG
	printf("msdosfs_create(cnp %08x, vap %08x\n", cnp, ap->a_vap);
#endif

	/*
	 * If this is the root directory and there is no space left we
	 * can't do anything.  This is because the root directory can not
	 * change size.
	 */
	if (pdep->de_StartCluster == MSDOSFSROOT
	    && pdep->de_fndoffset >= pdep->de_FileSize) {
		error = ENOSPC;
		goto bad;
	}

	/*
	 * Create a directory entry for the file, then call createde() to
	 * have it installed. NOTE: DOS files are always executable.  We
	 * use the absence of the owner write bit to make the file
	 * readonly.
	 */
#ifdef DIAGNOSTIC
	if ((cnp->cn_flags & HASBUF) == 0)
		panic("msdosfs_create: no name");
#endif
	bzero(&ndirent, sizeof(ndirent));
	if ((error = uniqdosname(pdep, cnp, ndirent.de_Name)) != 0)
		goto bad;

	ndirent.de_Attributes = (ap->a_vap->va_mode & VWRITE) ? ATTR_ARCHIVE : ATTR_ARCHIVE | ATTR_READONLY;
	ndirent.de_StartCluster = 0;
	ndirent.de_FileSize = 0;
	ndirent.de_dev = pdep->de_dev;
	ndirent.de_devvp = pdep->de_devvp;
	ndirent.de_pmp = pdep->de_pmp;
	ndirent.de_flag = DE_ACCESS | DE_CREATE | DE_UPDATE;
	vfs_timestamp(&ts);
	TIMEVAL_TO_TIMESPEC(&time, &ts);
	DETIMES(&ndirent, &ts, &ts, &ts, pdep->de_pmp->pm_gmtoff);
	error = createde(&ndirent, pdep, &dep, cnp);
	if (error != 0)
		goto bad;
	if ((cnp->cn_flags & SAVESTART) == 0)
		FREE(cnp->cn_pnbuf, M_NAMEI);
	vput(ap->a_dvp);
	*ap->a_vpp = DETOV(dep);
	return (0);

bad:
	FREE(cnp->cn_pnbuf, M_NAMEI);
	vput(ap->a_dvp);
	return (error);
}

int
msdosfs_mknod(ap)
	struct vop_mknod_args *ap;
{
	switch (ap->a_vap->va_type) {
	case VDIR:
		return (msdosfs_mkdir((struct vop_mkdir_args *)ap));
		break;

	case VREG:
		return (msdosfs_create((struct vop_create_args *)ap));
		break;

	default:
		FREE(ap->a_cnp->cn_pnbuf, M_NAMEI);
		vput(ap->a_dvp);
		return (EINVAL);
	}
	/* NOTREACHED */
}

int
msdosfs_open(ap)
	struct vop_open_args *ap;
{

	return (0);
}

int
msdosfs_close(ap)
	struct vop_close_args *ap;
{
	struct vnode *vp = ap->a_vp;
	struct denode *dep = VTODE(vp);
	struct timespec ts;

	simple_lock(&vp->v_interlock);
	if (vp->v_usecount > 1) {
		TIMEVAL_TO_TIMESPEC(&time, &ts);
		DETIMES(dep, &ts, &ts, &ts, dep->de_pmp->pm_gmtoff);
	}
	simple_unlock(&vp->v_interlock);
	return (0);
}

int
msdosfs_access(ap)
	struct vop_access_args *ap;
{
	struct vnode *vp = ap->a_vp;
	struct denode *dep = VTODE(vp);
	struct msdosfsmount *pmp = dep->de_pmp;
	mode_t mode = ap->a_mode;

	/*
	 * Disallow write attempts on read-only file systems;
	 * unless the file is a socket, fifo, or a block or
	 * character device resident on the file system.
	 */
	if (mode & VWRITE) {
		switch (vp->v_type) {
		case VDIR:
		case VLNK:
		case VREG:
			if (vp->v_mount->mnt_flag & MNT_RDONLY) {
				return (EROFS);
			}
		default:
			break;
		}
	}

	if ((dep->de_Attributes & ATTR_READONLY) == 0)
		mode = S_IRWXU | S_IRWXG | S_IRWXO;
	else
		mode = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	return (vaccess(ap->a_vp->v_type,
			mode & (vp->v_type == VDIR ? pmp->pm_dirmask : pmp->pm_mask),
			pmp->pm_uid, pmp->pm_gid, ap->a_mode, ap->a_cred));
}

int
msdosfs_getattr(ap)
	struct vop_getattr_args *ap;
{
	u_int cn;
	struct denode *dep = VTODE(ap->a_vp);
	struct vattr *vap = ap->a_vap;
	struct msdosfsmount *pmp = dep->de_pmp;
	mode_t mode;
	struct timespec ts;
	u_long dirsperblk = pmp->pm_BytesPerSec / sizeof(struct direntry);
	u_long fileid;

	TIMEVAL_TO_TIMESPEC(&time, &ts);
	DETIMES(dep, &ts, &ts, &ts, pmp->pm_gmtoff);
	vfs_getnewfsid(pmp->pm_mountp);

	/*
	 * The following computation of the fileid must be the same as that
	 * used in msdosfs_readdir() to compute d_fileno. If not, pwd
	 * doesn't work.
	 */
	if (dep->de_Attributes & ATTR_DIRECTORY) {
		fileid = cntobn(pmp, dep->de_StartCluster) * dirsperblk;
		if (dep->de_StartCluster == MSDOSFSROOT)
			fileid = 1;
	} else {
		fileid = cntobn(pmp, dep->de_dirclust) * dirsperblk;
		if (dep->de_dirclust == MSDOSFSROOT)
			fileid = roottobn(pmp, 0) * dirsperblk;
		fileid += dep->de_diroffset / sizeof(struct direntry);
	}
	vap->va_fileid = fileid;
	if ((dep->de_Attributes & ATTR_READONLY) == 0)
		mode = S_IRWXU | S_IRWXG | S_IRWXO;
	else
		mode = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	vap->va_mode = mode
			& (ap->a_vp->v_type == VDIR ? pmp->pm_dirmask : pmp->pm_mask);
	vap->va_uid = pmp->pm_uid;
	vap->va_gid = pmp->pm_gid;
	vap->va_nlink = 1;
	vap->va_rdev = NODEV;
	vap->va_size = dep->de_FileSize;
	dos2unixtime(dep->de_MDate, dep->de_MTime, 0, pmp->pm_gmtoff,
			&vap->va_mtime);
	if (dep->de_pmp->pm_flags & MSDOSFSMNT_LONGNAME) {
		dos2unixtime(dep->de_ADate, 0, 0, pmp->pm_gmtoff, &vap->va_atime);
		dos2unixtime(dep->de_CDate, dep->de_CTime, dep->de_CHun, pmp->pm_gmtoff,
				&vap->va_ctime);
	} else {
		vap->va_atime = vap->va_mtime;
		vap->va_ctime = vap->va_mtime;
	}
	vap->va_flags = 0;
	if (dep->de_Attributes & ATTR_ARCHIVE)
		vap->va_flags |= UF_ARCHIVE;
	if (dep->de_Attributes & ATTR_HIDDEN)
		vap->va_flags |= UF_HIDDEN;
	if (dep->de_Attributes & ATTR_READONLY)
		vap->va_flags |= UF_READONLY;
	if (dep->de_Attributes & ATTR_SYSTEM)
		vap->va_flags |= UF_SYSTEM;
	vap->va_gen = 0;
	vap->va_blocksize = pmp->pm_bpcluster;
	vap->va_bytes = (dep->de_FileSize + pmp->pm_crbomask) & ~pmp->pm_crbomask;
	vap->va_type = ap->a_vp->v_type;
	vap->va_filerev = dep->de_modrev;
	return (0);
}

int
msdosfs_setattr(ap)
	struct vop_setattr_args *ap;
{
	int error = 0, de_changed = 0;
	struct denode *dep = VTODE(ap->a_vp);
	struct msdosfsmount *pmp = dep->de_pmp;
	struct vnode *vp  = ap->a_vp;
	struct vattr *vap = ap->a_vap;
	struct ucred *cred = ap->a_cred;

#ifdef MSDOSFS_DEBUG
	printf("msdosfs_setattr(): vp %p, vap %p, cred %p, p %p\n",
	    ap->a_vp, vap, cred, ap->a_p);
#endif
	/*
	 * Note we silently ignore uid or gid changes.
	 */
	if ((vap->va_type != VNON) || (vap->va_nlink != (nlink_t)VNOVAL)
			|| (vap->va_fsid != VNOVAL) || (vap->va_fileid != VNOVAL)
			|| (vap->va_blocksize != VNOVAL) || (vap->va_rdev != VNOVAL)
			|| (vap->va_bytes != VNOVAL) || (vap->va_gen != VNOVAL)
			|| (vap->va_uid != VNOVAL && vap->va_uid != pmp->pm_uid)
			|| (vap->va_gid != VNOVAL && vap->va_gid != pmp->pm_gid)) {
#ifdef MSDOSFS_DEBUG
		printf("msdosfs_setattr(): returning EINVAL\n");
		printf("    va_type %d, va_nlink %x, va_fsid %lx, va_fileid %lx\n",
				vap->va_type, vap->va_nlink, vap->va_fsid, vap->va_fileid);
		printf("    va_blocksize %lx, va_rdev %x, va_bytes %qx, va_gen %lx\n",
				vap->va_blocksize, vap->va_rdev, (long long) vap->va_bytes,
				vap->va_gen);
#endif
		return (EINVAL);
	}
	/*
	 * Silently ignore attributes modifications on directories.
	 */
	if (ap->a_vp->v_type == VDIR)
		return 0;

	if (vap->va_size != VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY)
			return (EROFS);
		error = detrunc(dep, (u_long) vap->va_size, 0, cred, ap->a_p);
		if (error)
			return (error);
		de_changed = 1;
	}
	if (vap->va_atime.tv_sec != VNOVAL || vap->va_mtime.tv_sec != VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY)
			return (EROFS);
		if (cred->cr_uid != pmp->pm_uid
				&& (error = suser1(cred, &ap->a_p->p_acflag))
				&& ((vap->va_vaflags & VA_UTIMES_NULL) == 0 || (error =
						VOP_ACCESS(ap->a_vp, VWRITE, cred, ap->a_p))))
			return (error);
		if ((pmp->pm_flags & MSDOSFSMNT_NOWIN95) == 0&&
		vap->va_atime.tv_sec != VNOVAL)
			unix2dostime(&vap->va_atime, pmp->pm_gmtoff, &dep->de_ADate, NULL,
			NULL);
		if (vap->va_mtime.tv_sec != VNOVAL)
			unix2dostime(&vap->va_mtime, pmp->pm_gmtoff, &dep->de_MDate,
					&dep->de_MTime, NULL);
		dep->de_Attributes |= ATTR_ARCHIVE;
		dep->de_flag |= DE_MODIFIED;
		de_changed = 1;
	}
	/*
	 * DOS files only have the ability to have their writability
	 * attribute set, so we use the owner write bit to set the readonly
	 * attribute.
	 */
	if (vap->va_mode != (mode_t) VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY)
			return (EROFS);
		if (cred->cr_uid != pmp->pm_uid
				&& (error = suser1(cred, &ap->a_p->p_acflag)))
			return (error);
		/* We ignore the read and execute bits. */
		if (vap->va_mode & S_IWUSR)
			dep->de_Attributes &= ~ATTR_READONLY;
		else
			dep->de_Attributes |= ATTR_READONLY;
		dep->de_flag |= DE_MODIFIED;
		de_changed = 1;
	}
	/*
	 * Allow the `archived' bit to be toggled.
	 */
	if (vap->va_flags != VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY)
			return (EROFS);
		if (cred->cr_uid != pmp->pm_uid
				&& (error = suser1(cred, &ap->a_p->p_acflag)))
			return (error);
		if (vap->va_flags & SF_ARCHIVED)
			dep->de_Attributes &= ~ATTR_ARCHIVE;
		else
			dep->de_Attributes |= ATTR_ARCHIVE;
		dep->de_flag |= DE_MODIFIED;
		de_changed = 1;
	}

	if (de_changed) {
		return (deupdat(dep, 1));
	} else
		return (0);
}

int
msdosfs_read(ap)
	struct vop_read_args *ap;
{
	int error = 0;
	int64_t diff;
	int blsize;
	int orig_resid;
	long n;
	long on;
	daddr_t lbn;
	daddr_t rablock;
	int rasize;
	int seqcount;
	int isadir;
	struct buf *bp;
	struct vnode *vp = ap->a_vp;
	struct denode *dep = VTODE(vp);
	struct msdosfsmount *pmp = dep->de_pmp;
	struct uio *uio = ap->a_uio;

	/*
	 * If they didn't ask for any data, then we are done.
	 */

	if (uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset < 0)
		return (EINVAL);

	/*
	 * If they didn't ask for any data, then we are done.
	 */
	orig_resid = uio->uio_resid;
	if (orig_resid <= 0)
		return (0);

	seqcount = ap->a_ioflag >> 16;

	isadir = dep->de_Attributes & ATTR_DIRECTORY;
	do {
		if (uio->uio_offset >= dep->de_FileSize)
			break;
		lbn = de_cluster(pmp, uio->uio_offset);
		/*
		 * If we are operating on a directory file then be sure to
		 * do i/o with the vnode for the filesystem instead of the
		 * vnode for the directory.
		 */
		if (isadir) {
			/* convert cluster # to block # */
			error = pcbmap(dep, lbn, &lbn, 0, &blsize);
			if (error == E2BIG) {
				error = EINVAL;
				break;
			} else if (error)
				break;
			error = bread(pmp->pm_devvp, lbn, blsize, NOCRED, &bp);
		} else {
			blsize = pmp->pm_bpcluster;
			rablock = lbn + 1;
			if (seqcount > 1 &&
			    de_cn2off(pmp, rablock) < dep->de_FileSize) {
				rasize = pmp->pm_bpcluster;
				error = breadn(vp, lbn, blsize, &rablock, &rasize, 1, NOCRED,
						&bp);
			} else {
				error = bread(vp, lbn, blsize, NOCRED, &bp);
			}
		}
		if (error) {
			brelse(bp);
			break;
		}
		on = uio->uio_offset & pmp->pm_crbomask;
		n = MIN(pmp->pm_bpcluster - on, uio->uio_resid);
		if (uio->uio_offset >= dep->de_FileSize)
			return (0);
		/* file size (and hence diff) may be up to 4GB */
		diff = dep->de_FileSize - uio->uio_offset;
		if (diff < n)
			n = (long) diff;
		error = uiomove(bp->b_data + on, n, uio);
		brelse(bp);
	} while (error == 0 && uio->uio_resid > 0 && n != 0);
	if (!isadir && (error == 0 || uio->uio_resid != orig_resid)
			&& (vp->v_mount->mnt_flag & MNT_NOATIME) == 0)
		dep->de_flag |= DE_ACCESS;
	return (error);
}

/*
 * Write data to a file or directory.
 */
int
msdosfs_write(ap)
	struct vop_write_args *ap;
{
	int n;
	int croffset;
	int resid;
	u_long osize;
	int error = 0;
	u_long count;
	daddr_t bn, lastcn;
	struct buf *bp;
	int ioflag = ap->a_ioflag;
	struct uio *uio = ap->a_uio;
	struct proc *p = uio->uio_procp;
	struct vnode *vp = ap->a_vp;
	struct vnode *thisvp;
	struct denode *dep = VTODE(vp);
	struct msdosfsmount *pmp = dep->de_pmp;
	struct ucred *cred = ap->a_cred;
	bool_t async;
	int extended=0;

#ifdef MSDOSFS_DEBUG
	printf("msdosfs_write(vp %08x, uio %08x, ioflag %08x, cred %08x\n",
	       vp, uio, ioflag, cred);
	printf("msdosfs_write(): diroff %d, dirclust %d, startcluster %d\n",
	       dep->de_diroffset, dep->de_dirclust, dep->de_StartCluster);
#endif

	switch (vp->v_type) {
	case VREG:
		if (ioflag & IO_APPEND)
			uio->uio_offset = dep->de_FileSize;
		thisvp = vp;
		break;
	case VDIR:
		return EISDIR;
	default:
		panic("msdosfs_write(): bad file type");
	}

	if (uio->uio_offset < 0)
		return (EINVAL);

	if (uio->uio_resid == 0)
		return (0);


	/* Don't bother to try to write files larger than the fs limit */
	if (uio->uio_offset + uio->uio_resid > MSDOSFS_FILESIZE_MAX)
		return (EFBIG);

	/*
	 * If they've exceeded their filesize limit, tell them about it.
	 */
	if (p && ((uio->uio_offset + uio->uio_resid) > p->p_rlimit[RLIMIT_FSIZE].rlim_cur)) {
		psignal(p, SIGXFSZ);
		return (EFBIG);
	}

	/*
	 * If the offset we are starting the write at is beyond the end of
	 * the file, then they've done a seek.  Unix filesystems allow
	 * files with holes in them, DOS doesn't so we must fill the hole
	 * with zeroed blocks.
	 */
	if (uio->uio_offset > dep->de_FileSize) {
		if ((error = deextend(dep, uio->uio_offset, cred)) != 0)
			return (error);
	}

	/*
	 * Remember some values in case the write fails.
	 */
	async = vp->v_mount->mnt_flag & MNT_ASYNC;
	resid = uio->uio_resid;
	osize = dep->de_FileSize;

	/*
	 * If we write beyond the end of the file, extend it to its ultimate
	 * size ahead of the time to hopefully get a contiguous area.
	 */
	if (uio->uio_offset + resid > osize) {
		count = de_clcount(pmp, uio->uio_offset + resid) -
			de_clcount(pmp, osize);
		if ((error = extendfile(dep, count, NULL, NULL, 0)) &&
		    (error != ENOSPC || (ioflag & IO_UNIT)))
			goto errexit;
	}
	if (dep->de_FileSize < uio->uio_offset + resid) {
		dep->de_FileSize = uio->uio_offset + resid;
		vnode_pager_setsize(vp, dep->de_FileSize);
		extended = 1;
	}

	do {
		if (de_cluster(pmp, uio->uio_offset) > lastcn) {
			error = ENOSPC;
			break;
		}

		bn = de_blk(pmp, uio->uio_offset);
		if ((uio->uio_offset & pmp->pm_crbomask) == 0
				&& (de_blk(pmp, uio->uio_offset + uio->uio_resid)
						> de_blk(pmp, uio->uio_offset)
						|| uio->uio_offset + uio->uio_resid >= dep->de_FileSize)) {
			/*
			 * If either the whole cluster gets written,
			 * or we write the cluster from its start beyond EOF,
			 * then no need to read data from disk.
			 */
			bp = getblk(thisvp, bn, pmp->pm_bpcluster, 0, 0);
			clrbuf(bp);
			/*
			 * Do the bmap now, since pcbmap needs buffers
			 * for the fat table. (see msdosfs_strategy)
			 */
			if (bp->b_blkno == bp->b_lblkno) {
				error = pcbmap(dep, de_bn2cn(pmp, bp->b_lblkno), &bp->b_blkno,
						0, 0);
				if (error)
					bp->b_blkno = -1;
			}
			if (bp->b_blkno == -1) {
				brelse(bp);
				if (!error)
					error = EIO; /* XXX */
				break;
			}
		} else {
			/*
			 * The block we need to write into exists, so read it in.
			 */
			error = bread(thisvp, bn, pmp->pm_bpcluster,
			NOCRED, &bp);
			if (error) {
				brelse(bp);
				break;
			}
		}

		croffset = uio->uio_offset & pmp->pm_crbomask;
		n = min(uio->uio_resid, pmp->pm_bpcluster - croffset);
		if (uio->uio_offset + n > dep->de_FileSize) {
			dep->de_FileSize = uio->uio_offset + n;
			vnode_pager_setsize(vp, dep->de_FileSize); /* why? */
		}
		(void) vnode_pager_uncache(vp); /* why not? */
		/*
		 * Should these vnode_pager_* functions be done on dir
		 * files?
		 */

		/*
		 * Copy the data from user space into the buf header.
		 */
		error = uiomove(bp->b_data + croffset, n, uio);

		/*
		 * If they want this synchronous then write it and wait for
		 * it.  Otherwise, if on a cluster boundary write it
		 * asynchronously so we can move on to the next block
		 * without delay.  Otherwise do a delayed write because we
		 * may want to write somemore into the block later.
		 */
		if (ioflag & IO_SYNC)
			(void) bwrite(bp);
		else if (n + croffset == pmp->pm_bpcluster)
			bawrite(bp);
		else
			bdwrite(bp);
		dep->de_flag |= DE_UPDATE;
	} while (error == 0 && uio->uio_resid > 0);

	/*
	 * If the write failed and they want us to, truncate the file back
	 * to the size it was before the write was attempted.
	 */
errexit:
	if (error) {
		if (ioflag & IO_UNIT) {
			detrunc(dep, osize, ioflag & IO_SYNC, NOCRED, NULL);
			uio->uio_offset -= resid - uio->uio_resid;
			uio->uio_resid = resid;
		} else {
			detrunc(dep, dep->de_FileSize, ioflag & IO_SYNC, NOCRED, NULL);
			if (uio->uio_resid != resid)
				error = 0;
		}
	} else if (ioflag & IO_SYNC)
		error = deupdat(dep, 1);
	return (error);
}

int
msdosfs_ioctl(ap)
	struct vop_ioctl_args *ap;
{

	return (ENOTTY);
}

int
msdosfs_select(ap)
	struct vop_select_args *ap;
{

	return (1);		/* DOS filesystems never block? */
}

int
msdosfs_poll(ap)
	struct vop_poll_args *ap;
{

	return (1);
}

int
msdosfs_mmap(ap)
	struct vop_mmap_args *ap;
{

	return (EINVAL);
}

/*
 * Flush the blocks of a file to disk.
 *
 * This function is worthless for vnodes that represent directories. Maybe we
 * could just do a sync if they try an fsync on a directory file.
 */
int
msdosfs_fsync(ap)
	struct vop_fsync_args *ap;
{
	struct vnode *vp = ap->a_vp;
	int s;
	struct buf *bp, *nbp;

	/*
	 * Flush all dirty buffers associated with a vnode.
	 */
loop:
	s = splbio();

	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if ((bp->b_flags & B_DELWRI) == 0)
			panic("msdosfs_fsync: not dirty");
		bremfree(bp);
		splx(s);
		/* XXX Could do bawrite */
		(void) bwrite(bp);
		goto loop;
	}
#ifdef DIAGNOSTIC
	if (!LIST_EMPTY(&vp->v_dirtyblkhd)) {
		vprint("msdosfs_fsync: dirty", vp);
		goto loop;
	}
#endif
	splx(s);
	//vflushbuf(vp, ap->a_waitfor == MNT_WAIT);
	return (deupdat(VTODE(vp), ap->a_waitfor == MNT_WAIT));
}

/*
 * Now the whole work of extending a file is done in the write function.
 * So nothing to do here.
 */
int
msdosfs_seek(ap)
	struct vop_seek_args *ap;
{
	return (0);
}

int
msdosfs_remove(ap)
	struct vop_remove_args *ap;
{
	struct denode *dep = VTODE(ap->a_vp);
	struct denode *ddep = VTODE(ap->a_dvp);
	int error;

	error = removede(ddep, dep);
#ifdef MSDOSFS_DEBUG
	printf("msdosfs_remove(), dep %08x, v_usecount %d\n", dep, ap->a_vp->v_usecount);
#endif
	if (ddep == dep)
		vrele(ap->a_vp);
	else
		vput(ap->a_vp);	/* causes msdosfs_inactive() to be called
				 * via vrele() */
	vput(ap->a_dvp);
	return (error);
}

/*
 * DOS filesystems don't know what links are. But since we already called
 * msdosfs_lookup() with create and lockparent, the parent is locked so we
 * have to free it before we return the error.
 */
int
msdosfs_link(ap)
	struct vop_link_args *ap;
{
	VOP_ABORTOP(ap->a_vp, ap->a_cnp);
	vput(ap->a_vp);
	return (EOPNOTSUPP);
}

/*
 * Renames on files require moving the denode to a new hash queue since the
 * denode's location is used to compute which hash queue to put the file
 * in. Unless it is a rename in place.  For example "mv a b".
 *
 * What follows is the basic algorithm:
 *
 * if (file move) {
 *	if (dest file exists) {
 *		remove dest file
 *	}
 *	if (dest and src in same directory) {
 *		rewrite name in existing directory slot
 *	} else {
 *		write new entry in dest directory
 *		update offset and dirclust in denode
 *		move denode to new hash chain
 *		clear old directory entry
 *	}
 * } else {
 *	directory move
 *	if (dest directory exists) {
 *		if (dest is not empty) {
 *			return ENOTEMPTY
 *		}
 *		remove dest directory
 *	}
 *	if (dest and src in same directory) {
 *		rewrite name in existing entry
 *	} else {
 *		be sure dest is not a child of src directory
 *		write entry in dest directory
 *		update "." and ".." in moved directory
 *		update offset and dirclust in denode
 *		move denode to new hash chain
 *		clear old directory entry for moved directory
 *	}
 * }
 *
 * On entry:
 *	source's parent directory is unlocked
 *	source file or directory is unlocked
 *	destination's parent directory is locked
 *	destination file or directory is locked if it exists
 *
 * On exit:
 *	all denodes should be released
 *
 * Notes:
 * I'm not sure how the memory containing the pathnames pointed at by the
 * componentname structures is freed, there may be some memory bleeding
 * for each rename done.
 */
int
msdosfs_rename(ap)
	struct vop_rename_args *ap;
{
	struct vnode *tvp = ap->a_tvp;
	register struct vnode *tdvp = ap->a_tdvp;
	struct vnode *fvp = ap->a_fvp;
	register struct vnode *fdvp = ap->a_fdvp;
	register struct componentname *tcnp = ap->a_tcnp;
	register struct componentname *fcnp = ap->a_fcnp;
	register struct denode *ip, *xp, *dp, *zp;
	u_char toname[11], oldname[11];
	u_long from_diroffset, to_diroffset;
	u_char to_count;
	int doingdirectory = 0, newparent = 0;
	int error;
	u_long cn;
	daddr_t bn;
	struct msdosfsmount *pmp;
	struct direntry *dotdotp;
	struct buf *bp;
	int fdvp_dorele = 0;

	pmp = VFSTOMSDOSFS(fdvp->v_mount);

#ifdef DIAGNOSTIC
	if ((tcnp->cn_flags & HASBUF) == 0 ||
	    (fcnp->cn_flags & HASBUF) == 0)
		panic("msdosfs_rename: no name");
#endif
	/*
	 * Check for cross-device rename.
	 */
	if ((fvp->v_mount != tdvp->v_mount) ||
	    (tvp && (fvp->v_mount != tvp->v_mount))) {
		error = EXDEV;
abortit:
		VOP_ABORTOP(tdvp, tcnp);
		if (tdvp == tvp)
			vrele(tdvp);
		else
			vput(tdvp);
		if (tvp)
			vput(tvp);
		VOP_ABORTOP(fdvp, fcnp);
		vrele(fdvp);
		vrele(fvp);
		return (error);
	}

	/*
	 * If source and dest are the same, do nothing.
	 */
	if (tvp == fvp) {
		error = 0;
		goto abortit;
	}

	/* */
	if ((error = vn_lock(fvp, LK_EXCLUSIVE, fvp->v_proc)) != 0)
		goto abortit;
	dp = VTODE(fdvp);
	ip = VTODE(fvp);

	/*
	 * Be sure we are not renaming ".", "..", or an alias of ".". This
	 * leads to a crippled directory tree.  It's pretty tough to do a
	 * "ls" or "pwd" with the "." directory entry missing, and "cd .."
	 * doesn't work if the ".." entry is missing.
	 */
	if (ip->de_Attributes & ATTR_DIRECTORY) {
		/*
		 * Avoid ".", "..", and aliases of "." for obvious reasons.
		 */
		if ((fcnp->cn_namelen == 1 && fcnp->cn_nameptr[0] == '.') ||
		    dp == ip ||
		    (fcnp->cn_flags & ISDOTDOT) ||
		    (tcnp->cn_flags & ISDOTDOT) ||
		    (ip->de_flag & DE_RENAME)) {
			VOP_UNLOCK(fvp, 0, fvp->v_proc);
			error = EINVAL;
			goto abortit;
		}
		ip->de_flag |= DE_RENAME;
		doingdirectory++;
	}

	/*
	 * When the target exists, both the directory
	 * and target vnodes are returned locked.
	 */
	dp = VTODE(tdvp);
	xp = tvp ? VTODE(tvp) : NULL;
	/*
	 * Remember direntry place to use for destination
	 */
	to_diroffset = dp->de_fndoffset;
	to_count = dp->de_fndcnt;

	/*
	 * If ".." must be changed (ie the directory gets a new
	 * parent) then the source directory must not be in the
	 * directory heirarchy above the target, as this would
	 * orphan everything below the source directory. Also
	 * the user must have write permission in the source so
	 * as to be able to change "..". We must repeat the call
	 * to namei, as the parent directory is unlocked by the
	 * call to doscheckpath().
	 */
	error = VOP_ACCESS(fvp, VWRITE, tcnp->cn_cred, tcnp->cn_proc);
	VOP_UNLOCK(fvp, 0, fvp->v_proc);
	if (VTODE(fdvp)->de_StartCluster != VTODE(tdvp)->de_StartCluster)
		newparent = 1;
	vrele(fdvp);
	if (doingdirectory && newparent) {
		if (error)	/* write access check above */
			goto bad;
		if (xp != NULL)
			vput(tvp);
		/*
		 * doscheckpath() vput()'s dp,
		 * so we have to do a relookup afterwards
		 */
		if ((error = doscheckpath(ip, dp)) != 0)
			goto out;
		if ((tcnp->cn_flags & SAVESTART) == 0)
			panic("msdosfs_rename: lost to startdir");
		if ((error = relookup(tdvp, &tvp, tcnp)) != 0)
			goto out;
		dp = VTODE(tdvp);
		xp = tvp ? VTODE(tvp) : NULL;
	}

	if (xp != NULL) {
		/*
		 * Target must be empty if a directory and have no links
		 * to it. Also, ensure source and target are compatible
		 * (both directories, or both not directories).
		 */
		if (xp->de_Attributes & ATTR_DIRECTORY) {
			if (!dosdirempty(xp)) {
				error = ENOTEMPTY;
				goto bad;
			}
			if (!doingdirectory) {
				error = ENOTDIR;
				goto bad;
			}
			cache_purge(tdvp);
		} else if (doingdirectory) {
			error = EISDIR;
			goto bad;
		}
		if ((error = removede(dp, xp)) != 0)
			goto bad;
		cache_purge(tvp);
		vput(tvp);
		xp = NULL;
	}

	/*
	 * Convert the filename in tcnp into a dos filename. We copy this
	 * into the denode and directory entry for the destination
	 * file/directory.
	 */
	if ((error = uniqdosname(VTODE(tdvp), tcnp, toname)) != 0)
		goto abortit;

	/*
	 * Since from wasn't locked at various places above,
	 * have to do a relookup here.
	 */
	fcnp->cn_flags &= ~MODMASK;
	fcnp->cn_flags |= LOCKPARENT | LOCKLEAF;
	if ((fcnp->cn_flags & SAVESTART) == 0)
		panic("msdosfs_rename: lost from startdir");
	if (!newparent)
		VOP_UNLOCK(tdvp, 0, tdvp->v_proc);
	(void) relookup(fdvp, &fvp, fcnp);
	if (fvp == NULL) {
		/*
		 * From name has disappeared.
		 */
		if (doingdirectory)
			panic("rename: lost dir entry");
		vrele(ap->a_fvp);
		if (newparent)
			VOP_UNLOCK(tdvp, 0, tdvp->v_proc);
		vrele(tdvp);
		return 0;
	}
	fdvp_dorele = 1;
	xp = VTODE(fvp);
	zp = VTODE(fdvp);
	from_diroffset = zp->de_fndoffset;

	/*
	 * Ensure that the directory entry still exists and has not
	 * changed till now. If the source is a file the entry may
	 * have been unlinked or renamed. In either case there is
	 * no further work to be done. If the source is a directory
	 * then it cannot have been rmdir'ed or renamed; this is
	 * prohibited by the DE_RENAME flag.
	 */
	if (xp != ip) {
		if (doingdirectory)
			panic("rename: lost dir entry");
		vrele(ap->a_fvp);
		VOP_UNLOCK(fvp, 0, fvp->v_proc);
		if (newparent)
			VOP_UNLOCK(fdvp, 0, fdvp->v_proc);
		xp = NULL;
	} else {
		vrele(fvp);
		xp = NULL;

		/*
		 * First write a new entry in the destination
		 * directory and mark the entry in the source directory
		 * as deleted.  Then move the denode to the correct hash
		 * chain for its new location in the filesystem.  And, if
		 * we moved a directory, then update its .. entry to point
		 * to the new parent directory.
		 */
		bcopy(ip->de_Name, oldname, 11);
		bcopy(toname, ip->de_Name, 11);	/* update denode */
		dp->de_fndoffset = to_diroffset;
		dp->de_fndcnt = to_count;
		error = createde(ip, dp, (struct denode **)0, tcnp);
		if (error) {
			bcopy(oldname, ip->de_Name, 11);
			if (newparent)
				VOP_UNLOCK(fdvp, 0, fdvp->v_proc);
			VOP_UNLOCK(fvp, 0, fvp->v_proc);
			goto bad;
		}
		ip->de_refcnt++;
		zp->de_fndoffset = from_diroffset;
		if ((error = removede(zp, ip)) != 0) {
			/* XXX should really panic here, fs is corrupt */
			if (newparent)
				VOP_UNLOCK(fdvp, 0, fdvp->v_proc);
			VOP_UNLOCK(fvp, 0, fvp->v_proc);
			goto bad;
		}
		cache_purge(fvp);
		if (!doingdirectory) {
			error = pcbmap(dp, de_cluster(pmp, to_diroffset), 0,
				       &ip->de_dirclust, 0);
			if (error) {
				/* XXX should really panic here, fs is corrupt */
				if (newparent)
					VOP_UNLOCK(fdvp, 0, fdvp->v_proc);
				VOP_UNLOCK(fvp, 0, fvp->v_proc);
				goto bad;
			}
			if (ip->de_dirclust != MSDOSFSROOT)
				ip->de_diroffset = to_diroffset & pmp->pm_crbomask;
		}
		reinsert(ip);
		if (newparent)
			VOP_UNLOCK(fdvp, 0, fdvp->v_proc);
	}

	/*
	 * If we moved a directory to a new parent directory, then we must
	 * fixup the ".." entry in the moved directory.
	 */
	if (doingdirectory && newparent) {
		cn = ip->de_StartCluster;
		if (cn == MSDOSFSROOT) {
			/* this should never happen */
			panic("msdosfs_rename: updating .. in root directory?\n");
		} else
			bn = cntobn(pmp, cn);
		error = bread(pmp->pm_devvp, bn, pmp->pm_bpcluster,
		NOCRED, &bp);
		if (error) {
			/* XXX should really panic here, fs is corrupt */
			brelse(bp);
			VOP_UNLOCK(fvp, 0, fvp->v_proc);
			goto bad;
		}
		dotdotp = (struct direntry*) bp->b_data + 1;
		putushort(dotdotp->deStartCluster, dp->de_StartCluster);
		if (FAT32(pmp)) {
			putushort(dotdotp->deHighClust, dp->de_StartCluster >> 16);
		}
		if ((error = bwrite(bp)) != 0) {
			/* XXX should really panic here, fs is corrupt */
			VOP_UNLOCK(fvp, 0, fvp->v_proc);
			goto bad;
		}
	}

	VOP_UNLOCK(fvp, 0, fvp->v_proc);
bad:
	if (xp)
		vput(tvp);
	vput(tdvp);
out:
	ip->de_flag &= ~DE_RENAME;
	if (fdvp_dorele)
		vrele(fdvp);
	vrele(fdvp);
	vrele(fvp);
	return (error);
}

static struct {
	struct direntry dot;
	struct direntry dotdot;
} dosdirtemplate = {
	{	".       ", "   ",			/* the . entry */
		ATTR_DIRECTORY,				/* file attribute */
		0,	 						/* reserved */
		0, { 0, 0 }, { 0, 0 },		/* create time & date */
		{ 0, 0 },					/* access date */
		{ 0, 0 },					/* high bits of start cluster */
		{ 210, 4 }, { 210, 4 },		/* modify time & date */
		{ 0, 0 },					/* startcluster */
		{ 0, 0, 0, 0 } 				/* filesize */
	},
	{	"..      ", "   ",			/* the .. entry */
		ATTR_DIRECTORY,				/* file attribute */
		0,	 						/* reserved */
		0, { 0, 0 }, { 0, 0 },		/* create time & date */
		{ 0, 0 },					/* access date */
		{ 0, 0 },					/* high bits of start cluster */
		{ 210, 4 }, { 210, 4 },		/* modify time & date */
		{ 0, 0 },					/* startcluster */
		{ 0, 0, 0, 0 }				/* filesize */
	}
};

int
msdosfs_mkdir(ap)
	struct vop_mkdir_args *ap;
{
	struct componentname *cnp = ap->a_cnp;
	struct denode ndirent;
	struct denode *dep;
	struct denode *pdep = VTODE(ap->a_dvp);
	int error;
	int bn;
	u_long newcluster, pcl;
	struct direntry *denp;
	struct msdosfsmount *pmp = pdep->de_pmp;
	struct buf *bp;
	struct timespec ts;
	int async = pdep->de_pmp->pm_mountp->mnt_flag & MNT_ASYNC;

	/*
	 * If this is the root directory and there is no space left we
	 * can't do anything.  This is because the root directory can not
	 * change size.
	 */
	if (pdep->de_StartCluster == MSDOSFSROOT
	    && pdep->de_fndoffset >= pdep->de_FileSize) {
		error = ENOSPC;
		goto bad2;
	}

	/*
	 * Allocate a cluster to hold the about to be created directory.
	 */
	error = clusteralloc(pmp, 0, 1, &newcluster, NULL);
	if (error)
		goto bad2;

	bzero(&ndirent, sizeof(ndirent));
	ndirent.de_pmp = pmp;
	ndirent.de_flag = DE_ACCESS | DE_CREATE | DE_UPDATE;
	TIMEVAL_TO_TIMESPEC(&time, &ts);
	DETIMES(&ndirent, &ts, &ts, &ts, pmp->pm_gmtoff);

	/*
	 * Now fill the cluster with the "." and ".." entries. And write
	 * the cluster to disk.  This way it is there for the parent
	 * directory to be pointing at if there were a crash.
	 */
	bn = cntobn(pmp, newcluster);
	/* always succeeds */
	bp = getblk(pmp->pm_devvp, bn, pmp->pm_bpcluster, 0, 0);
	bzero(bp->b_data, pmp->pm_bpcluster);
	bcopy(&dosdirtemplate, bp->b_data, sizeof dosdirtemplate);
	denp = (struct direntry *)bp->b_data;
	putushort(denp[0].deStartCluster, newcluster);
	putushort(denp[0].deCDate, ndirent.de_CDate);
	putushort(denp[0].deCTime, ndirent.de_CTime);
	denp[0].deCHundredth = ndirent.de_CHun;
	putushort(denp[0].deADate, ndirent.de_ADate);
	putushort(denp[0].deMDate, ndirent.de_MDate);
	putushort(denp[0].deMTime, ndirent.de_MTime);
	pcl = pdep->de_StartCluster;
	if (FAT32(pmp) && pcl == pmp->pm_rootdirblk)
		pcl = 0;
	putushort(denp[1].deStartCluster, pcl);
	putushort(denp[1].deCDate, ndirent.de_CDate);
	putushort(denp[1].deCTime, ndirent.de_CTime);
	denp[1].deCHundredth = ndirent.de_CHun;
	putushort(denp[1].deADate, ndirent.de_ADate);
	putushort(denp[1].deMDate, ndirent.de_MDate);
	putushort(denp[1].deMTime, ndirent.de_MTime);
	if (FAT32(pmp)) {
		putushort(denp[0].deHighClust, newcluster >> 16);
		putushort(denp[1].deHighClust, pdep->de_StartCluster >> 16);
	}

	if (async)
		bdwrite(bp);
	else if ((error = bwrite(bp)) != 0)
		goto bad;

	/*
	 * Now build up a directory entry pointing to the newly allocated
	 * cluster.  This will be written to an empty slot in the parent
	 * directory.
	 */
#ifdef DIAGNOSTIC
	if ((cnp->cn_flags & HASBUF) == 0)
		panic("msdosfs_mkdir: no name");
#endif
	if ((error = uniqdosname(pdep, cnp, ndirent.de_Name)) != 0)
		goto bad;

	ndirent.de_Attributes = ATTR_DIRECTORY;
	ndirent.de_StartCluster = newcluster;
	ndirent.de_FileSize = 0;
	ndirent.de_dev = pdep->de_dev;
	ndirent.de_devvp = pdep->de_devvp;
	if ((error = createde(&ndirent, pdep, &dep, cnp)) != 0)
		goto bad;
	if ((cnp->cn_flags & SAVESTART) == 0)
		FREE(cnp->cn_pnbuf, M_NAMEI);
	vput(ap->a_dvp);
	*ap->a_vpp = DETOV(dep);
	return (0);

bad:
	clusterfree(pmp, newcluster, NULL);
bad2:
	FREE(cnp->cn_pnbuf, M_NAMEI);
	vput(ap->a_dvp);
	return (error);
}

int
msdosfs_rmdir(ap)
	struct vop_rmdir_args *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct vnode *dvp = ap->a_dvp;
	register struct componentname *cnp = ap->a_cnp;
	register struct denode *ip, *dp;
	int error;

	ip = VTODE(vp);
	dp = VTODE(dvp);
	/*
	 * No rmdir "." please.
	 */
	if (dp == ip) {
		vrele(dvp);
		vput(vp);
		return (EINVAL);
	}
	/*
	 * Verify the directory is empty (and valid).
	 * (Rmdir ".." won't be valid since
	 *  ".." will contain a reference to
	 *  the current directory and thus be
	 *  non-empty.)
	 */
	error = 0;
	if (!dosdirempty(ip) || (ip->de_flag & DE_RENAME)) {
		error = ENOTEMPTY;
		goto out;
	}
	/*
	 * Delete the entry from the directory.  For dos filesystems this
	 * gets rid of the directory entry on disk, the in memory copy
	 * still exists but the de_refcnt is <= 0.  This prevents it from
	 * being found by deget().  When the vput() on dep is done we give
	 * up access and eventually msdosfs_reclaim() will be called which
	 * will remove it from the denode cache.
	 */
	if ((error = removede(dp, ip)) != 0)
		goto out;
	/*
	 * This is where we decrement the link count in the parent
	 * directory.  Since dos filesystems don't do this we just purge
	 * the name cache and let go of the parent directory denode.
	 */
	cache_purge(dvp);
	vput(dvp);
	dvp = NULL;
	/*
	 * Truncate the directory that is being deleted.
	 */
	error = detrunc(ip, (u_long)0, IO_SYNC, cnp->cn_cred, cnp->cn_proc);
	cache_purge(vp);
out:
	if (dvp)
		vput(dvp);
	vput(vp);
	return (error);
}

/*
 * DOS filesystems don't know what symlinks are.
 */
int
msdosfs_symlink(ap)
	struct vop_symlink_args *ap;
{
	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
	vput(ap->a_dvp);
	return (EOPNOTSUPP);
}

int
msdosfs_readdir(ap)
	struct vop_readdir_args *ap;
{
	int error = 0;
	int diff;
	long n;
	int blsize;
	long on;
	long lost;
	long count;
	u_long cn;
	u_long fileno;
	u_long dirsperblk;
	long bias = 0;
	daddr_t bn, lbn;
	struct buf *bp;
	struct denode *dep = VTODE(ap->a_vp);
	struct msdosfsmount *pmp = dep->de_pmp;
	struct direntry *dentp;
	struct dirent dirbuf;
	struct uio *uio = ap->a_uio;
	u_long *cookies = NULL;
	int ncookies = 0, nc = 0;
	off_t offset, off;
	int chksum = -1;

#ifdef MSDOSFS_DEBUG
	printf("msdosfs_readdir(): vp %08x, uio %08x, cred %08x, eofflagp %08x\n",
	       ap->a_vp, uio, ap->a_cred, ap->a_eofflag);
#endif

	/*
	 * msdosfs_readdir() won't operate properly on regular files since
	 * it does i/o only with the the filesystem vnode, and hence can
	 * retrieve the wrong block from the buffer cache for a plain file.
	 * So, fail attempts to readdir() on a plain file.
	 */
	if ((dep->de_Attributes & ATTR_DIRECTORY) == 0)
		return (ENOTDIR);

	/*
	 * To be safe, initialize dirbuf
	 */
	bzero(dirbuf.d_name, sizeof(dirbuf.d_name));

	/*
	 * If the user buffer is smaller than the size of one dos directory
	 * entry or the file offset is not a multiple of the size of a
	 * directory entry, then we fail the read.
	 */
	count = uio->uio_resid & ~(sizeof(struct direntry) - 1);
	offset = uio->uio_offset;
	if (count < sizeof(struct direntry) ||
	    (offset & (sizeof(struct direntry) - 1)))
		return (EINVAL);
	lost = uio->uio_resid - count;
	uio->uio_resid = count;
	off = uio->uio_offset;

	if (ap->a_ncookies) {
		nc = uio->uio_resid / 16;
		cookies = malloc(nc * sizeof (off_t), M_TEMP, M_WAITOK);
		*ap->a_cookies = cookies;
	}

	dirsperblk = pmp->pm_BytesPerSec / sizeof(struct direntry);

	/*
	 * If they are reading from the root directory then, we simulate
	 * the . and .. entries since these don't exist in the root
	 * directory.  We also set the offset bias to make up for having to
	 * simulate these entries. By this I mean that at file offset 64 we
	 * read the first entry in the root directory that lives on disk.
	 */
	if (dep->de_StartCluster == MSDOSFSROOT
	    || (FAT32(pmp) && dep->de_StartCluster == pmp->pm_rootdirblk)) {
#if 0
		printf("msdosfs_readdir(): going after . or .. in root dir, offset %d\n",
		    offset);
#endif
		bias = 2 * sizeof(struct direntry);
		if (offset < bias) {
			for (n = (int) offset / sizeof(struct direntry); n < 2; n++) {
				if (FAT32(pmp))
					dirbuf.d_fileno = cntobn(pmp,
							pmp->pm_rootdirblk) * dirsperblk;
				else
					dirbuf.d_fileno = 1;
				dirbuf.d_type = DT_DIR;
				switch (n) {
				case 0:
					dirbuf.d_namlen = 1;
					strlcpy(dirbuf.d_name, ".", sizeof(dirbuf.d_name));
					break;
				case 1:
					dirbuf.d_namlen = 2;
					strlcpy(dirbuf.d_name, "..", sizeof(dirbuf.d_name));
					break;
				}
				dirbuf.d_reclen = GENERIC_DIRSIZ(&dirbuf);
				if (uio->uio_resid < dirbuf.d_reclen)
					goto out;
				error = uiomove((caddr_t) & dirbuf, dirbuf.d_reclen, uio);
				if (error)
					goto out;
				offset += sizeof(struct direntry);
				off = offset;
				if (cookies) {
					*cookies++ = offset;
					ncookies++;
					if (ncookies >= nc)
						goto out;
				}
			}
		}
	}

	while (uio->uio_resid > 0) {
		lbn = de_cluster(pmp, offset - bias);
		on = (offset - bias) & pmp->pm_crbomask;
		n = MIN(pmp->pm_bpcluster - on, uio->uio_resid);
		diff = dep->de_FileSize - (offset - bias);
		if (diff <= 0)
			break;
		n = MIN(n, diff);
		if ((error = pcbmap(dep, lbn, &bn, &cn, &blsize)) != 0)
			break;
		error = bread(pmp->pm_devvp, bn, blsize, NOCRED, &bp);
		if (error) {
			brelse(bp);
			return (error);
		}
		n = MIN(n, blsize - bp->b_resid);

		/*
		 * Convert from dos directory entries to fs-independent
		 * directory entries.
		 */
		for (dentp = (struct direntry*) (bp->b_data + on);
				(char*) dentp < bp->b_data + on + n; dentp++, offset +=
						sizeof(struct direntry)) {
#if 0

			printf("rd: dentp %08x prev %08x crnt %08x deName %02x attr %02x\n",
			    dentp, prev, crnt, dentp->deName[0], dentp->deAttributes);
#endif
			/*
			 * If this is an unused entry, we can stop.
			 */
			if (dentp->deName[0] == SLOT_EMPTY) {
				brelse(bp);
				goto out;
			}
			/*
			 * Skip deleted entries.
			 */
			if (dentp->deName[0] == SLOT_DELETED) {
				chksum = -1;
				continue;
			}

			/*
			 * Handle Win95 long directory entries
			 */
			if (dentp->deAttributes == ATTR_WIN95) {
				if (pmp->pm_flags & MSDOSFSMNT_SHORTNAME)
					continue;
				chksum = win2unixfn((struct winentry*) dentp, &dirbuf, chksum);
				continue;
			}

			/*
			 * Skip volume labels
			 */
			if (dentp->deAttributes & ATTR_VOLUME) {
				chksum = -1;
				continue;
			}
			/*
			 * This computation of d_fileno must match
			 * the computation of va_fileid in
			 * msdosfs_getattr.
			 */
			if (dentp->deAttributes & ATTR_DIRECTORY) {
				fileno = getushort(dentp->deStartCluster);
				if (FAT32(pmp))
					fileno |= getushort(dentp->deHighClust) << 16;
				/* if this is the root directory */
				if (fileno == MSDOSFSROOT)
					if (FAT32(pmp))
						fileno = cntobn(pmp,
								pmp->pm_rootdirblk) * dirsperblk;
					else
						fileno = 1;
				else
					fileno = cntobn(pmp, fileno) * dirsperblk;
				dirbuf.d_fileno = fileno;
				dirbuf.d_type = DT_DIR;
			} else {
				dirbuf.d_fileno = offset / sizeof(struct direntry);
				dirbuf.d_type = DT_REG;
			}
			if (chksum != winChksum(dentp->deName))
				dirbuf.d_namlen = dos2unixfn(dentp->deName,
						(u_char*) dirbuf.d_name,
						pmp->pm_flags & MSDOSFSMNT_SHORTNAME);
			else
				dirbuf.d_name[dirbuf.d_namlen] = 0;
			chksum = -1;
			dirbuf.d_reclen = GENERIC_DIRSIZ(&dirbuf);
			if (uio->uio_resid < dirbuf.d_reclen) {
				brelse(bp);
				goto out;
			}
			error = uiomove((caddr_t) & dirbuf, dirbuf.d_reclen, uio);
			if (error) {
				brelse(bp);
				goto out;
			}
			off = offset + sizeof(struct direntry);
			if (cookies) {
				*cookies++ = offset + sizeof(struct direntry);
				ncookies++;
				if (ncookies >= nc) {
					brelse(bp);
					goto out;
				}
			}
		}
		brelse(bp);
	}

out:
 uio->uio_offset = off;
	uio->uio_resid += lost;
	if (dep->de_FileSize - (offset - bias) <= 0)
		*ap->a_eofflag = 1;
	else
		*ap->a_eofflag = 0;

	if (ap->a_ncookies) {
		if (error) {
			free(*ap->a_cookies, M_TEMP);
			*ap->a_ncookies = 0;
			*ap->a_cookies = NULL;
		} else
			*ap->a_ncookies = ncookies;
	}
	return (error);
}

/*
 * DOS filesystems don't know what symlinks are.
 */
int
msdosfs_readlink(ap)
	struct vop_readlink_args *ap;
{

	return (EINVAL);
}

int
msdosfs_abortop(ap)
	struct vop_abortop_args *ap;
{
	if ((ap->a_cnp->cn_flags & (HASBUF | SAVESTART)) == HASBUF)
		FREE(ap->a_cnp->cn_pnbuf, M_NAMEI);
	return (0);
}

int
msdosfs_lock(ap)
	struct vop_lock_args *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct denode *dep;
#ifdef DIAGNOSTIC
	struct proc *p = curproc;	/* XXX */
#endif

	return (vop_nolock(ap));
}

int
msdosfs_unlock(ap)
	struct vop_unlock_args *ap;
{
	register struct denode *dep = VTODE(ap->a_vp);
#ifdef DIAGNOSTIC
	struct proc *p = curproc;	/* XXX */
#endif

	return (vop_nounlock(ap));
}

int
msdosfs_islocked(ap)
	struct vop_islocked_args *ap;
{
	return (vop_noislocked(ap));
}

/*
 * vp  - address of vnode file the file
 * bn  - which cluster we are interested in mapping to a filesystem block number.
 * vpp - returns the vnode for the block special file holding the filesystem
 *	 containing the file of interest
 * bnp - address of where to return the filesystem relative block number
 */
int
msdosfs_bmap(ap)
	struct vop_bmap_args *ap;
{
	struct denode *dep = VTODE(ap->a_vp);
	struct msdosfsmount *pmp = dep->de_pmp;

	if (ap->a_vpp != NULL)
		*ap->a_vpp = dep->de_devvp;
	if (ap->a_bnp == NULL)
		return (0);
	if (ap->a_runp) {
		/*
		 * Sequential clusters should be counted here.
		 */
		*ap->a_runp = 0;
	}

	return (pcbmap(dep, de_bn2cn(pmp, ap->a_bn), ap->a_bnp, 0, 0));
}

int
msdosfs_reallocblks(ap)
	struct vop_reallocblks_args *ap;
{
	/* Currently no support for clustering */		/* XXX */
	return (ENOSPC);
}

int
msdosfs_strategy(ap)
	struct vop_strategy_args *ap;
{
	struct buf *bp = ap->a_bp;
	struct denode *dep = VTODE(bp->b_vp);
	struct vnode *vp;
	int error = 0;

	if (bp->b_vp->v_type == VBLK || bp->b_vp->v_type == VCHR)
		panic("msdosfs_strategy: spec");
	/*
	 * If we don't already know the filesystem relative block number
	 * then get it using pcbmap().  If pcbmap() returns the block
	 * number as -1 then we've got a hole in the file.  DOS filesystems
	 * don't allow files with holes, so we shouldn't ever see this.
	 */
	if (bp->b_blkno == bp->b_lblkno) {
		error = pcbmap(dep, de_bn2cn(dep->de_pmp, bp->b_lblkno), &bp->b_blkno,
				0, 0);
		if (error)
			bp->b_blkno = -1;
		if (bp->b_blkno == -1)
			clrbuf(bp);
	}
	if (bp->b_blkno == -1) {
		biodone(bp);
		return (error);
	}
#ifdef DIAGNOSTIC
#endif
	/*
	 * Read/write the block from/to the disk that contains the desired
	 * file block.
	 */
	vp = dep->de_devvp;
	bp->b_dev = vp->v_rdev;
	VOP_STRATEGY(bp);
	return (0);
}

int
msdosfs_print(ap)
	struct vop_print_args *ap;
{
	struct denode *dep = VTODE(ap->a_vp);

	printf("tag VT_MSDOSFS, startcluster %d, dircluster %ld, diroffset %ld ",
	    dep->de_StartCluster, dep->de_dirclust, dep->de_diroffset);
	printf("on dev (%d, %d)\n", major(dep->de_dev), minor(dep->de_dev));
	return (0);
}

int
msdosfs_advlock(ap)
	struct vop_advlock_args *ap;
{
	register struct denode *dep = VTODE(ap->a_vp);

	return (lf_advlock(ap, &dep->de_lockf, dep->de_FileSize));
}

int
msdosfs_pathconf(ap)
	struct vop_pathconf_args *ap;
{
	struct msdosfsmount *pmp = VTODE(ap->a_vp)->de_pmp;

	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = 1;
		return (0);
	case _PC_NAME_MAX:
		*ap->a_retval = pmp->pm_flags & MSDOSFSMNT_LONGNAME ? WIN_MAXLEN : 12;
		return (0);
	case _PC_PATH_MAX:
		*ap->a_retval = PATH_MAX;
		return (0);
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return (0);
	case _PC_NO_TRUNC:
		*ap->a_retval = 0;
		return (0);
	default:
		return (EINVAL);
	}
	/* NOTREACHED */
}

/* Global vfs data structures for msdosfs */
#define	msdosfs_lease_check 	((int (*) (struct  vop_lease_args *))nullop)
#define	msdosfs_revoke 			((int (*) (struct  vop_revoke_args *))vop_norevoke)
#define msdosfs_blkatoff 		((int (*) (struct  vop_blkatoff_args *))eopnotsupp)
#define msdosfs_vfree 			((int (*) (struct  vop_vfree_args *))eopnotsupp)
#define msdosfs_valloc 			((int (*) (struct  vop_valloc_args *))eopnotsupp)
#define msdosfs_truncate 		((int (*) (struct  vop_truncate_args *))eopnotsupp)
#define msdosfs_update 			((int (*) (struct  vop_update_args *))eopnotsupp)

struct vnodeops msdosfs_vnodeops = {
		.vop_default = vop_default_error,	/* default */
		.vop_lookup = msdosfs_lookup,		/* lookup */
		.vop_create = msdosfs_create,		/* create */
		.vop_mknod = msdosfs_mknod,			/* mknod */
		.vop_open = msdosfs_open,			/* open */
		.vop_close = msdosfs_close,			/* close */
		.vop_access = msdosfs_access,		/* access */
		.vop_getattr = msdosfs_getattr,		/* getattr */
		.vop_setattr = msdosfs_setattr,		/* setattr */
		.vop_read = msdosfs_read,			/* read */
		.vop_write = msdosfs_write,			/* write */
		.vop_lease = msdosfs_lease_check,	/* lease */
		.vop_ioctl = msdosfs_ioctl,			/* ioctl */
		.vop_select = msdosfs_select,		/* select */
		.vop_select = msdosfs_poll,			/* poll */
		.vop_revoke = msdosfs_revoke,		/* revoke */
		.vop_mmap = msdosfs_mmap,			/* mmap */
		.vop_fsync = msdosfs_fsync,			/* fsync */
		.vop_seek = msdosfs_seek,			/* seek */
		.vop_remove = msdosfs_remove,		/* remove */
		.vop_link = msdosfs_link,			/* link */
		.vop_rename = msdosfs_rename,		/* rename */
		.vop_mkdir = msdosfs_mkdir,			/* mkdir */
		.vop_rmdir = msdosfs_rmdir,			/* rmdir */
		.vop_symlink = msdosfs_symlink,		/* symlink */
		.vop_readdir = msdosfs_readdir,		/* readdir */
		.vop_readlink = msdosfs_readlink,	/* readlink */
		.vop_abortop = msdosfs_abortop,		/* abortop */
		.vop_inactive = msdosfs_inactive,	/* inactive */
		.vop_reclaim = msdosfs_reclaim,		/* reclaim */
		.vop_lock = msdosfs_lock,			/* lock */
		.vop_unlock = msdosfs_unlock,		/* unlock */
		.vop_bmap = msdosfs_bmap,			/* bmap */
		.vop_strategy = msdosfs_strategy,	/* strategy */
		.vop_print = msdosfs_print,			/* print */
		.vop_islocked = msdosfs_islocked,	/* islocked */
		.vop_pathconf = msdosfs_pathconf,	/* pathconf */
		.vop_advlock = msdosfs_advlock,		/* advlock */
		.vop_blkatoff = msdosfs_blkatoff,	/* blkatoff */
		.vop_valloc = msdosfs_valloc,		/* valloc */
		.vop_vfree = msdosfs_vfree,			/* vfree */
		.vop_truncate = msdosfs_truncate,	/* truncate */
		.vop_update = msdosfs_update,		/* update */
		.vop_bwrite = vn_bwrite,			/* bwrite */
};
