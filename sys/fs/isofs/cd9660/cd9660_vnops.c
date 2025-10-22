/*-
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
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
 *	@(#)cd9660_vnops.c	8.19 (Berkeley) 5/27/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/dirent.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <sys/dir.h>

#include <miscfs/specfs/specdev.h>
#include <miscfs/fifofs/fifo.h>

#include <fs/isofs/cd9660/iso.h>
#include <fs/isofs/cd9660/cd9660_extern.h>
#include <fs/isofs/cd9660/cd9660_node.h>
#include <fs/isofs/cd9660/iso_rrip.h>
#include <fs/isofs/cd9660/cd9660_mount.h>

#if 0
/*
 * Mknod vnode call
 *  Actually remap the device number
 */
int
cd9660_mknod(ndp, vap, cred, p)
	struct nameidata *ndp;
	struct ucred *cred;
	struct vattr *vap;
	struct proc *p;
{
#ifndef	ISODEVMAP
	free(ndp->ni_pnbuf, M_NAMEI);
	vput(ndp->ni_dvp);
	vput(ndp->ni_vp);
	return (EINVAL);
#else
	register struct vnode *vp;
	struct iso_node *ip;
	struct iso_dnode *dp;
	int error;

	vp = ndp->ni_vp;
	ip = VTOI(vp);

	if (ip->i_mnt->iso_ftype != ISO_FTYPE_RRIP
	    || vap->va_type != vp->v_type
	    || (vap->va_type != VCHR && vap->va_type != VBLK)) {
		free(ndp->ni_pnbuf, M_NAMEI);
		vput(ndp->ni_dvp);
		vput(ndp->ni_vp);
		return (EINVAL);
	}

	dp = iso_dmap(ip->i_dev,ip->i_number,1);
	if (ip->inode.iso_rdev == vap->va_rdev || vap->va_rdev == VNOVAL) {
		/* same as the unmapped one, delete the mapping */
		remque(dp);
		FREE(dp,M_CACHE);
	} else
		/* enter new mapping */
		dp->d_dev = vap->va_rdev;

	/*
	 * Remove inode so that it will be reloaded by iget and
	 * checked to see if it is an alias of an existing entry
	 * in the inode cache.
	 */
	vput(vp);
	vp->v_type = VNON;
	vgone(vp);
	return (0);
#endif
}
#endif

/*
 * Setattr call. Only allowed for block and character special devices.
 */
int
cd9660_setattr(ap)
	struct vop_setattr_args /* {
		struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct vattr *vap = ap->a_vap;

	if (vap->va_type != VNON
			|| vap->va_nlink != (nlink_t)VNOVAL
			|| vap->va_fsid != VNOVAL
			|| vap->va_fileid != VNOVAL
			|| vap->va_blocksize != VNOVAL
			|| vap->va_rdev != (dev_t)VNOVAL
			|| (int)vap->va_bytes != VNOVAL
			|| vap->va_gen != VNOVAL
			|| vap->va_flags != VNOVAL
			|| vap->va_uid != (uid_t)VNOVAL
			|| vap->va_gid != (gid_t)VNOVAL
			|| vap->va_atime.tv_sec != VNOVAL
			|| vap->va_mtime.tv_sec != VNOVAL
			|| vap->va_mode != (mode_t)VNOVAL) {
		return EOPNOTSUPP;
	}
	if (vap->va_size != VNOVAL) {
		switch (vp->v_type) {
		case VDIR:
			return (EISDIR);
		case VLNK:
		case VREG:
			return (EROFS);
		case VCHR:
		case VBLK:
		case VSOCK:
		case VFIFO:
			return (0);
		}
	}
	return VOP_TRUNCATE(vp, vap->va_size, 0, ap->a_cred, ap->a_p);
}

/*
 * Open called.
 *
 * Nothing to do.
 */
/* ARGSUSED */
int
cd9660_open(ap)
	struct vop_open_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	return (0);
}

/*
 * Close called
 *
 * Update the times on the inode on writeable file systems.
 */
/* ARGSUSED */
int
cd9660_close(ap)
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	return (0);
}

/*
 * Check mode permission on inode pointer. Mode is READ, WRITE or EXEC.
 * The mode is shifted to select the owner/group/other fields. The
 * super user is granted all permissions.
 */
