/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_namei.c	1.5 (2.11BSD GTE) 1997/1/30
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/user.h>

#include <ufs/ufs211/ufs211_quota.h>
#include <ufs/ufs211/ufs211_inode.h>
#include <ufs/ufs211/ufs211_fs.h>
#include <ufs/ufs211/ufs211_mount.h>
#include <ufs/ufs211/ufs211_extern.h>

int	ufs211_dirchk = 0;
#define FSFMT(vp)	((vp)->v_mount->mnt_maxsymlinklen <= 0)

int
ufs211_lookup(ap)
	struct vop_lookup_args *ap;
{
	register struct vnode *vdp; 		/* vnode for directory being searched */
	register struct ufs211_inode *dp; 	/* inode for directory being searched */
	struct buf *bp; 					/* a buffer of directory entries */
	register struct direct *ep; 		/* the current directory entry */
	int entryoffsetinblock; 			/* offset of ep in bp's buffer */
	enum {NONE, COMPACT, FOUND} slotstatus;
	off_t slotoffset = -1;				/* offset of area with free space */
	int slotsize;						/* size of area at slotoffset */
	int slotfreespace;					/* amount of space free in slot */
	int slotneeded;						/* size of the entry we're seeking */
	int numdirpasses; 					/* strategy for directory search */
	ufs211_doff_t endsearch; 			/* offset to end directory search */
	ufs211_doff_t prevoff; 				/* prev entry dp->i_offset */
	struct vnode *pdp; 					/* saved dp during symlink work */
	struct vnode *tdp;					/* returned by VFS_VGET */
	ufs211_doff_t enduseful; 			/* pointer past last used dir slot */
	u_long bmask; 						/* block offset mask */
	int lockparent; 					/* 1 => lockparent flag is set */
	int wantparent; 					/* 1 => wantparent or lockparent flag */
	int docache;						/* == 0 do not cache last component */
	int namlen, error;
	struct vnode **vpp = ap->a_vpp;
	struct componentname *cnp = ap->a_cnp;
	struct ucred *cred = cnp->cn_cred;
	int flags = cnp->cn_flags;
	int nameiop = cnp->cn_nameiop;
	struct proc *p = cnp->cn_proc;

	bp = NULL;
	slotoffset = -1;
	*vpp = NULL;
	vdp = ap->a_dvp;
	dp = UFS211_VTOI(vdp);
	lockparent = flags & LOCKPARENT;
	wantparent = flags & (LOCKPARENT | WANTPARENT);
	docache = 	(flags  & NOCACHE) ^ NOCACHE;

	if (flags == DELETE || lockparent)
		docache = 0;

	/*
	 * Check accessiblity of directory.
	 */
	if ((dp->i_mode & UFS211_IFMT) != UFS211_IFDIR)
		return (ENOTDIR);
	if (error == VOP_ACCESS(vdp, VEXEC, cred, cnp->cn_proc))
		return (error);
	if ((flags & ISLASTCN) && (vdp->v_mount->mnt_flag & MNT_RDONLY)	&& (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME))
		return (EROFS);

	/*
	 * We now have a segment name to search for, and a directory to search.
	 *
	 * Before tediously performing a linear scan of the directory,
	 * check the name cache to see if the directory/name pair
	 * we are looking for is known already.  We don't do this
	 * if the segment name is long, simply so the cache can avoid
	 * holding long names (which would either waste space, or
	 * add greatly to the complexity).
	 */
	if (error == cache_lookup(vdp, vpp, cnp)) {
		int vpid; /* capability number of vnode */
		if (error == ENOENT)
			return (error);
		/*
		 * Get the next vnode in the path.
		 * See comment below starting `Step through' for
		 * an explaination of the locking protocol.
		 */
		pdp = vdp;
		dp = UFS211_VTOI(*vpp);
		vdp = *vpp;
		vpid = vdp->v_id;
		if (pdp == vdp) { /* lookup on "." */
			VREF(vdp);
			error = 0;
		} else if (flags & ISDOTDOT) {
			VOP_UNLOCK(pdp, 0, p);
			error = vget(vdp, LK_EXCLUSIVE, p);
			if (!error && lockparent && (flags & ISLASTCN))
				error = vn_lock(pdp, LK_EXCLUSIVE, p);
		} else {
			error = vget(vdp, LK_EXCLUSIVE, p);
			if (!lockparent || error || !(flags & ISLASTCN))
				VOP_UNLOCK(pdp, 0, p);
		}
		/*
		 * Check that the capability number did not change
		 * while we were waiting for the lock.
		 */
		if (!error) {
			if (vpid == vdp->v_id)
				return (0);
			vput(vdp);
			if (lockparent && pdp != vdp && (flags & ISLASTCN))
				VOP_UNLOCK(pdp, 0, p);
		}
		if (error == vn_lock(pdp, LK_EXCLUSIVE, p))
			return (error);
		vdp = pdp;
		dp = UFS211_VTOI(pdp);
		*vpp = NULL;
	}

