/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)vfs_htree.c	1.00
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/buf.h>

#include <miscfs/specfs/specdev.h>

#include <htbc.h>

#define	KDASSERT(x) 		assert(x)
#define	KASSERT(x) 			assert(x)
#define	htbc_alloc(s) 		malloc(s)
#define	htbc_free(a, s) 	free(a)
#define	htbc_calloc(n, s) 	calloc((n), (s))

#define htbc_lock()			simplelock()
#define htbc_unlock() 		simpleunlock()

struct htbc_dealloc {
	TAILQ_ENTRY(htbc_dealloc) 	hd_entries;
	daddr_t 					hd_blkno;	/* address of block */
	int 						hd_len;		/* size of block */
};

TAILQ_HEAD(htchain, htbc);
struct htbc {
	struct htchain				hc_head;
	TAILQ_ENTRY(htbc) 			hc_entry;
	struct vnode 				*hc_devvp;
	struct mount 				*hc_mount;
};

static struct htbc_dealloc 		*htbc_dealloc;

void
htbc_init()
{
	&htbc_dealloc = (struct htbc_dealloc *) malloc(sizeof(struct htbc_dealloc), M_HTBC, NULL);
}

void
htbc_fini()
{
	FREE(&htbc_dealloc, M_HTBC);
}

int
htbc_start()
{

}

int
htbc_stop()
{

}

int
htbc_read()
{

}

int
htbc_write()
{

}

/****************************************************************/

void
htbc_register_inode(struct htbc *ht, ino_t ino, mode_t mode)
{
	struct htbc_ino_head *hih;
	struct htbc_ino *hi;

}

void
htbc_unregister_inode(struct htbc *ht, ino_t ino, mode_t mode)
{
	struct htbc_ino *hi;

}

/****************************************************************/

/*
 * Return buffer with the contents of block "offset" from the beginning of
 * directory "ip".  If "res" is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
int
htbc_blkatoff(struct vnode *vp, off_t offset, char **res, struct buf **bpp)
{
	struct htbc_inode 	*ip;
	struct htbc_hi_mfs 	*fs;
	struct buf *bp;
	daddr_t lbn;
	int error;

	ip = VTOI(vp);
	fs = ip->hi_mfs;
	lbn = htbc_lblkno(fs, offset);

	*bpp = NULL;
	if ((error = bread(vp, lbn, fs->hi_bsize, 0, &bp)) != 0) {
		return error;
	}
	if (res)
		*res = (char *)bp->b_data + htbc_blkoff(fs, offset);
	*bpp = bp;
	return 0;
}

/****************************************************************/
/* HTBC Extents & Caching */

static bool
htbc_ext_binsearch_index(struct htbc_inode *ip, struct htbc_extent_path *path, daddr_t lbn, daddr_t *first_lbn, daddr_t *last_lbn)
{
	struct htbc_extent_header *ehp = path->ep_header;
	struct htbc_extent_index *first, *last, *l, *r, *m;

	first = (struct htbc_extent_index *)(char *)(ehp + 1);
	last = first + ehp->eh_ecount - 1;
	l = first;
	r = last;
	while (l <= r) {
		m = l + (r - l) / 2;
		if (lbn < m->ei_blk)
			r = m - 1;
		else
			l = m + 1;
	}

	if (l == first) {
		path->ep_sparse_ext.e_blk = *first_lbn;
		path->ep_sparse_ext.e_len = first->ei_blk - *first_lbn;
		path->ep_sparse_ext.e_start_hi = 0;
		path->ep_sparse_ext.e_start_lo = 0;
		path->ep_is_sparse = true;
		return true;
	}
	path->ep_index = l - 1;
	*first_lbn = path->ep_index->ei_blk;
	if (path->ep_index < last)
		*last_lbn = l->ei_blk - 1;
	return false;
}