/* ARGSUSED */
int
cd9660_access(ap)
	struct vop_access_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct iso_node *ip = VTOI(vp);
	struct ucred *cred = ap->a_cred;
	mode_t mask, mode = ap->a_mode;
	gid_t *gp;
	int i;

	/*
	 * Disallow write attempts unless the file is a socket,
	 * fifo, or a block or character device resident on the
	 * file system.
	 */
	if (mode & VWRITE) {
		switch (vp->v_type) {
		case VDIR:
		case VLNK:
		case VREG:
			return (EROFS);
		}
	}

	/* User id 0 always gets access. */
	if (cred->cr_uid == 0)
		return (0);

	mask = 0;

	/* Otherwise, check the owner. */
	if (cred->cr_uid == ip->inode.iso_uid) {
		if (mode & VEXEC)
			mask |= S_IXUSR;
		if (mode & VREAD)
			mask |= S_IRUSR;
		if (mode & VWRITE)
			mask |= S_IWUSR;
		return ((ip->inode.iso_mode & mask) == mask ? 0 : EACCES);
	}

	/* Otherwise, check the groups. */
	for (i = 0, gp = cred->cr_groups; i < cred->cr_ngroups; i++, gp++)
		if (ip->inode.iso_gid == *gp) {
			if (mode & VEXEC)
				mask |= S_IXGRP;
			if (mode & VREAD)
				mask |= S_IRGRP;
			if (mode & VWRITE)
				mask |= S_IWGRP;
			return ((ip->inode.iso_mode & mask) == mask ?
			    0 : EACCES);
		}

	/* Otherwise, check everyone else. */
	if (mode & VEXEC)
		mask |= S_IXOTH;
	if (mode & VREAD)
		mask |= S_IROTH;
	if (mode & VWRITE)
		mask |= S_IWOTH;
	return ((ip->inode.iso_mode & mask) == mask ? 0 : EACCES);


}

int
cd9660_getattr(ap)
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;

{
	struct vnode *vp = ap->a_vp;
	register struct vattr *vap = ap->a_vap;
	register struct iso_node *ip = VTOI(vp);
	int i;

	vap->va_fsid = ip->i_dev;
	vap->va_fileid = ip->i_number;

	vap->va_mode = ip->inode.iso_mode;
	vap->va_nlink = ip->inode.iso_links;
	vap->va_uid = ip->inode.iso_uid;
	vap->va_gid = ip->inode.iso_gid;
	vap->va_atime = ip->inode.iso_atime;
	vap->va_mtime = ip->inode.iso_mtime;
	vap->va_ctime = ip->inode.iso_ctime;
	vap->va_rdev = ip->inode.iso_rdev;

	vap->va_size = (u_quad_t) ip->i_size;
	if (ip->i_size == 0 && (vap->va_mode & S_IFMT) == S_IFLNK) {
		struct vop_readlink_args rdlnk;
		struct iovec aiov;
		struct uio auio;
		char *cp;

		MALLOC(cp, char *, MAXPATHLEN, M_TEMP, M_WAITOK);
		aiov.iov_base = cp;
		aiov.iov_len = MAXPATHLEN;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_rw = UIO_READ;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_procp = ap->a_p;
		auio.uio_resid = MAXPATHLEN;
		rdlnk.a_uio = &auio;
		rdlnk.a_vp = ap->a_vp;
		rdlnk.a_cred = ap->a_cred;
		if (cd9660_readlink(&rdlnk) == 0)
			vap->va_size = MAXPATHLEN - auio.uio_resid;
		FREE(cp, M_TEMP);
	}
	vap->va_flags = 0;
	vap->va_gen = 1;
	vap->va_blocksize = ip->i_mnt->logical_block_size;
	vap->va_bytes = (u_quad_t) ip->i_size;
	vap->va_type = vp->v_type;
	return (0);
}

#if ISO_DEFAULT_BLOCK_SIZE >= NBPG
#ifdef DEBUG
extern int doclusterread;
#else
#define doclusterread 1
#endif
#else
/* XXX until cluster routines can handle block sizes less than one page */
#define doclusterread 0
#endif

/*
 * Vnode op for reading.
 */