	/*
	 * Suppress search for slots unless creating
	 * file and at end of pathname, in which case
	 * we watch for a place to put the new file in
	 * case it doesn't already exist.
	 */
	slotstatus = FOUND;
	slotfreespace = slotsize = slotneeded = 0;
	if ((nameiop == CREATE || nameiop == RENAME) && (flags & ISLASTCN)) {
		slotstatus = NONE;
		slotfreespace = 0;
		slotneeded = (sizeof(struct direct) - UFS211_MAXNAMLEN + cnp->cn_namelen + 3) & ~3;
	}

	/*
	 * If this is the same directory that this process
	 * previously searched, pick up where we last left off.
	 * We cache only lookups as these are the most common
	 * and have the greatest payoff. Caching CREATE has little
	 * benefit as it usually must search the entire directory
	 * to determine that the entry does not exist. Caching the
	 * location of the last DELETE has not reduced profiling time
	 * and hence has been removed in the interest of simplicity.
	 */
	bmask = VFSTOUFS211(vdp->v_mount)->m_mountp->mnt_stat.f_iosize - 1;
	if (nameiop != LOOKUP || dp->i_diroff == 0 || dp->i_diroff > dp->i_size) {
		entryoffsetinblock = 0;
		dp->i_offset = 0;
		numdirpasses = 1;
	} else {
		dp->i_offset = dp->i_diroff;
		if ((entryoffsetinblock = dp->i_offset & bmask) && (error =
				VOP_BLKATOFF(vdp, (off_t ) dp->i_offset, NULL, &bp)))
			return (error);
		numdirpasses = 2;
		nchstats.ncs_2passes++;
	}
	prevoff = dp->i_offset;
	endsearch = roundup(dp->i_size, UFS211_DIRBLKSIZ);
	enduseful = 0;

searchloop:
	while (dp->i_offset < endsearch) {
		/*
		 * If necessary, get the next directory block.
		 */
		if ((dp->i_offset & bmask) == 0) {
			if (bp != NULL)
				brelse(bp);
			if (error == VOP_BLKATOFF(vdp, (off_t) dp->i_offset, NULL, &bp))
				return (error);
			entryoffsetinblock = 0;
		}
		/*
		 * If still looking for a slot, and at a DIRBLKSIZE
		 * boundary, have to start looking for free space again.
		 */
		if (slotstatus == NONE && (entryoffsetinblock & (UFS211_DIRBLKSIZ - 1)) == 0) {
			slotoffset = -1;
			slotfreespace = 0;
		}
		/*
		 * Get pointer to next entry.
		 * Full validation checks are slow, so we only check
		 * enough to insure forward progress through the
		 * directory. Complete checks can be run by patching
		 * "dirchk" to be true.
		 */
		ep = (struct direct*) ((char*) bp->b_data + entryoffsetinblock);
		if (ep->d_reclen == 0 || (ufs211_dirchk && ufs211_dirbadentry(vdp, ep, entryoffsetinblock))) {
			int i;

			ufs211_dirbad(dp, dp->i_offset, "mangled entry");
			i = UFS211_DIRBLKSIZ - (entryoffsetinblock & (UFS211_DIRBLKSIZ - 1));
			dp->i_offset += i;
			entryoffsetinblock += i;
			continue;
		}

		/*
		 * If an appropriate sized slot has not yet been found,
		 * check to see if one is available. Also accumulate space
		 * in the current block so that we can determine if
		 * compaction is viable.
		 */
		if (slotstatus != FOUND) {
			int size = ep->d_reclen;

			if (ep->d_ino != 0)
				size -= DIRSIZ(FSFMT(vdp), ep);
			if (size > 0) {
				if (size >= slotneeded) {
					slotstatus = FOUND;
					slotoffset = dp->i_offset;
					slotsize = ep->d_reclen;
				} else if (slotstatus == NONE) {
					slotfreespace += size;
					if (slotoffset == -1)
						slotoffset = dp->i_offset;
					if (slotfreespace >= slotneeded) {
						slotstatus = COMPACT;
						slotsize = dp->i_offset + ep->d_reclen - slotoffset;
					}
				}
			}
		}
		/*
		 * Check for a name match.
		 */
		if (ep->d_ino) {
#if (BYTE_ORDER == LITTLE_ENDIAN)
			if (vdp->v_mount->mnt_maxsymlinklen > 0)
				namlen = ep->d_namlen;
			else
				namlen = ep->d_type;
#else
				namlen = ep->d_namlen;
#endif
			if (namlen == cnp->cn_namelen
					&& !bcmp(cnp->cn_nameptr, ep->d_name, (unsigned) namlen)) {
				/*
				 * Save directory entry's inode number and
				 * reclen in ndp->ni_ufs area, and release
				 * directory buffer.
				 */
				if (vdp->v_mount->mnt_maxsymlinklen > 0 && ep->d_type == DT_WHT) {
					slotstatus = FOUND;
					slotoffset = dp->i_offset;
					slotsize = ep->d_reclen;
					dp->i_reclen = slotsize;
					enduseful = dp->i_size;
					ap->a_cnp->cn_flags |= ISWHITEOUT;
					numdirpasses--;
					goto notfound;
				}
				dp->i_ino = ep->d_ino;
				dp->i_reclen = ep->d_reclen;
				brelse(bp);
				goto found;
			}
		}
		prevoff = dp->i_offset;
		dp->i_offset += ep->d_reclen;
		entryoffsetinblock += ep->d_reclen;
		if (ep->d_ino)
			enduseful = dp->i_offset;
	}
notfound:
	/*
	 * If we started in the middle of the directory and failed
	 * to find our target, we must check the beginning as well.
	 */
	if (numdirpasses == 2) {
		numdirpasses--;
		dp->i_offset = 0;
		endsearch = dp->i_diroff;
		goto searchloop;
	}
	if (bp != NULL)
		brelse(bp);
	/*
	 * If creating, and at end of pathname and current
	 * directory has not been removed, then can consider
	 * allowing file to be created.
	 */
	if ((nameiop == CREATE || nameiop == RENAME || (nameiop == DELETE && (ap->a_cnp->cn_flags & DOWHITEOUT) && (ap->a_cnp->cn_flags & ISWHITEOUT))) && (flags & ISLASTCN) && dp->i_nlink != 0) {
		/*
		 * Access for write is interpreted as allowing
		 * creation of files in the directory.
		 */
		if (error == VOP_ACCESS(vdp, VWRITE, cred, cnp->cn_proc))
			return (error);
		/*
		 * Return an indication of where the new directory
		 * entry should be put.  If we didn't find a slot,
		 * then set dp->i_count to 0 indicating
		 * that the new slot belongs at the end of the
		 * directory. If we found a slot, then the new entry
		 * can be put in the range from dp->i_offset to
		 * dp->i_offset + dp->i_count.
		 */
		if (slotstatus == NONE) {
			dp->i_offset = roundup(dp->i_size, UFS211_DIRBLKSIZ);
			dp->i_count = 0;
			enduseful = dp->i_offset;
		} else if (nameiop == DELETE) {
			dp->i_offset = slotoffset;
			if ((dp->i_offset & (UFS211_DIRBLKSIZ - 1)) == 0)
				dp->i_count = 0;
			else
				dp->i_count = dp->i_offset - prevoff;
		} else {
			dp->i_offset = slotoffset;
			dp->i_count = slotsize;
			if (enduseful < slotoffset + slotsize)
				enduseful = slotoffset + slotsize;
		}
		dp->i_endoff = roundup(enduseful, UFS211_DIRBLKSIZ);
		dp->i_flag |= UFS211_ICHG | UFS211_IUPD;
		/*
		 * We return with the directory locked, so that
		 * the parameters we set up above will still be
		 * valid if we actually decide to do a direnter().
		 * We return ni_vp == NULL to indicate that the entry
		 * does not currently exist; we leave a pointer to
		 * the (locked) directory inode in ndp->ni_dvp.
		 * The pathname buffer is saved so that the name
		 * can be obtained later.
		 *
		 * NB - if the directory is unlocked, then this
		 * information cannot be used.
		 */
		cnp->cn_flags |= SAVENAME;
		if (!lockparent)
			VOP_UNLOCK(vdp, 0, p);
		return (EJUSTRETURN);
	}
	/*
	 * Insert name into cache (as non-existent) if appropriate.
	 */
	if ((cnp->cn_flags & MAKEENTRY) && nameiop != CREATE)
		cache_enter(vdp, *vpp, cnp);
	return (ENOENT);

