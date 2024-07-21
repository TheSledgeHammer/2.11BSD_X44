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
#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>

#include <ufs/efs/efs.h>
#include <ufs/efs/efs_extent.h>

struct efs_dirblk {
	uint8_t		db_slots;
	uint8_t		db_space[EFS_DIRBLK_SPACE_SIZE];
};

/*
 * Given an efs_dirblk structure and a componentname to search for, return the
 * corresponding inode if it is found.
 *
 * Returns 0 on success.
 */
static int
efs_dirblk_lookup(dir, cn, inode)
	struct efs_dirblk *dir;
	struct componentname *cn;
	ino_t *inode;
{
	struct direct *ep;
	int i;

	for (i = 0; i < dir->db_slots; i++) {
		if (ep->d_namlen == cn->cn_namelen && (strncmp(cn->cn_nameptr, ep->d_name, cn->cn_namelen) == 0)) {

		}
	}
	*inode = be32toh(ep->d_number);
	return (0);
}

/*
 * Given an extent descriptor that represents a directory, look up
 * componentname within its efs_dirblk's. If it is found, return the
 * corresponding inode in 'ino'.
 *
 * Returns 0 on success.
 */
static int
efs_extent_lookup(ump, ex, cn, ino)
	struct ufsmount *ump;
	struct efs_extent *ex;
	struct componentname *cn;
	ino_t *ino;
{
	struct efs_dirblk *db;
	struct buf *bp;
	int i, err;

	for (i = 0; i < ex->ex_length; i++) {
		err = efs_bread(ump, ex->ex_bn + i, NULL, &bp);
		if (err) {
			printf("efs: warning: invalid extent descriptor\n");
			return (err);
		}

		db = (struct efs_dirblk *)bp->b_data;
		if (efs_dirblk_lookup(db, cn, ino) == 0) {
			brelse(bp, 0);
			return (0);
		}
		brelse(bp, 0);
	}

	return (ENOENT);
}

/*
 * Given the provided in-core inode, look up the pathname requested. If
 * we find it, 'ino' reflects its corresponding on-disk inode number.
 *
 * Returns 0 on success.
 */
int
efs_inode_lookup(ump, ip, cn, ino)
	struct ufsmount *ump;
	struct inode *ip;
	struct componentname *cn;
	ino_t *ino;
{
	struct efs_extent *ex;
	struct efs_extent_iterator exi;
	int ret, error;

	KASSERT(VOP_ISLOCKED(ip->i_vnode));

	KASSERT((ip->i_mode & S_IFMT) == S_IFDIR);

	efs_extent_iterator_init(&exi, ip, 0);
	ret = efs_extent_iterator_next(&exi, ex, ip);
	while (ret == 0) {
		if (efs_extent_lookup(ump, ex, ino) == 0) {
			return (0);
		}
	}
	error = ((ret == -1) ? ENOENT : ret);
	return (error);
}

void
efs_extent_to_bitmap(ex, eb)
	struct efs_extent *ex;
	struct efs_bitmap *eb;
{
	KASSERT(ex != NULL && eb != NULL);
	KASSERT(ex->ex_magic == EFS_EXTENT_MAGIC);
	KASSERT((ex->ex_bn & ~EFS_EXTENT_BN_MASK) == 0);
	KASSERT((ex->ex_offset & ~EFS_EXTENT_OFFSET_MASK) == 0);

	eb->eb_words[0] = htobe32(ex->ex_bn);
	eb->eb_bytes[0] = ex->ex_magic;
	eb->eb_words[1] = htobe32(ex->ex_offset);
	eb->eb_bytes[4] = ex->ex_length;
}

void
efs_bitmap_to_extent(eb, ex)
	struct efs_bitmap *eb;
	struct efs_extent *ex;
{
	KASSERT(eb != NULL && ex != NULL);

	ex->ex_magic = eb->eb_bytes[0];
	ex->ex_bn = be32toh(eb->eb_words[0]) & 0x00ffffff;
	ex->ex_length = eb->eb_bytes[4];
	ex->ex_offset = be32toh(eb->eb_words[1]) & 0x00ffffff;
}

/*
 * Initialise an extent iterator.
 *
 * If start_hint is non-0, attempt to set up the iterator beginning with the
 * extent descriptor in which the start_hint'th byte exists. Callers must not
 * expect success (this is simply an optimisation), so we reserve the right
 * to start from the beginning.
 */