int
cd9660_read(ap)
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	register struct uio *uio = ap->a_uio;
	register struct iso_node *ip = VTOI(vp);
	register struct iso_mnt *imp;
	struct buf *bp;
	daddr_t lbn, bn, rablock;
	off_t diff;
	int rasize, error = 0;
	long size, n, on;

	if (uio->uio_resid == 0)
		return (0);
	if (uio->uio_offset < 0)
		return (EINVAL);
	ip->i_flag |= IN_ACCESS;
	imp = ip->i_mnt;
	do {
		lbn = lblkno(imp, uio->uio_offset);
		on = blkoff(imp, uio->uio_offset);
		n = MIN((u_int) (imp->logical_block_size - on), uio->uio_resid);
		diff = (off_t) ip->i_size - uio->uio_offset;
		if (diff <= 0)
			return (0);
		if (diff < n)
			n = diff;
		size = blksize(imp, ip, lbn);
		rablock = lbn + 1;
		if (doclusterread) {
			if (lblktosize(imp, rablock) <= ip->i_size)
				error = cluster_read(vp, (off_t) ip->i_size, lbn, size, NOCRED,
						&bp);
			else
				error = bread(vp, lbn, size, NOCRED, &bp);
		} else {
			if (vp->v_lastr + 1 == lbn &&
			lblktosize(imp, rablock) < ip->i_size) {
				rasize = blksize(imp, ip, rablock);
				error = breadn(vp, lbn, size, &rablock, &rasize, 1, NOCRED,
						&bp);
			} else
				error = bread(vp, lbn, size, NOCRED, &bp);
		}
		vp->v_lastr = lbn;
		n = min(n, size - bp->b_resid);
		if (error) {
			brelse(bp);
			return (error);
		}

		error = uiomove(bp->b_data + on, (int)n, uio);
		if (n + on == imp->logical_block_size
				|| uio->uio_offset == (off_t) ip->i_size)
			bp->b_flags |= B_AGE;
		brelse(bp);
	} while (error == 0 && uio->uio_resid > 0 && n != 0);
	return (error);
}

/* ARGSUSED */
int
cd9660_ioctl(ap)
	struct vop_ioctl_args /* {
		struct vnode *a_vp;
		u_long a_command;
		caddr_t  a_data;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct iso_node *ip = VTOI(vp);

	switch (ap->a_command) {

	case FIOGETLBA:
		*(int*) (ap->a_data) = ip->iso_start;
		return 0;
	default:
		return (ENOTTY);
	}
}

/* ARGSUSED */
int
cd9660_select(ap)
	struct vop_select_args /* {
		struct vnode *a_vp;
		int  a_which;
		int  a_fflags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{

	/*
	 * We should really check to see if I/O is possible.
	 */
	return (1);
}

int
cd9660_poll(ap)
	struct vop_poll_args /* {
		struct vnode 	*a_vp;
		int 			a_fflags;
		int 			a_events;
		struct proc 	*a_p;
	} */ *ap;
{

	return (1);
}

/*
 * Mmap a file
 *
 * NB Currently unsupported.
 */
/* ARGSUSED */
int
cd9660_mmap(ap)
	struct vop_mmap_args /* {
		struct vnode *a_vp;
		int  a_fflags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	return (EINVAL);
}

/*
 * Seek on a file
 *
 * Nothing to do, so just return.
 */
/* ARGSUSED */
int
cd9660_seek(ap)
	struct vop_seek_args /* {
		struct vnode *a_vp;
		off_t  a_oldoff;
		off_t  a_newoff;
		struct ucred *a_cred;
	} */ *ap;
{

	return (0);
}

/*
 * Structure for reading directories
 */
struct isoreaddir {
	struct direct 	saveent;
	struct direct 	assocent;
	struct direct 	current;
	off_t 			saveoff;
	off_t 			assocoff;
	off_t 			curroff;
	struct uio 		*uio;
	off_t 			uio_off;
	int 			eofflag;
	u_long 			*cookies;
	int 			ncookies;
};

int
iso_uiodir(idp,dp,off)
	struct isoreaddir *idp;
	struct dirent *dp;
	off_t off;
{
	int error;

	dp->d_name[dp->d_namlen] = 0;
	dp->d_reclen = GENERIC_DIRSIZ(dp);

	if (idp->uio->uio_resid < dp->d_reclen) {
		idp->eofflag = 0;
		return (-1);
	}

	if (idp->cookies) {
		if (idp->ncookies <= 0) {
			idp->eofflag = 0;
			return (-1);
		}

		*idp->cookies++ = off;
		--idp->ncookies;
	}

	if ((error = uiomove((caddr_t)dp, dp->d_reclen, idp->uio)))
		return (error);
	idp->uio_off = off;
	return (0);
}

int
iso_shipdir(idp)
	struct isoreaddir *idp;
{
	struct dirent *dp;
	int cl, sl, assoc;
	int error;
	char *cname, *sname;

	cl = idp->current.d_namlen;
	cname = idp->current.d_name;
	if (assoc == cl > 1 && *cname == ASSOCCHAR) {
		cl--;
		cname++;
	}