    found: 
	if (numdirpasses == 2)
		nchstats.ncs_pass2++;
	/*
	 * Check that directory length properly reflects presence
	 * of this entry.
	 */
	if (entryoffsetinblock + DIRSIZ(FSFMT(vdp), ep) > dp->i_size) {
		ufs211_dirbad(dp, dp->i_offset, "i_size too small");
		dp->i_size = entryoffsetinblock + DIRSIZ(FSFMT(vdp), ep);
		dp->i_flag |= UFS211_ICHG | UFS211_IUPD;
	}

	/*
	 * Found component in pathname.
	 * If the final component of path name, save information
	 * in the cache as to where the entry was found.
	 */
	if ((flags & ISLASTCN) && nameiop == LOOKUP)
		dp->i_diroff = dp->i_offset & ~(UFS211_DIRBLKSIZ - 1);

	/*
	 * If deleting, and at end of pathname, return
	 * parameters which can be used to remove file.
	 * If the wantparent flag isn't set, we return only
	 * the directory (in ndp->ni_dvp), otherwise we go
	 * on and lock the inode, being careful with ".".
	 */
	if (nameiop == DELETE && (flags & ISLASTCN)) {
		/*
		 * Write access to directory required to delete files.
		 */
		if (error == VOP_ACCESS(vdp, VWRITE, cred, cnp->cn_proc))
			return (error);
		/*
		 * Return pointer to current entry in dp->i_offset,
		 * and distance past previous entry (if there
		 * is a previous entry in this block) in dp->i_count.
		 * Save directory inode pointer in ndp->ni_dvp for dirremove().
		 */
		if ((dp->i_offset & (UFS211_DIRBLKSIZ - 1)) == 0)
			dp->i_count = 0;
		else
			dp->i_count = dp->i_offset - prevoff;
		if (dp->i_number == dp->i_ino) {
			VREF(vdp);
			*vpp = vdp;
			return (0);
		}
		if (error == VFS_VGET(vdp->v_mount, dp->i_ino, &tdp))
			return (error);
		/*
		 * If directory is "sticky", then user must own
		 * the directory, or the file in it, else she
		 * may not delete it (unless she's root). This
		 * implements append-only directories.
		 */
		if ((dp->i_mode & UFS211_ISVTX) && cred->cr_uid != 0
				&& cred->cr_uid != dp->i_uid && tdp->v_type != VLNK
				&& UFS211_VTOI(tdp)->i_uid != cred->cr_uid) {
			vput(tdp);
			return (EPERM);
		}
		*vpp = tdp;
		if (!lockparent)
			VOP_UNLOCK(vdp, 0, p);
		return (0);
	}
	/*
	 * If rewriting (RENAME), return the inode and the
	 * information required to rewrite the present directory
	 * Must get inode of directory entry to verify it's a
	 * regular file, or empty directory.
	 */
	if (nameiop == RENAME && wantparent && (flags & ISLASTCN)) {
		if (error == VOP_ACCESS(vdp, VWRITE, cred, cnp->cn_proc))
			return (error);
		/*
		 * Careful about locking second inode.
		 * This can only occur if the target is ".".
		 */
		if (dp->i_number == dp->i_ino)
			return (EISDIR);
		if (error == VFS_VGET(vdp->v_mount, dp->i_ino, &tdp))
			return (error);
		*vpp = tdp;
		cnp->cn_flags |= SAVENAME;
		if (!lockparent)
			VOP_UNLOCK(vdp, 0, p);
		return (0);
	}