void
extent_iterator_init(struct efs_extent_iterator *exi, struct inode *ip, off_t start_hint)
{
	struct efs *fs;
	struct efs_extent ex, ex2;
	struct buf *bp;
	struct ufsmount *ump;
	struct vnode *vp;
	off_t offset, length, next;
	int i, err, numextent, numinextents;
	int hi, lo, mid;
	int indir;

	vp = ITOV(ip);
	ump = VFSTOUFS(vp->v_mount);
	fs = ip->i_efs;

	exi->exi_efs = fs;
	exi->exi_logical = 0;
	exi->exi_direct = 0;
	exi->exi_indirect = 0;

	if (start_hint == 0)
		return;

	/* force iterator to end if hint is too big */
	if (start_hint >= DIP(ip, size)) {
		exi->exi_logical = DIP(ip, numextents);
		return;
	}

	/*
	 * Use start_hint to jump to the right extent descriptor. We'll
	 * iterate over the 12 indirect extents because it's cheap, then
	 * bring the appropriate vector into core and binary search it.
	 */

	/*
	 * Handle the small file case separately first...
	 */

	if (DIP(ip, numextents) < EFS_DIRECTEXTENTS) {
		for (i = 0; i < DIP(ip, numextents); i++) {
			efs_bitmap_to_extent(&fs->efs_extents[i], &ex);
			offset = ex.ex_offset * EFS_BB_SIZE;
			length = ex.ex_length * EFS_BB_SIZE;

			if (start_hint >= offset && start_hint < (offset + length)) {
				exi->exi_logical = exi->exi_direct = i;
				return;
			}
		}
		/* shouldn't get here, no? */
		EFS_DPRINTF(("efs_extent_iterator_init: bad direct extents\n"));
		return;
	}

	/*
	 * Now do the large files with indirect extents...
	 *
	 * The first indirect extent's ex_offset field contains the
	 * number of indirect extents used.
	 */
	efs_bitmap_to_extent(&fs->efs_extents[0], &ex);

	numinextents = ex.ex_offset;
	if (numinextents < 1 || numinextents >= EFS_DIRECTEXTENTS) {
		EFS_DPRINTF(("efs_extent_iterator_init: bad ex.ex_offset\n"));
		return;
	}

	next = 0;
	indir = -1;
	numextent = 0;
	for (i = 0; i < numinextents; i++) {
		efs_bitmap_to_extent(&fs->efs_extents[i], &ex);

		err = efs_bread(ump, ex.ex_bn, NULL, &bp);
		if (err) {
			return;
		}

		efs_bitmap_to_extent((struct efs_bitmap *)bp->b_data, &ex2);
		brelse(bp, 0);

		offset = ex2.ex_offset * EFS_BB_SIZE;

		if (offset > start_hint) {
			indir = MAX(0, i - 1);
			break;
		}

		/* number of extents prior to this indirect vector of extents */
		next += numextent;

		/* number of extents within this indirect vector of extents */
		numextent = ex.ex_length * EFS_EXTENTS_PER_BB;
		numextent = MIN(numextent, DIP(ip, numextents) - next);
	}

	/*
	 * We hit the end, so assume it's in the last extent.
	 */
	if (indir == -1)
		indir = numinextents - 1;

	/*
	 * Binary search to find our desired direct extent.
	 */
	lo = 0;
	mid = 0;
	hi = numextent - 1;
	efs_bitmap_to_extent(&fs->efs_extents[indir], &ex);
	while (lo <= hi) {
		int bboff;
		int index;

		mid = (lo + hi) / 2;

		bboff = mid / EFS_EXTENTS_PER_BB;
		index = mid % EFS_EXTENTS_PER_BB;

		err = efs_bread(ump, ex.ex_bn + bboff, NULL, &bp);
		if (err) {
			EFS_DPRINTF(("efs_extent_iterator_init: bsrch read\n"));
			return;
		}

		efs_bitmap_to_extent((struct efs_bitmap *)bp->b_data + index, &ex2);
		brelse(bp, 0);

		offset = ex2.ex_offset * EFS_BB_SIZE;
		length = ex2.ex_length * EFS_BB_SIZE;

		if (start_hint >= offset && start_hint < (offset + length))
			break;

		if (start_hint < offset)
			hi = mid - 1;
		else
			lo = mid + 1;
	}

	/*
	 * This is bad. Either the hint is bogus (which shouldn't
	 * happen) or the extent list must be screwed up. We
	 * have to abort.
	 */
	if (lo > hi) {
		EFS_DPRINTF(("efs_extent_iterator_init: bsearch "
		    "failed to find extent\n"));
		return;
	}

	exi->exi_logical	= next + mid;
	exi->exi_direct		= indir;
	exi->exi_indirect	= mid;
}

int
efs_extent_iterator_next(struct efs_extent_iterator *exi, struct efs_extent *exp, struct inode *ip)
{
	struct efs_extent ex;
	struct efs_bitmap *ebp;
	struct efs *fs;
	struct buf *bp;
	struct ufsmount *ump;
	struct vnode *vp;
	int err, bboff, index;

	vp = ITOV(ip);
	ump = VFSTOUFS(vp->v_mount);
	fs = ip->i_efs;

	if (exi->exi_logical++ >= DIP(ip, numextents)) {
		return (-1);
	}

	/* direct or indirect extents? */
	if (DIP(ip, numextents) <= EFS_DIRECTEXTENTS) {
		if (exp != NULL) {
			ebp = fs->efs_extents[exi->exi_direct++];
			efs_bitmap_to_extent(ebp, &ex);
		}
	} else {
		efs_bitmap_to_extent(fs->efs_extents[exi->exi_direct], &ex);
		bboff	= exi->exi_indirect / EFS_EXTENTS_PER_BB;
		index	= exi->exi_indirect % EFS_EXTENTS_PER_BB;

		err = efs_bread(ump, ex.ex_bn + bboff, NULL, &bp);
		if (err) {
			EFS_DPRINTF(("efs_extent_iterator_next: "
			    "efs_bread failed: %d\n", err));
			return (err);
		}

		if (exp != NULL) {
			ebp = (struct efs_bitmap *)bp->b_data + index;
			efs_bitmap_to_extent(ebp, exp);
		}
		brelse(bp, 0);

		bboff = exi->exi_indirect++ / EFS_EXTENTS_PER_BB;
		if (bboff >= ex.ex_length) {
			exi->exi_indirect = 0;
			exi->exi_direct++;
		}
	}

	return (0);
}
