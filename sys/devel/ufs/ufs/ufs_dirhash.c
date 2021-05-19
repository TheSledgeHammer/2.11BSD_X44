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

static int 		ufsdirhash_hash(struct dirhash *dh, char *name, int namelen);
static void 	ufsdirhash_adjfree(struct dirhash *dh, doff_t offset, int diff);
static void 	ufsdirhash_delslot(struct dirhash *dh, int slot);
static int 		ufsdirhash_findslot(struct dirhash *dh, char *name, int namelen, doff_t offset);
static doff_t 	ufsdirhash_getprev(struct direct *dp, doff_t offset);
static int 		ufsdirhash_recycle(int wanted);
static void 	ufsdirhash_lowmem(void);
static void 	ufsdirhash_free_locked(struct inode *ip);

#define WRAPINCR(val, limit)	(((val) + 1 == (limit)) ? 0 : ((val) + 1))
#define WRAPDECR(val, limit)	(((val) == 0) ? ((limit) - 1) : ((val) - 1))
#define OFSFMT(vp)				((vp)->v_mount->mnt_maxsymlinklen <= 0)
#define BLKFREE2IDX(n)			((n) > DH_NFSTATS ? DH_NFSTATS : (n))

/* Dirhash list; recently-used entries are near the tail. */
static TAILQ_HEAD(, dirhash) ufsdirhash_list;

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
		    dh->dh_firstfree[nfidx] == -1)
			dh->dh_firstfree[nfidx] = block;
	}
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
