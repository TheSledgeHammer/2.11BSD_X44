/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2001, 2002 Ian Dowse.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This implements a hash-based lookup scheme for UFS directories.
 */

#include <sys/cdefs.h>

//#ifdef UFS_DIRHASH

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/fnv_hash.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <devel/ufs/ufs/dirhash.h>
//#include <ufs/ufs/extattr.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#define UFS_DIRSIZ(fmt, dp)		DIRSIZ(fmt, dp)

#define	DIRHASH_DEFAULT_DIVIDER	64
#define	MIN_DEFAULT_DIRHASH_MEM	(2 * 1024 * 1024)
#define	MAX_DEFAULT_DIRHASH_MEM	(32 * 1024 * 1024)

#define WRAPINCR(val, limit)	(((val) + 1 == (limit)) ? 0 : ((val) + 1))
#define WRAPDECR(val, limit)	(((val) == 0) ? ((limit) - 1) : ((val) - 1))
#define OFSFMT(vp)				((vp)->v_mount->mnt_maxsymlinklen <= 0)
#define BLKFREE2IDX(n)			((n) > DH_NFSTATS ? DH_NFSTATS : (n))

#define M_DIRHASH 93

static u_int ufs_dirhashminblks = 5;
static u_int ufs_dirhashmaxmem = 0;
static u_int ufs_dirhashmem;
static u_int ufs_dirhashcheck = 0;

static int 		ufsdirhash_hash(struct dirhash *dh, char *name, int namelen);
static void 	ufsdirhash_adjfree(struct dirhash *dh, doff_t offset, int diff);
static void 	ufsdirhash_delslot(struct dirhash *dh, int slot);
static int 		ufsdirhash_findslot(struct dirhash *dh, char *name, int namelen, doff_t offset);
static doff_t 	ufsdirhash_getprev(struct direct *dp, doff_t offset);
static int 		ufsdirhash_recycle(int wanted);

#define DIRHASHLIST_LOCK() 			simple_lock(&ufsdirhash_lock)
#define DIRHASHLIST_UNLOCK() 		simple_unlock(&ufsdirhash_lock)
#define DIRHASH_LOCK(dh)			simple_lock(&(dh)->dh_lock)
#define DIRHASH_UNLOCK(dh)			simple_unlock(&(dh)->dh_lock)
#define	DIRHASH_ASSERT_LOCKED(dh)	KASSERT(&(dh)->dh_lock)

#define DIRHASH_BLKALLOC(dirhash)	malloc((dirhash), M_DIRHASH, M_NOWAIT)
#define DIRHASH_BLKFREE(dirhash) 	free((dirhash), M_DIRHASH)

/* Dirhash list; recently-used entries are near the tail. */
static TAILQ_HEAD(, dirhash) ufsdirhash_list;

/* Protects: ufsdirhash_list, `dh_list' field, ufs_dirhashmem. */
static struct lock_object	ufsdirhash_lock;

/*
 * Free any hash table associated with inode 'ip'.
 */
void
ufsdirhash_free(struct inode *ip)
{
	struct dirhash *dh;
	int i, mem;

	if ((dh = ip->i_dirhash) == NULL)
		return;

	ip->i_dirhash = NULL;

	DIRHASHLIST_LOCK();
	if (dh->dh_onlist)
		TAILQ_REMOVE(&ufsdirhash_list, dh, dh_list);
	DIRHASHLIST_UNLOCK();

	/* The dirhash pointed to by 'dh' is exclusively ours now. */
	mem = sizeof(*dh);
	if (dh->dh_hash != NULL) {
		for (i = 0; i < dh->dh_narrays; i++)
			DIRHASH_BLKFREE(dh->dh_hash[i]);
		free(dh->dh_hash, M_DIRHASH);
		if (dh->dh_blkfree != NULL) {
			free(dh->dh_blkfree, M_DIRHASH);
		}
		/*
		mem += dh->dh_hashsz;
		mem += dh->dh_narrays * DH_NBLKOFF * sizeof(**dh->dh_hash);
		mem += dh->dh_nblk * sizeof(*dh->dh_blkfree);
		*/
	}
	//mutex_destroy(&dh->dh_lock);
	pool_cache_put(ufsdirhash_cache, dh);

	atomic_add_int(&ufs_dirhashmem, -mem);
}