static void
htbc_ext_binsearch(struct htbc_inode *ip, struct htbc_extent_path *path, daddr_t lbn, daddr_t first_lbn, daddr_t last_lbn)
{
	struct htbc_extent_header *ehp = path->ep_header;
	struct htbc_extent *first, *l, *r, *m;

	if (ehp->eh_ecount == 0)
		return;

	first = (struct htbc_extent *)(char *)(ehp + 1);
	l = first;
	r = first + ehp->eh_ecount - 1;
	while (l <= r) {
		m = l + (r - l) / 2;
		if (lbn < m->e_blk)
			r = m - 1;
		else
			l = m + 1;
	}

	if (l == first) {
		path->ep_sparse_ext.e_blk = first_lbn;
		path->ep_sparse_ext.e_len = first->e_blk - first_lbn;
		path->ep_sparse_ext.e_start_hi = 0;
		path->ep_sparse_ext.e_start_lo = 0;
		path->ep_is_sparse = true;
		return;
	}
	path->ep_ext = l - 1;
	if (path->ep_ext->e_blk + path->ep_ext->e_len <= lbn) {
		path->ep_sparse_ext.e_blk = path->ep_ext->e_blk +
		    path->ep_ext->e_len;
		if (l <= (first + ehp->eh_ecount - 1))
			path->ep_sparse_ext.e_len = l->e_blk -
			    path->ep_sparse_ext.e_blk;
		else
			path->ep_sparse_ext.e_len = last_lbn -
			    path->ep_sparse_ext.e_blk + 1;
		path->ep_sparse_ext.e_start_hi = 0;
		path->ep_sparse_ext.e_start_lo = 0;
		path->ep_is_sparse = true;
	}
}

/*
 * Find a block in ext4 extent cache.
 */
int
htbc_ext_in_cache(struct htbc_inode *ip, daddr_t lbn, struct htbc_extent *ep)
{
	struct htbc_extent_cache *ecp;
	int ret = HTBC_EXT_CACHE_NO;

	ecp = &ip->hi_mfs->hi_ext.hi_ext_cache;

	/* cache is invalid */
	if (ecp->ec_type == HTBC_EXT_CACHE_NO)
		return ret;

	if (lbn >= ecp->ec_blk && lbn < ecp->ec_blk + ecp->ec_len) {
		ep->e_blk = ecp->ec_blk;
		ep->e_start_lo = ecp->ec_start & 0xffffffff;
		ep->e_start_hi = ecp->ec_start >> 32 & 0xffff;
		ep->e_len = ecp->ec_len;
		ret = ecp->ec_type;
	}
	return ret;
}

/*
 * Put an htbc_extent structure in htbc cache.
 */
void
htbc_ext_put_cache(struct htbc_inode *ip, struct htbc_extent *ep, int type)
{
	struct htbc_extent_cache *ecp;

	ecp = &ip->hi_mfs->hi_ext.hi_ext_cache;
	ecp->ec_type = type;
	ecp->ec_blk = ep->e_blk;
	ecp->ec_len = ep->e_len;
	ecp->ec_start = (daddr_t)ep->e_start_hi << 32 | ep->e_start_lo;
}

/*
 * Find an extent.
 */
struct htbc_extent_path *
htbc_ext_find_extent(struct htbc_hi_mfs *fs, struct htbc_inode *ip, daddr_t lbn, struct htbc_extent_path *path)
{
	struct htbc_extent_header *ehp;
	uint16_t i;
	int error, size;
	daddr_t nblk;

	ehp = (struct htbc_extent_header *)ip->hi_blocks;

	if (ehp->eh_magic != HTBC_EXT_MAGIC)
		return NULL;

	path->ep_header = ehp;

	daddr_t first_lbn = 0;
	daddr_t last_lbn = lblkno(ip->hi_mfs, ip->hi_size);

	for (i = ehp->eh_depth; i != 0; --i) {
		path->ep_depth = i;
		path->ep_ext = NULL;
		if (htbc_ext_binsearch_index(ip, path, lbn, &first_lbn, &last_lbn)) {
			return path;
		}

		nblk = (daddr_t)path->ep_index->ei_leaf_hi << 32 |
		    path->ep_index->ei_leaf_lo;
		size = blksize(fs, ip, nblk);
		if (path->ep_bp != NULL) {
			brelse(path->ep_bp, 0);
			path->ep_bp = NULL;
		}
		error = bread(ip->hi_devvp, fsbtodb(fs, nblk), size, 0, &path->ep_bp);
		if (error) {
			brelse(path->ep_bp, 0);
			path->ep_bp = NULL;
			return NULL;
		}
		ehp = (struct htbc_extent_header *)path->ep_bp->b_data;
		path->ep_header = ehp;
	}

	path->ep_depth = i;
	path->ep_ext = NULL;
	path->ep_index = NULL;
	path->ep_is_sparse = false;

	htbc_ext_binsearch(ip, path, lbn, first_lbn, last_lbn);
	return path;
}

/****************************************************************/