	/*
	 * Step through the translation in the name.  We do not `vput' the
	 * directory because we may need it again if a symbolic link
	 * is relative to the current directory.  Instead we save it
	 * unlocked as "pdp".  We must get the target inode before unlocking
	 * the directory to insure that the inode will not be removed
	 * before we get it.  We prevent deadlock by always fetching
	 * inodes from the root, moving down the directory tree. Thus
	 * when following backward pointers ".." we must unlock the
	 * parent directory before getting the requested directory.
	 * There is a potential race condition here if both the current
	 * and parent directories are removed before the VFS_VGET for the
	 * inode associated with ".." returns.  We hope that this occurs
	 * infrequently since we cannot avoid this race condition without
	 * implementing a sophisticated deadlock detection algorithm.
	 * Note also that this simple deadlock detection scheme will not
	 * work if the file system has any hard links other than ".."
	 * that point backwards in the directory structure.
	 */
	pdp = vdp;
	if (flags & ISDOTDOT) {
		VOP_UNLOCK(pdp, 0, p); /* race to get the inode */
		if (error == VFS_VGET(vdp->v_mount, dp->i_ino, &tdp)) {
			vn_lock(pdp, LK_EXCLUSIVE | LK_RETRY, p);
			return (error);
		}
		if (lockparent && (flags & ISLASTCN)
				&& (error = vn_lock(pdp, LK_EXCLUSIVE, p))) {
			vput(tdp);
			return (error);
		}
		*vpp = tdp;
	} else if (dp->i_number == dp->i_ino) {
		VREF(vdp); /* we want ourself, ie "." */
		*vpp = vdp;
	} else {
		if (error == VFS_VGET(vdp->v_mount, dp->i_ino, &tdp))
			return (error);
		if (!lockparent || !(flags & ISLASTCN))
			VOP_UNLOCK(pdp, 0, p);
		*vpp = tdp;
	}