/*
 * Find the offset of the specified name within the given inode.
 * Returns 0 on success, ENOENT if the entry does not exist, or
 * EJUSTRETURN if the caller should revert to a linear search.
 *
 * If successful, the directory offset is stored in *offp, and a
 * pointer to a struct buf containing the entry is stored in *bpp. If
 * prevoffp is non-NULL, the offset of the previous entry within
 * the UFS_DIRBLKSIZ-sized block is stored in *prevoffp (if the entry
 * is the first in a block, the start of the block is used).
 */
int
ufsdirhash_lookup(struct inode *ip, const char *name, int namelen, doff_t *offp,
    struct buf **bpp, doff_t *prevoffp)
{
	struct dirhash *dh, *dh_next;
	struct direct *dp;
	struct vnode *vp;
	struct buf *bp;
	doff_t blkoff, bmask, offset, prevoff, seqoff;
	int i, slot;

	if ((dh = ip->i_dirhash) == NULL)
		return (EJUSTRETURN);

	/*
	 * Move this dirhash towards the end of the list if it has a
	 * score higher than the next entry, and acquire the dh_lock.
	 * Optimise the case where it's already the last by performing
	 * an unlocked read of the TAILQ_NEXT pointer.
	 *
	 * In both cases, end up holding just dh_lock.
	 */
	if (TAILQ_NEXT(dh, dh_list) != NULL) {
		DIRHASHLIST_LOCK();
		DIRHASH_LOCK(dh);
		/*
		 * If the new score will be greater than that of the next
		 * entry, then move this entry past it. With both mutexes
		 * held, dh_next won't go away, but its dh_score could
		 * change; that's not important since it is just a hint.
		 */
		if (dh->dh_hash != NULL &&
		    (dh_next = TAILQ_NEXT(dh, dh_list)) != NULL &&
		    dh->dh_score >= dh_next->dh_score) {
			KASSERT(dh->dh_onlist);
			TAILQ_REMOVE(&ufsdirhash_list, dh, dh_list);
			TAILQ_INSERT_AFTER(&ufsdirhash_list, dh_next, dh,
			    dh_list);
		}
		DIRHASHLIST_UNLOCK();
	} else {
		/* Already the last, though that could change as we wait. */
		DIRHASH_LOCK(dh);
	}
	if (dh->dh_hash == NULL) {
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return (EJUSTRETURN);
	}

	/* Update the score. */
	if (dh->dh_score < DH_SCOREMAX)
		dh->dh_score++;

	vp = ip->i_vnode;
	bmask = VFSTOUFS(vp->v_mount)->um_mountp->mnt_stat.f_iosize - 1;
	blkoff = -1;
	bp = NULL;
	seqoff = dh->dh_seqoff;
restart:
	slot = ufsdirhash_hash(dh, name, namelen);

	if (seqoff != -1) {
		/*
		 * Sequential access optimisation. dh_seqoff contains the
		 * offset of the directory entry immediately following
		 * the last entry that was looked up. Check if this offset
		 * appears in the hash chain for the name we are looking for.
		 */
		for (i = slot; (offset = DH_ENTRY(dh, i)) != DIRHASH_EMPTY;
		    i = WRAPINCR(i, dh->dh_hlen))
			if (offset == seqoff)
				break;
		if (offset == seqoff) {
			/*
			 * We found an entry with the expected offset. This
			 * is probably the entry we want, but if not, the
			 * code below will turn off seqoff and retry.
			 */
			slot = i;
		} else
			seqoff = 0;
	}

