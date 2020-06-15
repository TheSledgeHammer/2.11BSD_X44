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
 * @(#)vfs_htbc.c	1.00
 */

/* Augment Caching, Defragmentation, Read-Write and Lookup of Log-Structured Filesystems  */

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

#include <devel/htbc/htbc.h>
#include <devel/htbc/htbc_htree.h>

#define	KDASSERT(x) 			assert(x)
#define	KASSERT(x) 				assert(x)
#define	htbc_alloc(s) 			malloc(s)
#define	htbc_free(a, s) 		free(a)
#define	htbc_calloc(n, s) 		calloc((n), (s))

#define htbc_lock()				simplelock()
#define htbc_unlock() 			simpleunlock()

struct htbc_dealloc {
	TAILQ_ENTRY(htbc_dealloc) 	hd_entries;
	daddr_t 					hd_blkno;	/* address of block */
	int 						hd_len;		/* size of block */
};

LIST_HEAD(htbc_ino_head, htbc_ino);
struct htbc {
	CIRCLEQ_ENTRY(hbchain) 		*ht_bc_entry;
	struct vnode 				*ht_devvp;
	struct mount 				*ht_mount;

	struct htbc_hc_header 		*ht_hc_header;
	struct simplelock 			ht_interlock;

	TAILQ_HEAD(, htbc_dealloc) 	ht_dealloclist;
	int 						ht_dealloccnt;
	int 						ht_dealloclim;

	struct htbc_ino_head 		*ht_inohash;
	u_long 						ht_inohashmask;
	int 						ht_inohashcnt;

	TAILQ_HEAD(, htbc_entry) 	ht_entries;
};

static struct htbc_dealloc 		*htbc_dealloc;
static struct htbc_entry 		*htbc_entry;
static struct htbc_hbchain 		*htbc_hbchain;

struct htbc_ino {
	LIST_ENTRY(htbc_ino) 		hti_hash;
	ino_t 						hti_ino;
	mode_t 						hti_mode;
};

void
htbc_init()
{
	&htbc_entry = (struct htbc_entry *) malloc(sizeof(struct htbc_entry), M_HTBC, NULL);
	&htbc_dealloc = (struct htbc_dealloc *) malloc(sizeof(struct htbc_dealloc), M_HTBC, NULL);
	&htbc_hbchain = (struct hbchain *) malloc(sizeof(struct hbchain), M_HTBC, NULL);
}

void
htbc_fini()
{
	FREE(&htbc_entry, M_HTBC);
	FREE(&htbc_dealloc, M_HTBC);
	FREE(&htbc_hbchain, M_HTBC);
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
		path->ep_is_sparse = TRUE;
		return TRUE;
	}
	path->ep_index = l - 1;
	*first_lbn = path->ep_index->ei_blk;
	if (path->ep_index < last)
		*last_lbn = l->ei_blk - 1;
	return FALSE;
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
		path->ep_is_sparse = TRUE;
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
		path->ep_is_sparse = TRUE;
	}
}

/*
 * Find a block in htbc extent cache.
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
	path->ep_is_sparse = FALSE;

	htbc_ext_binsearch(ip, path, lbn, first_lbn, last_lbn);
	return path;
}

/****************************************************************/
/* HTBC htree hash */

/*
 * FF, GG, and HH are transformations for rounds 1, 2, and 3.
 * Rotation is separated from addition to prevent recomputation.
 */

#define FF(a, b, c, d, x, s) { \
	(a) += F ((b), (c), (d)) + (x); \
	(a) = ROTATE_LEFT ((a), (s)); \
}

#define GG(a, b, c, d, x, s) { \
	(a) += G ((b), (c), (d)) + (x) + (uint32_t)0x5A827999; \
	(a) = ROTATE_LEFT ((a), (s)); \
}

#define HH(a, b, c, d, x, s) { \
	(a) += H ((b), (c), (d)) + (x) + (uint32_t)0x6ED9EBA1; \
	(a) = ROTATE_LEFT ((a), (s)); \
}

static void
htbc_prep_hashbuf(const char *src, int slen, uint32_t *dst, int dlen, int unsigned_char)
{
	uint32_t padding = slen | (slen << 8) | (slen << 16) | (slen << 24);
	uint32_t buf_val;
	const unsigned char *ubuf = (const unsigned char *)src;
	const signed char *sbuf = (const signed char *)src;
	int len, i;
	int buf_byte;

	if (slen > dlen)
		len = dlen;
	else
		len = slen;

	buf_val = padding;

	for (i = 0; i < len; i++) {
		if (unsigned_char)
			buf_byte = (u_int)ubuf[i];
		else
			buf_byte = (int)sbuf[i];

		if ((i % 4) == 0)
			buf_val = padding;

		buf_val <<= 8;
		buf_val += buf_byte;

		if ((i % 4) == 3) {
			*dst++ = buf_val;
			dlen -= sizeof(uint32_t);
			buf_val = padding;
		}
	}

	dlen -= sizeof(uint32_t);
	if (dlen >= 0)
		*dst++ = buf_val;

	dlen -= sizeof(uint32_t);
	while (dlen >= 0) {
		*dst++ = padding;
		dlen -= sizeof(uint32_t);
	}
}

