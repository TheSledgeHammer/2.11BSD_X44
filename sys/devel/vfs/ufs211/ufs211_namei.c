/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_namei.c	1.5 (2.11BSD GTE) 1997/1/30
 */
#include <sys/param.h>

#include <sys/systm.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/namei.h>

#include "vfs/ufs211/ufs211_dir.h"
#include "vfs/ufs211/ufs211_extern.h"
#include "vfs/ufs211/ufs211_fs.h"
#include "vfs/ufs211/ufs211_inode.h"
#include "vfs/ufs211/ufs211_mount.h"
#include "vfs/ufs211/ufs211_quota.h"

struct	buf *blkatoff();
int	dirchk = 0;

/*
 * Structures associated with name cacheing.
 */
#define	NCHHASH		16	/* size of hash table */

#if	((NCHHASH)&((NCHHASH)-1)) != 0
#define	NHASH(h, i, d)	((unsigned)((h) + (i) + 13 * (int)(d)) % (NCHHASH))
#else
#define	NHASH(h, i, d)	((unsigned)((h) + (i) + 13 * (int)(d)) & ((NCHHASH)-1))
#endif

union nchash {
	union	nchash 		*nch_head[2];
	struct	namecache 	*nch_chain[2];
} nchash[NCHHASH];
#define	nch_forw	nch_chain[0]
#define	nch_back	nch_chain[1]

struct	namecache *nchhead, **nchtail;	/* LRU chain pointers */
struct	nchstats nchstats;				/* cache effectiveness statistics */