	for (; (offset = DH_ENTRY(dh, slot)) != DIRHASH_EMPTY;
	    slot = WRAPINCR(slot, dh->dh_hlen)) {
		if (offset == DIRHASH_DEL)
			continue;

		if (offset < 0 || offset >= ip->i_size)
			panic("ufsdirhash_lookup: bad offset in hash array");
		if ((offset & ~bmask) != blkoff) {
			if (bp != NULL)
				brelse(bp, 0);
			blkoff = offset & ~bmask;
			if (ufs_blkatoff(vp, (off_t)blkoff,
			    NULL, &bp, false) != 0) {
				DIRHASH_UNLOCK(dh);
				return (EJUSTRETURN);
			}
		}
		dp = (struct direct *)((char *)bp->b_data + (offset & bmask));
		if (dp->d_reclen == 0 || dp->d_reclen >
		    DIRBLKSIZ - (offset & (DIRBLKSIZ - 1))) {
			/* Corrupted directory. */
			DIRHASH_UNLOCK(dh);
			brelse(bp, 0);
			return (EJUSTRETURN);
		}
		if (dp->d_namlen == namelen &&
		    memcmp(dp->d_name, name, namelen) == 0) {
			/* Found. Get the prev offset if needed. */
			if (prevoffp != NULL) {
				if (offset & (DIRBLKSIZ - 1)) {
					prevoff = ufsdirhash_getprev(dp,
					    offset, DIRBLKSIZ);
					if (prevoff == -1) {
						brelse(bp, 0);
						return (EJUSTRETURN);
					}
				} else
					prevoff = offset;
				*prevoffp = prevoff;
			}

			/* Check for sequential access, and update offset. */
			if (seqoff == 0 && dh->dh_seqoff == offset)
				seqoff = 1;
			dh->dh_seqoff = offset + UFS_DIRSIZ(0, dp);
			DIRHASH_UNLOCK(dh);

			*bpp = bp;
			*offp = offset;
			return (0);
		}

		if (dh->dh_hash == NULL) {
			DIRHASH_UNLOCK(dh);
			if (bp != NULL)
				brelse(bp, 0);
			ufsdirhash_free(ip);
			return (EJUSTRETURN);
		}
		/*
		 * When the name doesn't match in the seqopt case, go back
		 * and search normally.
		 */
		if (seqoff != -1) {
			seqoff = -1;
			goto restart;
		}
	}
	DIRHASH_UNLOCK(dh);
	if (bp != NULL)
		brelse(bp, 0);
	return (ENOENT);
}

/*
 * Find a directory block with room for 'slotneeded' bytes. Returns
 * the offset of the directory entry that begins the free space.
 * This will either be the offset of an existing entry that has free
 * space at the end, or the offset of an entry with d_ino == 0 at
 * the start of a UFS_DIRBLKSIZ block.
 *
 * To use the space, the caller may need to compact existing entries in
 * the directory. The total number of bytes in all of the entries involved
 * in the compaction is stored in *slotsize. In other words, all of
 * the entries that must be compacted are exactly contained in the
 * region beginning at the returned offset and spanning *slotsize bytes.
 *
 * Returns -1 if no space was found, indicating that the directory
 * must be extended.
 */