static uint32_t
htbc_legacy_hash(const char *name, int len, int unsigned_char)
{
	uint32_t h0, h1 = 0x12A3FE2D, h2 = 0x37ABE8F9;
	uint32_t multi = 0x6D22F5;
	const unsigned char *uname = (const unsigned char *)name;
	const signed char *sname = (const signed char *)name;
	int val, i;

	for (i = 0; i < len; i++) {
		if (unsigned_char)
			val = (u_int)*uname++;
		else
			val = (int)*sname++;

		h0 = h2 + (h1 ^ (val * multi));
		if (h0 & 0x80000000)
			h0 -= 0x7FFFFFFF;
		h2 = h1;
		h1 = h0;
	}

	return h1 << 1;
}

/*
 * MD4 basic transformation.  It transforms state based on block.
 *
 * This is a half md4 algorithm since Linux uses this algorithm for dir
 * index.  This function is derived from the RSA Data Security, Inc. MD4
 * Message-Digest Algorithm and was modified as necessary.
 *
 * The return value of this function is uint32_t in Linux, but actually we don't
 * need to check this value, so in our version this function doesn't return any
 * value.
 */
static void
htbc_half_md4(uint32_t hash[4], uint32_t data[8])
{
	uint32_t a = hash[0], b = hash[1], c = hash[2], d = hash[3];

	/* Round 1 */
	FF(a, b, c, d, data[0],  3);
	FF(d, a, b, c, data[1],  7);
	FF(c, d, a, b, data[2], 11);
	FF(b, c, d, a, data[3], 19);
	FF(a, b, c, d, data[4],  3);
	FF(d, a, b, c, data[5],  7);
	FF(c, d, a, b, data[6], 11);
	FF(b, c, d, a, data[7], 19);

	/* Round 2 */
	GG(a, b, c, d, data[1],  3);
	GG(d, a, b, c, data[3],  5);
	GG(c, d, a, b, data[5],  9);
	GG(b, c, d, a, data[7], 13);
	GG(a, b, c, d, data[0],  3);
	GG(d, a, b, c, data[2],  5);
	GG(c, d, a, b, data[4],  9);
	GG(b, c, d, a, data[6], 13);

	/* Round 3 */
	HH(a, b, c, d, data[3],  3);
	HH(d, a, b, c, data[7],  9);
	HH(c, d, a, b, data[2], 11);
	HH(b, c, d, a, data[6], 15);
	HH(a, b, c, d, data[1],  3);
	HH(d, a, b, c, data[5],  9);
	HH(c, d, a, b, data[0], 11);
	HH(b, c, d, a, data[4], 15);

	hash[0] += a;
	hash[1] += b;
	hash[2] += c;
	hash[3] += d;
}

/*
 * Tiny Encryption Algorithm.
 */
static void
htbc_tea(uint32_t hash[4], uint32_t data[8])
{
	uint32_t tea_delta = 0x9E3779B9;
	uint32_t sum;
	uint32_t x = hash[0], y = hash[1];
	int n = 16;
	int i = 1;

	while (n-- > 0) {
		sum = i * tea_delta;
		x += ((y << 4) + data[0]) ^ (y + sum) ^ ((y >> 5) + data[1]);
		y += ((x << 4) + data[2]) ^ (x + sum) ^ ((x >> 5) + data[3]);
		i++;
	}

	hash[0] += x;
	hash[1] += y;
}

int
htbc_htree_hash(const char *name, int len,
    uint32_t *hash_seed, int hash_version,
    uint32_t *hash_major, uint32_t *hash_minor)
{
	uint32_t hash[4];
	uint32_t data[8];
	uint32_t major = 0, minor = 0;
	int unsigned_char = 0;

	if (!name || !hash_major)
		return -1;

	if (len < 1 || len > 255)
		goto error;

	hash[0] = 0x67452301;
	hash[1] = 0xEFCDAB89;
	hash[2] = 0x98BADCFE;
	hash[3] = 0x10325476;

	if (hash_seed)
		memcpy(hash, hash_seed, sizeof(hash));

	switch (hash_version) {
	case HTREE_TEA_UNSIGNED:
		unsigned_char = 1;
		/* FALLTHROUGH */
	case HTREE_TEA:
		while (len > 0) {
			htbc_prep_hashbuf(name, len, data, 16, unsigned_char);
			htbc_tea(hash, data);
			len -= 16;
			name += 16;
		}
		major = hash[0];
		minor = hash[1];
		break;
	case HTREE_LEGACY_UNSIGNED:
		unsigned_char = 1;
		/* FALLTHROUGH */
	case HTREE_LEGACY:
		major = htbc_legacy_hash(name, len, unsigned_char);
		break;
	case HTREE_HALF_MD4_UNSIGNED:
		unsigned_char = 1;
		/* FALLTHROUGH */
	case HTREE_HALF_MD4:
		while (len > 0) {
			htbc_prep_hashbuf(name, len, data, 32, unsigned_char);
			htbc_half_md4(hash, data);
			len -= 32;
			name += 32;
		}
		major = hash[1];
		minor = hash[2];
		break;
	default:
		goto error;
	}

	major &= ~1;
	if (major == (HTREE_EOF << 1))
		major = (HTREE_EOF - 1) << 1;
	*hash_major = major;
	if (hash_minor)
		*hash_minor = minor;

	return 0;

error:
	*hash_major = 0;
	if (hash_minor)
		*hash_minor = 0;
	return -1;
}

/****************************************************************/