	dp = &idp->saveent;
	sname = dp->d_name;
	if (!(sl = dp->d_namlen)) {
		dp = &idp->assocent;
		sname = dp->d_name + 1;
		sl = dp->d_namlen - 1;
	}
	if (sl > 0) {
		if (sl != cl || bcmp(sname, cname, sl)) {
			if (idp->assocent.d_namlen) {
				if ((error = iso_uiodir(idp, &idp->assocent, idp->assocoff)))
					return (error);
				idp->assocent.d_namlen = 0;
			}
			if (idp->saveent.d_namlen) {
				if ((error = iso_uiodir(idp, &idp->saveent, idp->saveoff)))
					return (error);
				idp->saveent.d_namlen = 0;
			}
		}
	}
	idp->current.d_reclen = GENERIC_DIRSIZ(&idp->current);
	if (assoc) {
		idp->assocoff = idp->curroff;
		bcopy(&idp->current, &idp->assocent, idp->current.d_reclen);
	} else {
		idp->saveoff = idp->curroff;
		bcopy(&idp->current, &idp->saveent, idp->current.d_reclen);
	}
	return (0);
}

/*
 * Vnode op for readdir
 */
int
cd9660_readdir(ap)
	struct vop_readdir_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
		int *a_eofflag;
		int *a_ncookies;
		u_long *a_cookies;
	} */ *ap;
{
	register struct uio *uio = ap->a_uio;
	struct isoreaddir *idp;
	struct vnode *vdp = ap->a_vp;
	struct iso_node *dp;
	struct iso_mnt *imp;
	struct buf *bp = NULL;
	struct iso_directory_record *ep;
	int entryoffsetinblock;
	doff_t endsearch;
	u_long bmask;
	int error = 0;
	int reclen;
	u_short namelen;
	int ncookies = 0;
	u_long *cookies = NULL;

	dp = VTOI(vdp);
	imp = dp->i_mnt;
	bmask = imp->im_bmask;

	MALLOC(idp, struct isoreaddir *, sizeof(*idp), M_TEMP, M_WAITOK);
	idp->saveent.d_namlen = idp->assocent.d_namlen = 0;
	/*
	 * XXX
	 * Is it worth trying to figure out the type?
	 */
	idp->saveent.d_type = idp->assocent.d_type = idp->current.d_type =
	DT_UNKNOWN;
	idp->uio = uio;
	if (ap->a_ncookies == NULL) {
		idp->cookies = NULL;
	} else {
		/*
		 * Guess the number of cookies needed.
		 */
		ncookies = uio->uio_resid / 16;
		cookies = malloc(ncookies * sizeof(*cookies), M_TEMP, M_WAITOK);
		idp->cookies = cookies;
		idp->ncookies = ncookies;
	}
	idp->eofflag = 1;
	idp->curroff = uio->uio_offset;

	if ((entryoffsetinblock = idp->curroff & bmask)
			&& (error = VOP_BLKATOFF(vdp, (off_t )idp->curroff, NULL, &bp))) {
		FREE(idp, M_TEMP);
		return (error);
	}
	endsearch = dp->i_size;

	while (idp->curroff < endsearch) {
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 */
		if ((idp->curroff & bmask) == 0) {
			if (bp != NULL)
				brelse(bp);
			if ((error = VOP_BLKATOFF(vdp, (off_t )idp->curroff, NULL, &bp)))
				break;
			entryoffsetinblock = 0;
		}
		/*
		 * Get pointer to next entry.
		 */
		ep = (struct iso_directory_record*) ((char*) bp->b_data
				+ entryoffsetinblock);

		reclen = isonum_711(ep->length);
		if (reclen == 0) {
			/* skip to next block, if any */
			idp->curroff = (idp->curroff & ~bmask) + imp->logical_block_size;
			continue;
		}

		if (reclen < ISO_DIRECTORY_RECORD_SIZE) {
			error = EINVAL;
			/* illegal entry, stop */
			break;
		}

		if (entryoffsetinblock + reclen > imp->logical_block_size) {
			error = EINVAL;
			/* illegal directory, so stop looking */
			break;
		}

		idp->current.d_namlen = isonum_711(ep->name_len);

		if (reclen < ISO_DIRECTORY_RECORD_SIZE + idp->current.d_namlen) {
			error = EINVAL;
			/* illegal entry, stop */
			break;
		}

		if (isonum_711(ep->flags) & 2)
			idp->current.d_fileno = isodirino(ep, imp);
		else
			idp->current.d_fileno = dbtob(bp->b_blkno) + entryoffsetinblock;

		idp->curroff += reclen;

		switch (imp->iso_ftype) {
		case ISO_FTYPE_RRIP:
			cd9660_rrip_getname(ep, idp->current.d_name, &namelen,
					&idp->current.d_fileno, imp);
			idp->current.d_namlen = (u_char) namelen;
			if (idp->current.d_namlen)
				error = iso_uiodir(idp, &idp->current, idp->curroff);
			break;
		default: /* ISO_FTYPE_DEFAULT || ISO_FTYPE_9660 */
			strcpy(idp->current.d_name, "..");
			switch (ep->name[0]) {
			case 0:
				idp->current.d_namlen = 1;
				error = iso_uiodir(idp, &idp->current, idp->curroff);
				break;
			case 1:
				idp->current.d_namlen = 2;
				error = iso_uiodir(idp, &idp->current, idp->curroff);
				break;
			default:
				isofntrans(ep->name, idp->current.d_namlen, idp->current.d_name,
						&namelen, imp->iso_ftype == ISO_FTYPE_9660,
						(imp->im_flags & ISOFSMNT_NOCASETRANS) == 0,
						isonum_711(ep->flags) & 4, imp->im_joliet_level);
				idp->current.d_namlen = (u_char) namelen;
				if (imp->iso_ftype == ISO_FTYPE_DEFAULT)
					error = iso_shipdir(idp);
				else
					error = iso_uiodir(idp, &idp->current, idp->curroff);
				break;
			}
		}
		if (error)
			break;

		entryoffsetinblock += reclen;
	}

	if (!error && imp->iso_ftype == ISO_FTYPE_DEFAULT) {
		idp->current.d_namlen = 0;
		error = iso_shipdir(idp);
	}
	if (error < 0)
		error = 0;

	if (ap->a_ncookies != NULL) {
		if (error)
			free(cookies, M_TEMP);
		else {
			/*
			 * Work out the number of cookies actually used.
			 */
			*ap->a_ncookies = ncookies - idp->ncookies;
			*ap->a_cookies = cookies;
		}
	}

	if (bp)
		brelse(bp);

	uio->uio_offset = idp->uio_off;
	*ap->a_eofflag = idp->eofflag;

	FREE(idp, M_TEMP);

	return (error);
}