doff_t
ufsdirhash_findfree(struct inode *ip, int slotneeded, int *slotsize)
{
	struct direct *dp;
	struct dirhash *dh;
	struct buf *bp;
	doff_t pos, slotstart;
	int dirblock, error, freebytes, i;

	if ((dh = ip->i_dirhash) == NULL)
		return (-1);

	DIRHASH_LOCK(dh);
	if (dh->dh_hash == NULL) {
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return (-1);
	}

	/* Find a directory block with the desired free space. */
	dirblock = -1;
	for (i = howmany(slotneeded, DIRALIGN); i <= DH_NFSTATS; i++)
		if ((dirblock = dh->dh_firstfree[i]) != -1)
			break;
	if (dirblock == -1) {
		DIRHASH_UNLOCK(dh);
		return (-1);
	}

	KASSERT(dirblock < dh->dh_nblk &&
	    dh->dh_blkfree[dirblock] >= howmany(slotneeded, DIRALIGN));
	pos = dirblock * DIRBLKSIZ;
	error = ufs_blkatoff(ip->i_vnode, (off_t)pos, (void *)&dp, &bp, false);
	if (error) {
		DIRHASH_UNLOCK(dh);
		return (-1);
	}
	/* Find the first entry with free space. */
	for (i = 0; i < DIRBLKSIZ; ) {
		if (dp->d_reclen == 0) {
			DIRHASH_UNLOCK(dh);
			brelse(bp, 0);
			return (-1);
		}
		if (dp->d_ino == 0 || dp->d_reclen > DIRSIZ(0, dp))
			break;
		i += dp->d_reclen;
		dp = (struct direct *)((char *)dp + dp->d_reclen);
	}
	if (i > DIRBLKSIZ) {
		DIRHASH_UNLOCK(dh);
		brelse(bp, 0);
		return (-1);
	}
	slotstart = pos + i;

	/* Find the range of entries needed to get enough space */
	freebytes = 0;
	while (i < DIRBLKSIZ && freebytes < slotneeded) {
		freebytes += dp->d_reclen;
		if (dp->d_ino != 0)
			freebytes -= UFS_DIRSIZ(0, dp);
		if (dp->d_reclen == 0) {
			DIRHASH_UNLOCK(dh);
			brelse(bp, 0);
			return (-1);
		}
		i += dp->d_reclen;
		dp = (struct direct *)((char *)dp + dp->d_reclen);
	}
	if (i > DIRBLKSIZ) {
		DIRHASH_UNLOCK(dh);
		brelse(bp, 0);
		return (-1);
	}
	if (freebytes < slotneeded)
		panic("ufsdirhash_findfree: free mismatch");
	DIRHASH_UNLOCK(dh);
	brelse(bp, 0);
	*slotsize = pos + i - slotstart;
	return (slotstart);
}

/*
 * Return the start of the unused space at the end of a directory, or
 * -1 if there are no trailing unused blocks.
 */
doff_t
ufsdirhash_enduseful(struct inode *ip)
{
	struct dirhash *dh;
	int i;

	if ((dh = ip->i_dirhash) == NULL)
		return (-1);

	DIRHASH_LOCK(dh);
	if (dh->dh_hash == NULL) {
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return (-1);
	}

	if (dh->dh_blkfree[dh->dh_dirblks - 1] != DIRBLKSIZ / DIRALIGN) {
		DIRHASH_UNLOCK(dh);
		return (-1);
	}

	for (i = dh->dh_dirblks - 1; i >= 0; i--)
		if (dh->dh_blkfree[i] != DIRBLKSIZ / DIRALIGN)
			break;
	DIRHASH_UNLOCK(dh);
	return ((doff_t)(i + 1) * DIRBLKSIZ);
}

/*
 * Insert information into the hash about a new directory entry. dirp
 * points to a struct direct containing the entry, and offset specifies
 * the offset of this entry.
 */
void
ufsdirhash_add(struct inode *ip, struct direct *dirp, doff_t offset)
{
	struct dirhash *dh;
	int slot;

	if ((dh = ip->i_dirhash) == NULL)
		return;

	DIRHASH_LOCK(dh);
	if (dh->dh_hash == NULL) {
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return;
	}

	KASSERT(offset < dh->dh_dirblks * DIRBLKSIZ);
	/*
	 * Normal hash usage is < 66%. If the usage gets too high then
	 * remove the hash entirely and let it be rebuilt later.
	 */
	if (dh->dh_hused >= (dh->dh_hlen * 3) / 4) {
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return;
	}

	/* Find a free hash slot (empty or deleted), and add the entry. */
	slot = ufsdirhash_hash(dh, dirp->d_name, dirp->d_namlen);
	while (DH_ENTRY(dh, slot) >= 0)
		slot = WRAPINCR(slot, dh->dh_hlen);
	if (DH_ENTRY(dh, slot) == DIRHASH_EMPTY)
		dh->dh_hused++;
	DH_ENTRY(dh, slot) = offset;

	/* Update the per-block summary info. */
	ufsdirhash_adjfree(dh, offset, -DIRSIZ(0, dirp), DIRBLKSIZ);
	DIRHASH_UNLOCK(dh);
}