	/*
	 * Insert name into cache if appropriate.
	 */
	if (cnp->cn_flags & MAKEENTRY)
		cache_enter(vdp, *vpp, cnp);
	return (0);
}

void
ufs211_dirbad(ip, offset, how)
	struct ufs211_inode *ip;
	off_t offset;
	char *how;
{
	struct mount *mp;
	mp = UFS211_ITOV(ip)->v_mount;
	//printf("%s: bad dir I=%u off %ld: %s\n", ip->i_fs.fs_fsmnt, ip->i_number, offset, how);
	if ((mp->mnt_stat.f_flags & MNT_RDONLY) == 0)
		panic("bad dir");
}

/*
 * Do consistency checking on a directory entry:
 *	record length must be multiple of 4
 *	entry must fit in rest of its DIRBLKSIZ block
 *	record must be large enough to contain entry
 *	name is not longer than MAXNAMLEN
 *	name must be as long as advertised, and null terminated
 */
int
ufs211_dirbadentry(dp, ep, entryoffsetinblock)
	struct vnode *dp;
	register struct direct *ep;
	int entryoffsetinblock;
{
	register int i;
	int namlen;

#if (BYTE_ORDER == LITTLE_ENDIAN)
	if (dp->v_mount->mnt_maxsymlinklen > 0)
		namlen = ep->d_namlen;
	else
		namlen = ep->d_type;
#else
		namlen = ep->d_namlen;
#endif
	if ((ep->d_reclen & 0x3) != 0
			|| ep->d_reclen > UFS211_DIRBLKSIZ - (entryoffsetinblock & (UFS211_DIRBLKSIZ - 1))
			|| ep->d_reclen < DIRSIZ(FSFMT(dp), ep) || namlen > UFS211_MAXNAMLEN)
		/*return (1); */
		printf("First bad\n");
		return (1);
	for (i = 0; i < namlen; i++)
		if (ep->d_name[i] == '\0') {
			/*return (1); */
			printf("Second bad\n");
			return (1);
		}
	if (ep->d_name[i]) {
		return (1);
	}
	return (0);
}

/*
 * Write a directory entry after a call to namei, using the parameters
 * which it left in the u. area.  The argument ip is the inode which
 * the new directory entry will refer to.  The u. area field ndp->ni_pdir is
 * a pointer to the directory to be written, which was left locked by
 * namei.  Remaining parameters (ndp->ni_offset, ndp->ni_count) indicate
 * how the space for the new entry is to be gotten.
 */
int
ufs211_direnter(ip, dvp, cnp)
	struct ufs211_inode *ip;
	struct vnode *dvp;
	register struct componentname *cnp;
{
	register struct ufs211_inode *dp;
	struct direct newdir;

#ifdef DIAGNOSTIC
	if ((cnp->cn_flags & SAVENAME) == 0)
		panic("direnter: missing name");
#endif
	dp = UFS211_VTOI(dvp);
	newdir.d_ino = ip->i_number;
	newdir.d_namlen = cnp->cn_namelen;
	bcopy(cnp->cn_nameptr, newdir.d_name, (unsigned) cnp->cn_namelen + 1);
	if (dvp->v_mount->mnt_maxsymlinklen > 0)
		newdir.d_type = IFTODT(ip->i_mode);
	else {
		newdir.d_type = 0;
#if (BYTE_ORDER == LITTLE_ENDIAN)
		{
			u_char tmp = newdir.d_namlen;
			newdir.d_namlen = newdir.d_type;
			newdir.d_type = tmp;
		}
#endif
	}
	//return (ufs211_direnter2(dvp, &newdir, cnp->cn_cred, cnp->cn_proc));
	return (ufs211_direnter2(ip, &newdir, dvp, cnp));
}