/*
 * Return target name of a symbolic link
 * Shouldn't we get the parent vnode and read the data from there?
 * This could eventually result in deadlocks in cd9660_lookup.
 * But otherwise the block read here is in the block buffer two times.
 */
typedef struct iso_directory_record ISODIR;
typedef struct iso_node             ISONODE;
typedef struct iso_mnt              ISOMNT;
int
cd9660_readlink(ap)
	struct vop_readlink_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
	} */ *ap;
{
	ISONODE	*ip;
	ISODIR	*dirp;
	ISOMNT	*imp;
	struct	buf *bp;
	struct	uio *uio;
	u_short	symlen;
	int	error;
	char	*symname;
	ino_t	ino;

	ip  = VTOI(ap->a_vp);
	imp = ip->i_mnt;
	uio = ap->a_uio;

	if (imp->iso_ftype != ISO_FTYPE_RRIP)
		return (EINVAL);

	/*
	 * Get parents directory record block that this inode included.
	 */
	error = bread(imp->im_devvp,
			(ip->i_number >> imp->im_bshift) << (imp->im_bshift - DEV_BSHIFT),
			imp->logical_block_size, NOCRED, &bp);
	if (error) {
		brelse(bp);
		return (EINVAL);
	}

	/*
	 * Setup the directory pointer for this inode
	 */
	dirp = (ISODIR*) (bp->b_data + (ip->i_number & imp->im_bmask));

	/*
	 * Just make sure, we have a right one....
	 *   1: Check not cross boundary on block
	 */
	if ((ip->i_number & imp->im_bmask) + isonum_711(dirp->length)
			> imp->logical_block_size) {
		brelse(bp);
		return (EINVAL);
	}

	/*
	 * Now get a buffer
	 * Abuse a namei buffer for now.
	 */
	if (uio->uio_segflg == UIO_SYSSPACE && uio->uio_iov->iov_len >= MAXPATHLEN) {
		symname = uio->uio_iov->iov_base;
	} else {
		MALLOC(symname, char*, MAXPATHLEN, M_NAMEI, M_WAITOK);
	}

	/*
	 * Ok, we just gathering a symbolic name in SL record.
	 */
	if (cd9660_rrip_getsymname(dirp, symname, &symlen, imp) == 0) {
		if (uio->uio_segflg
				!= UIO_SYSSPACE|| uio->uio_iov->iov_len < MAXPATHLEN) {
			FREE(symname, M_NAMEI);
		}
		brelse(bp);
		return (EINVAL);
	}
	/*
	 * Don't forget before you leave from home ;-)
	 */
	brelse(bp);

	/*
	 * return with the symbolic name to caller's.
	 */
	if (uio->uio_segflg != UIO_SYSSPACE) {
		error = uiomove(symname, symlen, uio);
		FREE(symname, M_NAMEI);
		return (error);
	}
	uio->uio_resid -= symlen;
	uio->uio_iov->iov_base += symlen;
	uio->uio_iov->iov_len -= symlen;
	return (0);
}