/*
 * Remove the specified directory entry from the hash. The entry to remove
 * is defined by the name in `dirp', which must exist at the specified
 * `offset' within the directory.
 */
void
ufsdirhash_remove(struct inode *ip, struct direct *dirp, doff_t offset)
{
	struct dirhash *dh;
	int slot;

	if ((dh = ip->i_dirhash) == NULL)
		return;

	DIRHASH_LOCK(dh);
	if (dh->dh_hash == NULL) {
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return;
	}

	KASSERT(offset < dh->dh_dirblks * DIRBLKSIZ);
	/* Find the entry */
	slot = ufsdirhash_findslot(dh, dirp->d_name, dirp->d_namlen, offset);

	/* Remove the hash entry. */
	ufsdirhash_delslot(dh, slot);

	/* Update the per-block summary info. */
	ufsdirhash_adjfree(dh, offset, DIRSIZ(0, dirp), DIRBLKSIZ);
	DIRHASH_UNLOCK(dh);
}

/*
 * Change the offset associated with a directory entry in the hash. Used
 * when compacting directory blocks.
 */
void
ufsdirhash_move(struct inode *ip, struct direct *dirp, doff_t oldoff, doff_t newoff)
{
	struct dirhash *dh;
	int slot;

	if ((dh = ip->i_dirhash) == NULL)
		return;
	DIRHASH_LOCK(dh);
	if (dh->dh_hash == NULL) {
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return;
	}

	KASSERT(oldoff < dh->dh_dirblks * DIRBLKSIZ && newoff < dh->dh_dirblks * ip->i_ump->um_dirblksiz);
	/* Find the entry, and update the offset. */
	slot = ufsdirhash_findslot(dh, dirp->d_name, dirp->d_namlen, oldoff);
	DH_ENTRY(dh, slot) = newoff;
	DIRHASH_UNLOCK(dh);
}

/*
 * Inform dirhash that the directory has grown by one block that
 * begins at offset (i.e. the new length is offset + UFS_DIRBLKSIZ).
 */
void
ufsdirhash_newblk(struct inode *ip, doff_t offset)
{
	struct dirhash *dh;
	int block;

	if ((dh = ip->i_dirhash) == NULL)
		return;
	DIRHASH_LOCK(dh);
	if (dh->dh_hash == NULL) {
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return;
	}

	KASSERT(offset == dh->dh_dirblks * DIRBLKSIZ);
	block = offset / DIRBLKSIZ;
	if (block >= dh->dh_nblk) {
		/* Out of space; must rebuild. */
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return;
	}
	dh->dh_dirblks = block + 1;

	/* Account for the new free block. */
	dh->dh_blkfree[block] = DIRBLKSIZ / DIRALIGN;
	if (dh->dh_firstfree[DH_NFSTATS] == -1)
		dh->dh_firstfree[DH_NFSTATS] = block;
	DIRHASH_UNLOCK(dh);
}

/*
 * Inform dirhash that the directory is being truncated.
 */
void
ufsdirhash_dirtrunc(struct inode *ip, doff_t offset)
{
	struct dirhash *dh;
	int block, i;

	if ((dh = ip->i_dirhash) == NULL)
		return;

	DIRHASH_LOCK(dh);
	if (dh->dh_hash == NULL) {
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return;
	}

	KASSERT(offset <= dh->dh_dirblks * DIRBLKSIZ);
	block = howmany(offset, DIRBLKSIZ);
	/*
	 * If the directory shrinks to less than 1/8 of dh_nblk blocks
	 * (about 20% of its original size due to the 50% extra added in
	 * ufsdirhash_build) then free it, and let the caller rebuild
	 * if necessary.
	 */
	if (block < dh->dh_nblk / 8 && dh->dh_narrays > 1) {
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return;
	}

	/*
	 * Remove any `first free' information pertaining to the
	 * truncated blocks. All blocks we're removing should be
	 * completely unused.
	 */
	if (dh->dh_firstfree[DH_NFSTATS] >= block)
		dh->dh_firstfree[DH_NFSTATS] = -1;
	for (i = block; i < dh->dh_dirblks; i++)
		if (dh->dh_blkfree[i] != DIRBLKSIZ / DIRALIGN)
			panic("ufsdirhash_dirtrunc: blocks in use");
	for (i = 0; i < DH_NFSTATS; i++)
		if (dh->dh_firstfree[i] >= block)
			panic("ufsdirhash_dirtrunc: first free corrupt");
	dh->dh_dirblks = block;
	DIRHASH_UNLOCK(dh);
}