int
ufs211_direnter2(ip, dirp, dvp, cnp)
	struct ufs211_inode *ip;
	struct direct *dirp;
	struct vnode *dvp;
	register struct componentname *cnp;
{
	register struct direct *ep, *nep;
	register struct ufs211_inode *dp;
	struct buf *bp;
	struct ucred *cr;
	struct iovec aiov;
	struct uio auio;
	int loc, spacefree, error = 0;
	u_int dsize;
	int newentrysize;
	char *dirbuf;

#ifdef DIAGNOSTIC
	if ((cnp->cn_flags & SAVENAME) == 0)
		panic("direnter: missing name");
#endif
	dp = UFS211_VTOI(dvp);
	newentrysize = DIRSIZ(FSFMT(dvp), dirp);

	ep->d_ino = ip->i_number;
	ep->d_namlen = cnp->cn_namelen;
	bcopy(cnp->cn_nameptr, ep->d_name, (unsigned) cnp->cn_namelen + 1);
	if (dvp->v_mount->mnt_maxsymlinklen > 0) {
		ep->d_type = IFTODT(ip->i_mode);
	} else {
#if (BYTE_ORDER == LITTLE_ENDIAN)
			{ u_char tmp = ep->d_namlen;
			ep->d_namlen = ep->d_type;
			ep->d_type = tmp; }
#endif
	}

	if(dp->i_count == 0) {
		/*
		 * If dp->i_count is 0, then namei could find no space in the
		 * directory. In this case dp->i_offset will be on a directory
		 * block boundary and we will write the new entry into a fresh
		 * block.
		 */
		if (dp->i_offset & (UFS211_DIRBLKSIZ-1)) {
			panic("wdir: newblk");
		}
		auio.uio_offset = dp->i_offset;
		dirp->d_reclen = UFS211_DIRBLKSIZ;
		auio.uio_resid = newentrysize;
		aiov.iov_len = newentrysize;
		aiov.iov_base = (caddr_t)dirp;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_rw = UIO_WRITE;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_procp = (struct proc *)0;
		error = VOP_WRITE(dvp, &auio, IO_SYNC, cnp->cn_cred);

		if (UFS211_DIRBLKSIZ > VFSTOUFS211(dvp->v_mount)->m_mountp->mnt_stat.f_bsize) {
			/* XXX should grow with balloc() */
			panic("ufs_direnter: frag size");
		} else if (!error) {
			dp->i_size = roundup(dp->i_size, UFS211_DIRBLKSIZ);
			dp->i_flag |= UFS211_ICHG;
		}
		return (error);
	}

	/*
	 * If ndp->ni_count is non-zero, then namei found space for the new
	 * entry in the range ndp->ni_offset to ndp->ni_offset + ndp->ni_count.
	 * in the directory.  To use this space, we may have to compact
	 * the entries located there, by copying them together towards
	 * the beginning of the block, leaving the free space in
	 * one usable chunk at the end.
	 */

	/*
	 * Increase size of directory if entry eats into new space.
	 * This should never push the size past a new multiple of
	 * DIRBLKSIZE.
	 *
	 * N.B. - THIS IS AN ARTIFACT OF 4.2 AND SHOULD NEVER HAPPEN.
	 */
	if (dp->i_offset + dp->i_count > dp->i_size)
			dp->i_size = dp->i_offset + dp->i_count;
	/*
	 * Get the block containing the space for the new directory
	 * entry.  Should return error by result instead of u.u_error.
	 */
	if (error == VOP_BLKATOFF(dvp, (off_t)dp->i_offset, &dirbuf, &bp))
			return (u.u_error);

	/*
	 * Find space for the new entry.  In the simple case, the
	 * entry at offset base will have the space.  If it does
	 * not, then namei arranged that compacting the region
	 * ndp->ni_offset to ndp->ni_offset+ndp->ni_count would yield the space.
	 */

	ep = (struct direct *)dirbuf;
	dsize = DIRSIZ(FSFMT(dvp), ep);
	spacefree = ep->d_reclen - dsize;
	for (loc = ep->d_reclen; loc < dp->i_count;) {
		nep = (struct direct*) (dirbuf + loc);
		if (ep->d_ino) {
			/* trim the existing slot */
			ep->d_reclen = dsize;
			ep = (struct direct*) ((char*) ep + dsize);
		} else {
			/* overwrite; nothing there; header is ours */
			spacefree += dsize;
		}
		dsize = DIRSIZ(FSFMT(dvp), nep);
		spacefree += nep->d_reclen - dsize;
		loc += nep->d_reclen;
		bcopy((caddr_t) nep, (caddr_t) ep, dsize);
	}
	/*
	 * Update the pointer fields in the previous entry (if any),
	 * copy in the new entry, and write out the block.
	 */
	if (ep->d_ino == 0|| (ep->d_ino == UFS211_WINO && bcmp(ep->d_name, dirp->d_name, dirp->d_namlen) == 0)) {
		if (spacefree + dsize < newentrysize)
			panic("wdir: compact1");
		dirp->d_reclen = spacefree + dsize;
	} else {
		if (spacefree < newentrysize)
			panic("wdir: compact2");
		dirp->d_reclen = spacefree;
		ep->d_reclen = dsize;
		ep = (struct direct *)((char *)ep + dsize);
	}
	bcopy((caddr_t)&dirp, (caddr_t)ep, (u_int)newentrysize);
	error = VOP_BWRITE(bp);
	dp->i_flag |= UFS211_ICHG | UFS211_IUPD;
	if (!error && dp->i_endoff && dp->i_endoff < dp->i_size)
		error = VOP_TRUNCATE(dvp, (off_t)dp->i_endoff, IO_SYNC, cnp->cn_cred, cnp->cn_proc);
	return (error);
}