int
cd9660_link(ap)
	struct vop_link_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap;
{
	VOP_ABORTOP(ap->a_vp, ap->a_cnp);
	vput(ap->a_vp);
	return (EROFS);
}

int
cd9660_symlink(ap)
	struct vop_symlink_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap;
{

	VOP_ABORTOP(ap->a_dvp, ap->a_cnp);
	vput(ap->a_dvp);
	return (EROFS);
}

/*
 * Ufs abort op, called after namei() when a CREATE/DELETE isn't actually
 * done. If a buffer has been saved in anticipation of a CREATE, delete it.
 */
int
cd9660_abortop(ap)
	struct vop_abortop_args /* {
		struct vnode *a_dvp;
		struct componentname *a_cnp;
	} */ *ap;
{
	if ((ap->a_cnp->cn_flags & (HASBUF | SAVESTART)) == HASBUF)
		FREE(ap->a_cnp->cn_pnbuf, M_NAMEI);
	return (0);
}

/*
 * Lock an inode.
 */
int
cd9660_lock(ap)
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;

	return (lockmgr(&VTOI(vp)->i_lock, ap->a_flags, &vp->v_interlock,
			ap->a_p->p_pid));
}

/*
 * Unlock an inode.
 */
int
cd9660_unlock(ap)
	struct vop_unlock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;

	return (lockmgr(&VTOI(vp)->i_lock, ap->a_flags | LK_RELEASE, &vp->v_interlock, ap->a_p->p_pid));
}

/*
 * Calculate the logical to physical mapping if not done already,
 * then call the device strategy routine.
 */
int
cd9660_strategy(ap)
	struct vop_strategy_args /* {
		struct buf *a_bp;
	} */ *ap;
{
	register struct buf *bp = ap->a_bp;
	register struct vnode *vp = bp->b_vp;
	register struct iso_node *ip;
	int error;

	ip = VTOI(vp);
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		panic("cd9660_strategy: spec");
	if (bp->b_blkno == bp->b_lblkno) {
		if ((error = VOP_BMAP(vp, bp->b_lblkno, NULL, &bp->b_blkno, NULL))) {
			bp->b_error = error;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return (error);
		}
		if ((long) bp->b_blkno == -1)
			clrbuf(bp);
	}
	if ((long) bp->b_blkno == -1) {
		biodone(bp);
		return (0);
	}
	vp = ip->i_devvp;
	bp->b_dev = vp->v_rdev;
	VOP_STRATEGY(bp);
	return (0);
}

/*
 * Print out the contents of an inode.
 */
int
cd9660_print(ap)
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap;
{

	printf("tag VT_ISOFS, isofs vnode\n");
	return (0);
}

/*
 * Check for a locked inode.
 */
int
cd9660_islocked(ap)
	struct vop_islocked_args /* {
		struct vnode *a_vp;
	} */ *ap;
{

	return (lockstatus(&VTOI(ap->a_vp)->i_lock));
}

/*
 * Return POSIX pathconf information applicable to cd9660 filesystems.
 */
int
cd9660_pathconf(ap)
	struct vop_pathconf_args /* {
		struct vnode *a_vp;
		int a_name;
		register_t *a_retval;
	} */ *ap;
{

	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = 1;
		return (0);
	case _PC_NAME_MAX:
		if (VTOI(ap->a_vp)->i_mnt->iso_ftype == ISO_FTYPE_RRIP)
			*ap->a_retval = NAME_MAX;
		else
			*ap->a_retval = 37;
		return (0);
	case _PC_PATH_MAX:
		*ap->a_retval = PATH_MAX;
		return (0);
	case _PC_PIPE_BUF:
		*ap->a_retval = PIPE_BUF;
		return (0);
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return (0);
	case _PC_NO_TRUNC:
		*ap->a_retval = 1;
		return (0);
	default:
		return (EINVAL);
	}
	/* NOTREACHED */
}

/*
 * Global vfs data structures for cd9660
 */