/*
 * Debugging function to check that the dirhash information about
 * a directory block matches its actual contents. Panics if a mismatch
 * is detected.
 *
 * On entry, `sbuf' should point to the start of an in-core
 * DIRBLKSIZ-sized directory block, and `offset' should contain the
 * offset from the start of the directory of that block.
 */
void
ufsdirhash_checkblock(struct inode *ip, char *sbuf, doff_t offset)
{
	struct dirhash *dh;
	struct direct *dp;
	int block, ffslot, i, nfree;

	if (!ufs_dirhashcheck)
		return;
	if ((dh = ip->i_dirhash) == NULL)
		return;

	DIRHASH_LOCK(dh);
	if (dh->dh_hash == NULL) {
		DIRHASH_UNLOCK(dh);
		ufsdirhash_free(ip);
		return;
	}

	block = offset / DIRBLKSIZ;
	if ((offset & (DIRBLKSIZ - 1)) != 0 || block >= dh->dh_dirblks)
		panic("ufsdirhash_checkblock: bad offset");

	nfree = 0;
	for (i = 0; i < DIRBLKSIZ; i += dp->d_reclen) {
		dp = (struct direct *)(sbuf + i);
		if (dp->d_reclen == 0 || i + dp->d_reclen > DIRBLKSIZ)
			panic("ufsdirhash_checkblock: bad dir");

		if (dp->d_ino == 0) {
#if 0
			/*
			 * XXX entries with d_ino == 0 should only occur
			 * at the start of a DIRBLKSIZ block. However the
			 * ufs code is tolerant of such entries at other
			 * offsets, and fsck does not fix them.
			 */
			if (i != 0)
				panic("ufsdirhash_checkblock: bad dir inode");
#endif
			nfree += dp->d_reclen;
			continue;
		}

		/* Check that the entry	exists (will panic if it doesn't). */
		ufsdirhash_findslot(dh, dp->d_name, dp->d_namlen, offset + i);

		nfree += dp->d_reclen - DIRSIZ(0, dp);
	}
	if (i != DIRBLKSIZ)
		panic("ufsdirhash_checkblock: bad dir end");

	if (dh->dh_blkfree[block] * DIRALIGN != nfree)
		panic("ufsdirhash_checkblock: bad free count");

	ffslot = BLKFREE2IDX(nfree / DIRALIGN);
	for (i = 0; i <= DH_NFSTATS; i++)
		if (dh->dh_firstfree[i] == block && i != ffslot)
			panic("ufsdirhash_checkblock: bad first-free");
	if (dh->dh_firstfree[ffslot] == -1)
		panic("ufsdirhash_checkblock: missing first-free entry");
	DIRHASH_UNLOCK(dh);
}

/*
 * Hash the specified filename into a dirhash slot.
 */
static int
ufsdirhash_hash(struct dirhash *dh, char *name, int namelen)
{
	u_int32_t hash;

	/*
	 * We hash the name and then some other bit of data that is
	 * invariant over the dirhash's lifetime. Otherwise names
	 * differing only in the last byte are placed close to one
	 * another in the table, which is bad for linear probing.
	 */
	hash = fnv_32_buf(name, namelen, FNV1_32_INIT);
	hash = fnv_32_buf(&dh, sizeof(dh), hash);
	return (hash % dh->dh_hlen);
}

