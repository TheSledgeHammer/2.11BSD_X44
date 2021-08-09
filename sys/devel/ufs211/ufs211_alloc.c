/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_alloc.c	1.3 (2.11BSD GTE) 1996/9/19
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include "devel/ufs211/ufs211_dir.h"
#include "devel/ufs211/ufs211_fs.h"
#include "devel/ufs211/ufs211_inode.h"
#include "devel/ufs211/ufs211_quota.h"

typedef	struct ufs211_fblk *FBLKP;

/*
 * Allocate a block in the file system.
 *
 * alloc will obtain the next available free disk block from the
 * free list of the specified device.  The super block has up to
 * NICFREE remembered free blocks; the last of these is read to
 * obtain NICFREE more...
 */
struct buf *
balloc(ip, flags)
	struct ufs211_inode *ip;
	int flags;
{
	register struct ufs211_fs *fs;
	register struct buf *bp;
	int async;
	daddr_t bno;

	fs = ip->i_fs;
	async = fs->fs_flags & MNT_ASYNC;

	while (fs->fs_flock)
		sleep((caddr_t) &fs->fs_flock, PINOD);
	do {
		if (fs->fs_nfree <= 0)
			goto nospace;
		if (fs->fs_nfree > UFS211_NICFREE) {
			fserr(fs, "bad free count");
			goto nospace;
		}
		bno = fs->fs_free[--fs->fs_nfree];
		if (bno == 0)
			goto nospace;
	} while (badblock(fs, bno));
	if (fs->fs_nfree <= 0) {
		fs->fs_flock++;
		bp = bread(ip->i_dev, bno);
		if (((bp->b_flags & B_ERROR) == 0) && (bp->b_resid == 0)) {
			register struct ufs211_fblk *fbp;
			ufs211_mapin(bp);
			fbp = (FBLKP) bp;
			*((FBLKP) &fs->fs_nfree) = *fbp;
			ufs211_mapout(bp);
		}
		brelse(bp);
		/*
		 * Write the superblock back, synchronously if requested,
		 * so that the free list pointer won't point at garbage.
		 * We can still end up with dups in free if we then
		 * use some of the blocks in this freeblock, then crash
		 * without a sync.
		 */
		bp = getblk(ip->i_dev, UFS211_SUPERB);
		fs->fs_fmod = 0;
		fs->fs_time = time.tv_sec;
		{
			register struct ufs211_fs *fps;

			ufs211_mapin(bp);
			fps = (struct ufs211_fs *)bp;
			*fps = *fs;
		}
		ufs211_mapout(bp);
		if (!async)
			bwrite(bp);
		else
			bdwrite(bp);
		fs->fs_flock = 0;
		wakeup((caddr_t) &fs->fs_flock);
		if (fs->fs_nfree <= 0)
			goto nospace;
	}
	bp = getblk(ip->i_dev, bno);
	bp->b_resid = 0;
	if (flags & B_CLRBUF)
		clrbuf(bp);
	fs->fs_fmod = 1;
	fs->fs_tfree--;
	return (bp);

nospace:
	fs->fs_nfree = 0;
	fs->fs_tfree = 0;
	fserr(fs, "file system full");
	/*
	 * THIS IS A KLUDGE...
	 * SHOULD RATHER SEND A SIGNAL AND SUSPEND THE PROCESS IN A
	 * STATE FROM WHICH THE SYSTEM CALL WILL RESTART
	 */
	uprintf("\n%s: write failed, file system full\n", fs->fs_fsmnt);
	{
		register int i;

		for (i = 0; i < 5; i++)
			sleep((caddr_t) &lbolt, PRIBIO);
	}
	u->u_error = ENOSPC;
	return (NULL);
}

/*
 * Free a block or fragment.
 *
 * Place the specified disk block back on the free list of the
 * specified device.
 */
free(ip, bno)
	struct ufs211_inode *ip;
	daddr_t bno;
{
	register struct ufs211_fs *fs;
	register struct buf *bp;
	struct ufs211_fblk *fbp;

	fs = ip->i_fs;
	if (badblock(fs, bno)) {
		printf("bad block %D, ino %d\n", bno, ip->i_number);
		return;
	}
	while (fs->fs_flock)
		sleep((caddr_t)&fs->fs_flock, PINOD);
	if (fs->fs_nfree <= 0) {
		fs->fs_nfree = 1;
		fs->fs_free[0] = 0;
	}
	if (fs->fs_nfree >= UFS211_NICFREE) {
		fs->fs_flock++;
		bp = getblk(ip->i_dev, bno);
		ufs211_mapin(bp);
		fbp = (FBLKP) bp;
		*fbp = *((FBLKP)&fs->fs_nfree);
		ufs211_mapout(bp);
		fs->fs_nfree = 0;
		if (fs->fs_flags & MNT_ASYNC)
			bdwrite(bp);
		else
			bwrite(bp);
		fs->fs_flock = 0;
		wakeup((caddr_t)&fs->fs_flock);
	}
	fs->fs_free[fs->fs_nfree++] = bno;
	fs->fs_tfree++;
	fs->fs_fmod = 1;
}

/*
 * Fserr prints the name of a file system with an error diagnostic.
 *
 * The form of the error message is:
 *	fs: error message
 */
void
fserr(fp, cp)
	struct ufs211_fs *fp;
	char *cp;
{
	printf("%s: %s\n", fp->fs_fsmnt, cp);
}