#define cd9660_create 		((int (*) (struct  vop_create_args *))eopnotsupp)
#define cd9660_mknod 		((int (*) (struct  vop_mknod_args *))eopnotsupp)
#define cd9660_write 		((int (*) (struct  vop_write_args *))eopnotsupp)
#ifdef NFS
int	 lease_check (struct vop_lease_args *);
#define	cd9660_lease_check 	lease_check
#else
#define	cd9660_lease_check 	((int (*) (struct vop_lease_args *))nullop)
#endif
#define	cd9660_revoke 		((int (*) (struct  vop_revoke_args *))vop_norevoke)
#define cd9660_fsync 		((int (*) (struct  vop_fsync_args *))nullop)
#define cd9660_remove 		((int (*) (struct  vop_remove_args *))eopnotsupp)
#define cd9660_link 		((int (*) (struct  vop_link_args *))eopnotsupp)
#define cd9660_rename 		((int (*) (struct  vop_rename_args *))eopnotsupp)
#define cd9660_mkdir 		((int (*) (struct  vop_mkdir_args *))eopnotsupp)
#define cd9660_rmdir 		((int (*) (struct  vop_rmdir_args *))eopnotsupp)
#define cd9660_symlink 		((int (*) (struct  vop_symlink_args *))eopnotsupp)
#define cd9660_advlock 		((int (*) (struct  vop_advlock_args *))eopnotsupp)
#define cd9660_valloc 		((int (*) (struct  vop_valloc_args *))eopnotsupp)
#define cd9660_vfree 		((int (*) (struct  vop_vfree_args *))eopnotsupp)
#define cd9660_truncate 	((int (*) (struct  vop_truncate_args *))eopnotsupp)
#define cd9660_update 		((int (*) (struct  vop_update_args *))eopnotsupp)
#define cd9660_bwrite 		((int (*) (struct  vop_bwrite_args *))eopnotsupp)

struct vnodeops cd9660_vnodeops = {
		.vop_default = vop_default_error,		/* default */
		.vop_lookup = cd9660_lookup,			/* lookup */
		.vop_create = cd9660_create,			/* create */
		.vop_mknod = cd9660_mknod,				/* mknod */
		.vop_open =	cd9660_open,				/* open */
		.vop_close = cd9660_close,				/* close */
		.vop_access = cd9660_access,			/* access */
		.vop_getattr = cd9660_getattr,			/* getattr */
		.vop_setattr = cd9660_setattr,			/* setattr */
		.vop_read = cd9660_read,				/* read */
		.vop_write = cd9660_write,				/* write */
		.vop_lease = cd9660_lease_check,		/* lease */
		.vop_ioctl = cd9660_ioctl,				/* ioctl */
		.vop_select= cd9660_select,				/* select */
		.vop_poll= cd9660_poll,					/* poll */
		.vop_revoke = cd9660_revoke,			/* revoke */
		.vop_mmap = cd9660_mmap,				/* mmap */
		.vop_fsync = cd9660_fsync,				/* fsync */
		.vop_seek = cd9660_seek,				/* seek */
		.vop_remove = cd9660_remove,			/* remove */
		.vop_link = cd9660_link,				/* link */
		.vop_rename = cd9660_rename,			/* rename */
		.vop_mkdir = cd9660_mkdir,				/* mkdir */
		.vop_rmdir = cd9660_rmdir,				/* rmdir */
		.vop_symlink = cd9660_symlink,			/* symlink */
		.vop_readdir = cd9660_readdir,			/* readdir */
		.vop_readlink = cd9660_readlink,		/* readlink */
		.vop_abortop = cd9660_abortop,			/* abortop */
		.vop_inactive = cd9660_inactive,		/* inactive */
		.vop_reclaim = cd9660_reclaim,			/* reclaim */
		.vop_lock = cd9660_lock,				/* lock */
		.vop_unlock = cd9660_unlock,			/* unlock */
		.vop_bmap = cd9660_bmap,				/* bmap */
		.vop_strategy = cd9660_strategy,		/* strategy */
		.vop_print = cd9660_print,				/* print */
		.vop_islocked = cd9660_islocked,		/* islocked */
		.vop_pathconf = cd9660_pathconf,		/* pathconf */
		.vop_advlock = cd9660_advlock,			/* advlock */
		.vop_blkatoff = cd9660_blkatoff,		/* blkatoff */
		.vop_valloc = cd9660_valloc,			/* valloc */
		.vop_vfree = cd9660_vfree,				/* vfree */
		.vop_truncate = cd9660_truncate,		/* truncate */
		.vop_update = cd9660_update,			/* update */
		.vop_bwrite = vn_bwrite,				/* bwrite */
};

/*
 * Special device vnode ops
 */