/*
 * Adjust the number of free bytes in the block containing `offset'
 * by the value specified by `diff'.
 *
 * The caller must ensure we have exclusive access to `dh'; normally
 * that means that dh_lock should be held, but this is also called
 * from ufsdirhash_build() where exclusive access can be assumed.
 */
static void
ufsdirhash_adjfree(struct dirhash *dh, doff_t offset, int diff)
{
	int block, i, nfidx, ofidx;

	/* Update the per-block summary info. */
	block = offset / DIRBLKSIZ;
	KASSERT(block < dh->dh_nblk && block < dh->dh_dirblks ("dirhash bad offset"));
	ofidx = BLKFREE2IDX(dh->dh_blkfree[block]);
	dh->dh_blkfree[block] = (int)dh->dh_blkfree[block] + (diff / DIRALIGN);
	nfidx = BLKFREE2IDX(dh->dh_blkfree[block]);

	/* Update the `first free' list if necessary. */
	if (ofidx != nfidx) {
		/* If removing, scan forward for the next block. */
		if (dh->dh_firstfree[ofidx] == block) {
			for (i = block + 1; i < dh->dh_dirblks; i++)
				if (BLKFREE2IDX(dh->dh_blkfree[i]) == ofidx)
					break;
			dh->dh_firstfree[ofidx] = (i < dh->dh_dirblks) ? i : -1;
		}

		/* Make this the new `first free' if necessary */
		if (dh->dh_firstfree[nfidx] > block ||
		    dh->dh_firstfree[nfidx] == -1) {
			dh->dh_firstfree[nfidx] = block;
		}
	}
}

/*
 * Find the specified name which should have the specified offset.
 * Returns a slot number, and panics on failure.
 *
 * `dh' must be locked on entry and remains so on return.
 */
static int
ufsdirhash_findslot(struct dirhash *dh, char *name, int namelen, doff_t offset)
{
	int slot;

	DIRHASH_ASSERT_LOCKED(dh);

	/* Find the entry. */
	KASSERT(dh->dh_hused < dh->dh_hlen, ("dirhash find full"));
	slot = ufsdirhash_hash(dh, name, namelen);
	while (DH_ENTRY(dh, slot) != offset &&
	    DH_ENTRY(dh, slot) != DIRHASH_EMPTY)
		slot = WRAPINCR(slot, dh->dh_hlen);
	if (DH_ENTRY(dh, slot) != offset)
		panic("ufsdirhash_findslot: '%.*s' not found", namelen, name);

	return (slot);
}

/*
 * Remove the entry corresponding to the specified slot from the hash array.
 *
 * `dh' must be locked on entry and remains so on return.
 */
static void
ufsdirhash_delslot(struct dirhash *dh, int slot)
{
	int i;

	DIRHASH_ASSERT_LOCKED(dh);

	/* Mark the entry as deleted. */
	DH_ENTRY(dh, slot) = DIRHASH_DEL;

	/* If this is the end of a chain of DIRHASH_DEL slots, remove them. */
	for (i = slot; DH_ENTRY(dh, i) == DIRHASH_DEL;)
		i = WRAPINCR(i, dh->dh_hlen);
	if (DH_ENTRY(dh, i) == DIRHASH_EMPTY) {
		i = WRAPDECR(i, dh->dh_hlen);
		while (DH_ENTRY(dh, i) == DIRHASH_DEL) {
			DH_ENTRY(dh, i) = DIRHASH_EMPTY;
			dh->dh_hused--;
			i = WRAPDECR(i, dh->dh_hlen);
		}
		KASSERT(dh->dh_hused >= 0 ("ufsdirhash_delslot neg hlen"));
	}
}

/*
 * Given a directory entry and its offset, find the offset of the
 * previous entry in the same DIRBLKSIZ-sized block. Returns an
 * offset, or -1 if there is no previous entry in the block or some
 * other problem occurred.
 */