void
dirbad(ip, offset, how)
	struct ufs211_inode *ip;
	off_t offset;
	char *how;
{

	printf("%s: bad dir I=%u off %ld: %s\n",
	    ip->i_fs->fs_fsmnt, ip->i_number, offset, how);
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
dirbadentry(ep, entryoffsetinblock)
	register struct direct *ep;
	int entryoffsetinblock;
{
	register int i;

	if ((ep->d_reclen & 0x3) != 0 ||
	    ep->d_reclen > DIRBLKSIZ - (entryoffsetinblock & (DIRBLKSIZ - 1)) ||
	    ep->d_reclen < DIRSIZ(ep) || ep->d_namlen > MAXNAMLEN)
		return (1);
	for (i = 0; i < ep->d_namlen; i++)
		if (ep->d_name[i] == '\0')
			return (1);
	return (ep->d_name[i]);
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
direnter(ip, ndp)
	struct ufs211_inode *ip;
	register struct nameidata *ndp;
{
	register struct ufs211_direct *ep, *nep;
	register struct ufs211_inode *dp = ndp->ni_pdir;
	struct buf *bp;
	int loc, spacefree, error = 0;
	u_int dsize;
	int newentrysize;
	char *dirbuf;

	ndp->ni_dent.d_ino = ip->i_number;
	newentrysize = DIRSIZ(&ndp->ni_dent);
	if (ndp->ni_count == 0) {
		/*
		 * If ndp->ni_count is 0, then namei could find no space in the
		 * directory. In this case ndp->ni_offset will be on a directory
		 * block boundary and we will write the new entry into a fresh
		 * block.
		 */
		if (ndp->ni_offset&(DIRBLKSIZ-1))
			panic("wdir: newblk");
		ndp->ni_dent.d_reclen = DIRBLKSIZ;
		error = rdwri(UIO_WRITE, dp, (caddr_t)&ndp->ni_dent,
		    		newentrysize, ndp->ni_offset, UIO_SYSSPACE, 
				IO_UNIT|IO_SYNC, (int *)0);
		dp->i_size = roundup(dp->i_size, DIRBLKSIZ);
		iput(dp);
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
	if (ndp->ni_offset + ndp->ni_count > dp->i_size)
		dp->i_size = ndp->ni_offset + ndp->ni_count;
	/*
	 * Get the block containing the space for the new directory
	 * entry.  Should return error by result instead of u.u_error.
	 */
	bp = blkatoff(dp, ndp->ni_offset, (char **)&dirbuf);
	if (bp == 0) {
		iput(dp);
		return (u->u_error);
	}
	/*
	 * Find space for the new entry.  In the simple case, the
	 * entry at offset base will have the space.  If it does
	 * not, then namei arranged that compacting the region
	 * ndp->ni_offset to ndp->ni_offset+ndp->ni_count would yield the space.
	 */
	ep = (struct direct *)dirbuf;
	dsize = DIRSIZ(ep);
	spacefree = ep->d_reclen - dsize;
	for (loc = ep->d_reclen; loc < ndp->ni_count; ) {
		nep = (struct direct *)(dirbuf + loc);
		if (ep->d_ino) {
			/* trim the existing slot */
			ep->d_reclen = dsize;
			ep = (struct direct *)((char *)ep + dsize);
		} else {
			/* overwrite; nothing there; header is ours */
			spacefree += dsize;	
		}
		dsize = DIRSIZ(nep);
		spacefree += nep->d_reclen - dsize;
		loc += nep->d_reclen;
		bcopy((caddr_t)nep, (caddr_t)ep, dsize);
	}
	/*
	 * Update the pointer fields in the previous entry (if any),
	 * copy in the new entry, and write out the block.
	 */
	if (ep->d_ino == 0) {
		if (spacefree + dsize < newentrysize)
			panic("wdir: compact1");
		ndp->ni_dent.d_reclen = spacefree + dsize;
	} else {
		if (spacefree < newentrysize)
			panic("wdir: compact2");
		ndp->ni_dent.d_reclen = spacefree;
		ep->d_reclen = dsize;
		ep = (struct direct *)((char *)ep + dsize);
	}
	bcopy((caddr_t)&ndp->ni_dent, (caddr_t)ep, (u_int)newentrysize);
	mapout(bp);
	bwrite(bp);
	dp->i_flag |= UFS211_IUPD|UFS211_ICHG;
	if (ndp->ni_endoff && ndp->ni_endoff < dp->i_size)
		itrunc(dp, (u_long)ndp->ni_endoff, 0);
	iput(dp);
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
dirremove(ndp)
	register struct nameidata *ndp;
{
	register struct ufs211_inode *dp = ndp->ni_pdir;
	register struct buf *bp;
	struct ufs211_direct *ep;

	if (ndp->ni_count == 0) {
		/*
		 * First entry in block: set d_ino to zero.
		 */
		ndp->ni_dent.d_ino = 0;
		(void) rdwri(UIO_WRITE, dp, (caddr_t)&ndp->ni_dent,
		    		(int)DIRSIZ(&ndp->ni_dent), ndp->ni_offset,
				UIO_SYSSPACE, IO_UNIT|IO_SYNC, (int *)0);
	} else {
		/*
		 * Collapse new free space into previous entry.
		 */
		bp = blkatoff(dp, ndp->ni_offset - ndp->ni_count, (char **)&ep);
		if (bp == 0)
			return (0);
		ep->d_reclen += ndp->ni_dent.d_reclen;
		mapout(bp);
		bwrite(bp);
		dp->i_flag |= UFS211_IUPD|UFS211_ICHG;
	}
	return (1);
}

/*
 * Rewrite an existing directory entry to point at the inode
 * supplied.  The parameters describing the directory entry are
 * set up by a call to namei.
 */
void
dirrewrite(dp, ip, ndp)
	register struct ufs211_inode *dp;
	struct ufs211_inode *ip;
	register struct nameidata *ndp;
{

	ndp->ni_cdir.d_ino = ip->i_number;
	u->u_error = rdwri(UIO_WRITE, dp, (caddr_t)&ndp->ni_dent,
			(int)DIRSIZ(&ndp->ni_dent), ndp->ni_offset,	UIO_SYSSPACE, IO_UNIT|IO_SYNC, (int *)0);
	iput(dp);
}

/*
 * Return buffer with contents of block "offset"
 * from the beginning of directory "ip".  If "res"
 * is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 *
 * A mapin() of the buffer is done even if "res" is zero so that the
 * mapout() done later will have something to work with.
 */
struct buf *
blkatoff(ip, offset, res)
	struct ufs211_inode *ip;
	ufs211_off_t offset;
	char **res;
{
	register struct ufs211_fs *fs = ip->i_fs;
	ufs211_daddr_t lbn = lblkno(offset);
	register struct buf *bp;
	ufs211_daddr_t bn;
	char *junk;

	bn = bmap(ip, lbn, B_READ, 0);
	if (u->u_error)
		return (0);
	if (bn == (ufs211_daddr_t)-1) {
		dirbad(ip, offset, "hole in dir");
		return (0);
	}
	bp = bread(ip->i_dev, bn);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (0);
	}
	junk = (caddr_t)mapin(bp);
	if (res)
		*res = junk + (u_int)blkoff(offset);
	return (bp);
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
dirempty(ip, parentino)
	register struct ufs211_inode *ip;
	ufs211_ino_t parentino;
{
	register ufs211_off_t off;
	struct dirtemplate dbuf;
	register struct ufs211_direct *dp = (struct ufs211_direct *)&dbuf;
	int error, count;
#define	MINDIRSIZ (sizeof (struct dirtemplate) / 2)

	for (off = 0; off < ip->i_size; off += dp->d_reclen) {
		error = rdwri(UIO_READ, ip, (caddr_t)dp, MINDIRSIZ,
		    off, UIO_SYSSPACE, IO_UNIT, &count);
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
		if (dp->d_namlen > 2)
			return (0);
		if (dp->d_name[0] != '.')
			return (0);
		/*
		 * At this point d_namlen must be 1 or 2.
		 * 1 implies ".", 2 implies ".." if second
		 * char is also "."
		 */
		if (dp->d_namlen == 1)
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
 * The target is always iput() before returning.
 */
int
checkpath(source, target)
	struct ufs211_inode *source, *target;
{
	struct dirtemplate dirbuf;
	register struct ufs211_inode *ip;
	register int error = 0;

	ip = target;
	if (ip->i_number == source->i_number) {
		error = EEXIST;
		goto out;
	}
	if (ip->i_number == UFS211_ROOTINO)
		goto out;

	for (;;) {
		if ((ip->i_mode&UFS211_IFMT) != UFS211_IFDIR) {
			error = ENOTDIR;
			break;
		}
		error = rdwri(UIO_READ, ip, (caddr_t)&dirbuf, 
				sizeof(struct dirtemplate), (ufs211_off_t)0,
				UIO_SYSSPACE, IO_UNIT, (int *)0);
		if (error != 0)
			break;
		if (dirbuf.dotdot_namlen != 2 ||
		    dirbuf.dotdot_name[0] != '.' ||
		    dirbuf.dotdot_name[1] != '.') {
			error = ENOTDIR;
			break;
		}
		if (dirbuf.dotdot_ino == source->i_number) {
			error = EINVAL;
			break;
		}
		if (dirbuf.dotdot_ino == UFS211_ROOTINO)
			break;
		iput(ip);
		ip = iget(ip->i_dev, ip->i_fs, dirbuf.dotdot_ino);
		if (ip == NULL) {
			error = u->u_error;
			break;
		}
	}

out:
	if (error == ENOTDIR)
		printf("checkpath: .. !dir\n");
	if (ip != NULL)
		iput(ip);
	return (error);
}

/*
 * Name cache initialization, from main() when we are booting
 */
void
nchinit()
{
	register union nchash *nchp;
	register struct namecache *ncp;
	//segm	seg5;

	//saveseg5(seg5);
	//mapseg5(nmidesc.se_addr,nmidesc.se_desc);
	nchhead = 0;
	nchtail = &nchhead;
	for (ncp = namecache; ncp < &namecache[nchsize]; ncp++) {
		ncp->nc_forw = ncp;			/* hash chain */
		ncp->nc_back = ncp;
		ncp->nc_nxt = NULL;			/* lru chain */
		*nchtail = ncp;
		ncp->nc_prev = nchtail;
		nchtail = &ncp->nc_nxt;
		/* all else is zero already */
	}
	for (nchp = nchash; nchp < &nchash[NCHHASH]; nchp++) {
		nchp->nch_head[0] = nchp;
		nchp->nch_head[1] = nchp;
	}
	//restorseg5(seg5);
}

/*
 * Cache flush, called when filesys is umounted to
 * remove entries that would now be invalid
 *
 * The line "nxtcp = nchhead" near the end is to avoid potential problems
 * if the cache lru chain is modified while we are dumping the
 * inode.  This makes the algorithm O(n^2), but do you think I care?
 */
void
nchinval(dev)
	register ufs211_dev_t dev;
{
	register struct namecache *ncp, *nxtcp;
	//segm	seg5;

	//saveseg5(seg5);
	//mapseg5(nmidesc.se_addr,nmidesc.se_desc);
	for (ncp = nchhead; ncp; ncp = nxtcp) {
		nxtcp = ncp->nc_nxt;
		if (ncp->nc_ip == NULL ||
		    (ncp->nc_idev != dev && ncp->nc_dev != dev))
			continue;
		/* free the resources we had */
		ncp->nc_idev = NODEV;
		ncp->nc_dev = NODEV;
		ncp->nc_id = NULL;
		ncp->nc_ino = 0;
		ncp->nc_ip = NULL;
		remque(ncp);		/* remove entry from its hash chain */
		ncp->nc_forw = ncp;	/* and make a dummy one */
		ncp->nc_back = ncp;
		/* delete this entry from LRU chain */
		*ncp->nc_prev = nxtcp;
		if (nxtcp)
			nxtcp->nc_prev = ncp->nc_prev;
		else
			nchtail = ncp->nc_prev;
		/* cause rescan of list, it may have altered */
		nxtcp = nchhead;
		/* put the now-free entry at head of LRU */
		ncp->nc_nxt = nxtcp;
		ncp->nc_prev = &nchhead;
		nxtcp->nc_prev = &ncp->nc_nxt;
		nchhead = ncp;
	}
	//restorseg5(seg5);
}

/*
 * Name cache invalidation of all entries.
 */
void
cacheinvalall()
{
	register struct namecache *ncp, *encp = &namecache[nchsize];
	segm	seg5;

	//saveseg5(seg5);
	//mapseg5(nmidesc.se_addr, nmidesc.se_desc);
	for (ncp = namecache; ncp < encp; ncp++)
		ncp->nc_id = 0;
	//restorseg5(seg5);
}