/*
 * Remove a directory entry after a call to namei, using the
 * parameters which it left in the u. area.  The u. entry
 * ni_offset contains the offset into the directory of the
 * entry to be eliminated.  The ni_count field contains the
 * size of the previous record in the directory.  If this
 * is 0, the first entry is being deleted, so we need only
 * zero the inode number to mark the entry as free.  If the
 * entry isn't the first in the directory, we must reclaim
 * the space of the now empty record by adding the record size
 * to the size of the previous entry.
 */
int
ufs211_dirremove(dvp, cnp)
	struct vnode *dvp;
	struct componentname *cnp;
{
	register struct ufs211_inode *dp;
	struct buf *bp;
	struct direct *ep;

	dp = UFS211_VTOI(dvp);

	if (cnp->cn_flags & DOWHITEOUT) {
		/*
		 * Whiteout entry: set d_ino to WINO.
		 */
		if(u.u_error == VOP_BLKATOFF(dvp, (off_t)dp->i_offset, (char **)&ep, &bp))
			return (u.u_error);
		ep->d_ino = UFS211_WINO;
		ep->d_type = DT_WHT;
		u.u_error = VOP_BWRITE(bp);
		dp->i_flag |= UFS211_ICHG | UFS211_IUPD;
		return (u.u_error);
	}
	if (dp->i_count == 0) {

		/*
		 * First entry in block: set d_ino to zero.
		 */
		if(u.u_error == VOP_BLKATOFF(dvp, (off_t)dp->i_offset, (char **)&ep, &bp))
			return (u.u_error);
		ep->d_ino = 0;
		u.u_error = VOP_BWRITE(bp);
		dp->i_flag |= UFS211_ICHG | UFS211_IUPD;
		return (u.u_error);
	}
	/*
	 * Collapse new free space into previous entry.
	 */
	if (u.u_error == VOP_BLKATOFF(dvp, (off_t)(dp->i_offset - dp->i_count), (char **)&ep, &bp))
		return (u.u_error);
	ep->d_reclen += dp->i_reclen;
	u.u_error = VOP_BWRITE(bp);
	dp->i_flag |= UFS211_ICHG | UFS211_IUPD;
	return (u.u_error);
}

/*
 * Rewrite an existing directory entry to point at the inode
 * supplied.  The parameters describing the directory entry are
 * set up by a call to namei.
 */
int
ufs211_dirrewrite(dp, ip, cnp)
	struct ufs211_inode *dp, *ip;
	struct componentname *cnp;
{
	struct buf *bp;
	struct direct *ep;
	struct vnode *vdp = UFS211_ITOV(dp);

	if (u.u_error == VOP_BLKATOFF(vdp, (off_t)dp->i_offset, (char **)&ep, &bp)) {
		return (u.u_error);
	}
	ep->d_ino = ip->i_number;
	if (vdp->v_mount->mnt_maxsymlinklen > 0)
		ep->d_type = IFTODT(ip->i_mode);