static doff_t
ufsdirhash_getprev(struct direct *dirp, doff_t offset)
{
	struct direct *dp;
	char *blkbuf;
	doff_t blkoff, prevoff;
	int entrypos, i;

	blkoff = rounddown2(offset, DIRBLKSIZ);	/* offset of start of block */
	entrypos = offset & (DIRBLKSIZ - 1);	/* entry relative to block */
	blkbuf = (char *)dirp - entrypos;
	prevoff = blkoff;

	/* If `offset' is the start of a block, there is no previous entry. */
	if (entrypos == 0)
		return (-1);

	/* Scan from the start of the block until we get to the entry. */
	for (i = 0; i < entrypos; i += dp->d_reclen) {
		dp = (struct direct *)(blkbuf + i);
		if (dp->d_reclen == 0 || i + dp->d_reclen > entrypos)
			return (-1);	/* Corrupted directory. */
		prevoff = blkoff + i;
	}
	return (prevoff);
}

/*
 * Try to free up `wanted' bytes by stealing memory from existing
 * dirhashes. Returns zero with list locked if successful.
 */
static int
ufsdirhash_recycle(int wanted)
{
	struct dirhash *dh;
	doff_t **hash;
	u_int8_t *blkfree;
	int i, mem, narrays;

	DIRHASHLIST_LOCK();
	dh = TAILQ_FIRST(&ufsdirhash_list);
	while (wanted + ufs_dirhashmem > ufs_dirhashmaxmem) {
		/* Decrement the score; only recycle if it becomes zero. */
		if (dh == NULL || --dh->dh_score > 0) {
			DIRHASHLIST_UNLOCK();
			return (-1);
		}

		DIRHASH_LOCK(dh);
		KASSERT(dh->dh_hash != NULL);

		/* Remove it from the list and detach its memory. */
		TAILQ_REMOVE(&ufsdirhash_list, dh, dh_list);
		dh->dh_onlist = 0;
		hash = dh->dh_hash;
		dh->dh_hash = NULL;
		blkfree = dh->dh_blkfree;
		dh->dh_blkfree = NULL;
		narrays = dh->dh_narrays;
		mem = narrays * sizeof(*dh->dh_hash) +
		    narrays * DH_NBLKOFF * sizeof(**dh->dh_hash) +
		    dh->dh_nblk * sizeof(*dh->dh_blkfree);
		dh->dh_memreq = 0;

		/* Unlock dirhash and free the detached memory. */
		DIRHASH_UNLOCK(dh);
		DIRHASHLIST_UNLOCK();

		for (i = 0; i < narrays; i++) {
			DIRHASH_BLKFREE(hash[i]);
		}
		free(hash, M_DIRHASH);
		free(blkfree, M_DIRHASH);

		/* Account for the returned memory. */
		DIRHASHLIST_LOCK();
		ufs_dirhashmem -= mem;
	}
	/* Success; return with list locked. */
	return (0);
}

void
ufsdirhash_init(void)
{

	/*
	 * Only initialise defaults for the dirhash size if it hasn't
	 * hasn't been set.
	 */
	if (ufs_dirhashmaxmem == 0) {
		/* Use 64-bit math to avoid overflows. */
		uint64_t physmem_bytes, hash_bytes;

		physmem_bytes = ctob((uint64_t)physmem);
		hash_bytes = physmem_bytes / DIRHASH_DEFAULT_DIVIDER;

		if (hash_bytes < MIN_DEFAULT_DIRHASH_MEM)
			hash_bytes = 0;

		if (hash_bytes > MAX_DEFAULT_DIRHASH_MEM)
			hash_bytes = MAX_DEFAULT_DIRHASH_MEM;

		ufs_dirhashmaxmem = (u_int)hash_bytes;
	}

	simple_lock_init(&ufsdirhash_lock, "ufsdirhash_lock");
	TAILQ_INIT(&ufsdirhash_list);
}

void
ufsdirhash_done(void)
{
	KASSERT(TAILQ_EMPTY(&ufsdirhash_list));
}