struct vnodeops cd9660_specops = {
		.vop_default = vop_default_error,		/* default */
		.vop_lookup = spec_lookup,				/* lookup */
		.vop_create = spec_create,				/* create */
		.vop_mknod = spec_mknod,				/* mknod */
		.vop_open =	spec_open,					/* open */
		.vop_close = spec_close,				/* close */
		.vop_access = cd9660_access,			/* access */
		.vop_getattr = cd9660_getattr,			/* getattr */
		.vop_setattr = cd9660_setattr,			/* setattr */
		.vop_read = spec_read,					/* read */
		.vop_write = spec_write,				/* write */
		.vop_lease = spec_lease_check,			/* lease */
		.vop_ioctl = spec_ioctl,				/* ioctl */
		.vop_select= spec_select,				/* select */
		.vop_poll= spec_poll,					/* poll */
		.vop_revoke = spec_revoke,				/* revoke */
		.vop_mmap = spec_mmap,					/* mmap */
		.vop_fsync = spec_fsync,				/* fsync */
		.vop_seek = spec_seek,					/* seek */
		.vop_remove = spec_remove,				/* remove */
		.vop_link = spec_link,					/* link */
		.vop_rename = spec_rename,				/* rename */
		.vop_mkdir = spec_mkdir,				/* mkdir */
		.vop_rmdir = spec_rmdir,				/* rmdir */
		.vop_symlink = spec_symlink,			/* symlink */
		.vop_readdir = spec_readdir,			/* readdir */
		.vop_readlink = spec_readlink,			/* readlink */
		.vop_abortop = spec_abortop,			/* abortop */
		.vop_inactive = cd9660_inactive,		/* inactive */
		.vop_reclaim = cd9660_reclaim,			/* reclaim */
		.vop_lock = cd9660_lock,				/* lock */
		.vop_unlock = cd9660_unlock,			/* unlock */
		.vop_bmap = spec_bmap,					/* bmap */
		.vop_strategy = spec_strategy,			/* strategy */
		.vop_print = cd9660_print,				/* print */
		.vop_islocked = cd9660_islocked,		/* islocked */
		.vop_pathconf = spec_pathconf,			/* pathconf */
		.vop_advlock = spec_advlock,			/* advlock */
		.vop_blkatoff = spec_blkatoff,			/* blkatoff */
		.vop_valloc = spec_valloc,				/* valloc */
		.vop_vfree = spec_vfree,				/* vfree */
		.vop_truncate = spec_truncate,			/* truncate */
		.vop_update = cd9660_update,			/* update */
		.vop_bwrite = vn_bwrite,				/* bwrite */
};

#ifdef FIFO
struct vnodeops cd9660_fifoops = {
		.vop_default = vop_default_error,		/* default */
		.vop_lookup = fifo_lookup, 				/* lookup */
		.vop_create = fifo_create, 				/* create */
		.vop_mknod = fifo_mknod, 				/* mknod */
		.vop_open = fifo_open, 					/* open */
		.vop_close = fifo_close, 				/* close */
		.vop_access = cd9660_access, 			/* access */
		.vop_getattr = cd9660_getattr, 			/* getattr */
		.vop_setattr = cd9660_setattr, 			/* setattr */
		.vop_read = fifo_read, 					/* read */
		.vop_write = fifo_write, 				/* write */
		.vop_lease = fifo_lease_check, 			/* lease */
		.vop_ioctl = fifo_ioctl, 				/* ioctl */
		.vop_select = fifo_select, 				/* select */
		.vop_poll= fifo_poll,					/* poll */
		.vop_revoke = fifo_revoke, 				/* revoke */
		.vop_mmap = fifo_mmap, 					/* mmap */
		.vop_fsync = fifo_fsync, 				/* fsync */
		.vop_seek = fifo_seek, 					/* seek */
		.vop_remove = fifo_remove, 				/* remove */
		.vop_link = fifo_link, 					/* link */
		.vop_rename = fifo_rename, 				/* rename */
		.vop_mkdir = fifo_mkdir, 				/* mkdir */
		.vop_rmdir = fifo_rmdir, 				/* rmdir */
		.vop_symlink = fifo_symlink, 			/* symlink */
		.vop_readdir = fifo_readdir, 			/* readdir */
		.vop_readlink= fifo_readlink, 			/* readlink */
		.vop_abortop = fifo_abortop, 			/* abortop */
		.vop_inactive = cd9660_inactive,		/* inactive */
		.vop_reclaim = cd9660_reclaim, 			/* reclaim */
		.vop_lock = cd9660_lock, 				/* lock */
		.vop_unlock = cd9660_unlock, 			/* unlock */
		.vop_bmap = fifo_bmap, 					/* bmap */
		.vop_strategy = fifo_strategy, 			/* strategy */
		.vop_print = cd9660_print, 				/* print */
		.vop_islocked = cd9660_islocked,		/* islocked */
		.vop_pathconf = fifo_pathconf, 			/* pathconf */
		.vop_advlock  fifo_advlock, 			/* advlock */
		.vop_blkatoff = fifo_blkatoff, 			/* blkatoff */
		.vop_valloc = fifo_valloc, 				/* valloc */
		.vop_vfree = fifo_vfree, 				/* vfree */
		.vop_truncate = fifo_truncate, 			/* truncate */
		.vop_update = cd9660_update, 			/* update */
		.vop_bwrite = vn_bwrite,
};
#endif /* FIFO */