	u.u_error = VOP_BWRITE(bp);
	dp->i_flag |= UFS211_ICHG | UFS211_IUPD;
	return (u.u_error);
}

/*
 * Check if a directory is empty or not.
 * Inode supplied must be locked.
 *
 * Using a struct dirtemplate here is not precisely
 * what we want, but better than using a struct direct.
 *
 * NB: does not handle corrupted directories.
 */
int
ufs211_dirempty(ip, parentino)
	register struct ufs211_inode *ip;
	ino_t parentino;
{
	register off_t off;
	struct ufs211_dirtemplate dbuf;
	register struct direct *dp = (struct direct *)&dbuf;
	int error, count, namlen;
#define	MINDIRSIZ (sizeof (struct ufs211_dirtemplate) / 2)

	for (off = 0; off < ip->i_size; off += dp->d_reclen) {
		error = vn_rdwr(UIO_READ, UFS211_ITOV(ip), (caddr_t)dp, MINDIRSIZ, off, UIO_SYSSPACE, IO_NODELOCKED, u.u_ucred, &count, (struct proc*) 0);
		/*
		 * Since we read MINDIRSIZ, residual must
		 * be 0 unless we're at end of file.
		 */
		if (error || count != 0)
			return (0);
		/* avoid infinite loops */
		if (dp->d_reclen == 0)
			return (0);
		/* skip empty entries */
		if (dp->d_ino == 0)
			continue;
		/* accept only "." and ".." */
#if (BYTE_ORDER == LITTLE_ENDIAN)
		if (UFS211_ITOV(ip)->v_mount->mnt_maxsymlinklen > 0)
			namlen = dp->d_namlen;
		else
			namlen = dp->d_type;
#else
			namlen = dp->d_namlen;
#endif
		if (namlen > 2)
			return (0);
		if (dp->d_name[0] != '.')
			return (0);
		/*
		 * At this point d_namlen must be 1 or 2.
		 * 1 implies ".", 2 implies ".." if second
		 * char is also "."
		 */
		if (namlen == 1)
			continue;
		if (dp->d_name[1] == '.' && dp->d_ino == parentino)
			continue;
		return (0);
	}
	return (1);
}

/*
 * Check if source directory is in the path of the target directory.
 * Target is supplied locked, source is unlocked.
 * The target is always vput() before returning.
 */
int
ufs211_checkpath(source, target)
	struct ufs211_inode *source, *target;
{
	struct ufs211_dirtemplate dirbuf;
	struct vnode *vp;
	register int error, rootino, namlen;

	vp = UFS211_ITOV(target);
	if (target->i_number == source->i_number) {
		error = EEXIST;
		goto out;
	}

	rootino = UFS211_ROOTINO;
	error = 0;
	if (target->i_number == rootino)
		goto out;

	for (;;) {
		if (vp->v_type != VDIR) {
			error = ENOTDIR;
			break;
		}
		error = vn_rdwr(UIO_READ, vp, (caddr_t) & dirbuf,
				sizeof(struct ufs211_dirtemplate), (off_t) 0, UIO_SYSSPACE,
				IO_NODELOCKED, u.u_ucred, (int*) 0, (struct proc*) 0);
		if (error != 0)
			break;
#if (BYTE_ORDER == LITTLE_ENDIAN)
		if (vp->v_mount->mnt_maxsymlinklen > 0) {
			namlen = dirbuf.dotdot_namlen;
		} else {
			namlen = dirbuf.dotdot_type;
#else
			namlen = dirbuf.dotdot_namlen;
#endif
		}
		if (dirbuf.dotdot_namlen != 2 || dirbuf.dotdot_name[0] != '.'
				|| dirbuf.dotdot_name[1] != '.') {
			error = ENOTDIR;
			break;
		}
		if (dirbuf.dotdot_ino == source->i_number) {
			error = EINVAL;
			break;
		}
		if (dirbuf.dotdot_ino == rootino)
			break;
		vput(vp);
		if (error == VFS_VGET(vp->v_mount, dirbuf.dotdot_ino, &vp)) {
			error = u.u_error;
			vp = NULL;
			break;
		}
	}

out:
	if (error == ENOTDIR) {
		printf("checkpath: .. !dir\n");
	}
	if (vp != NULL) {
		vput(vp);
	}
	return (error);
}
