/*	$NetBSD: ext2fs_htree.c,v 1.9 2016/08/23 06:23:26 christos Exp $	*/

/*-
 * Copyright (c) 2010, 2012 Zheng Liu <lz@freebsd.org>
 * Copyright (c) 2012, Vyacheslav Matyushin
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
 * $FreeBSD: head/sys/fs/ext2fs/ext2fs_htree.c 294653 2016-01-24 02:41:49Z pfg $
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: ext2fs_htree.c,v 1.9 2016/08/23 06:23:26 christos Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/lockf.h>
#include <sys/signalvar.h>
#include <sys/malloc.h>

#include <devel/htbc/htbc_hash.h>
#include <devel/htbc/htbc_htree.h>
#include <devel/htbc/htbc.h>

static int htree_find_leaf(struct htbc_inode *, const char *, int , uint32_t *, uint8_t *, struct htree_lookup_info *);

static off_t
htree_get_block(struct htree_entry *ep)
{
	return ep->h_blk & 0x00FFFFFF;
}

static void
htree_release(struct htree_lookup_info *info)
{
	for (u_int i = 0; i < info->h_levels_num; i++) {
		struct buf *bp = info->h_levels[i].h_bp;
		if (bp != NULL)
			brelse(bp, 0);
	}
}

static uint16_t
htree_get_limit(struct htree_entry *ep)
{
	return ((struct htree_count *)(ep))->h_entries_max;
}

static uint32_t
htree_root_limit(struct htbc_inode *ip, int len)
{
	uint32_t space = ip->hi_mfs->hi_bsize - EXT2_DIR_REC_LEN(1) - EXT2_DIR_REC_LEN(2) - len;
	return space / sizeof(struct htree_entry);
}

static uint16_t
htree_get_count(struct htree_entry *ep)
{
	return ((struct htree_count *)(ep))->h_entries_num;
}

static uint32_t
htree_get_hash(struct htree_entry *ep)
{
	return ep->h_hash;
}

static void
htree_set_block(struct htree_entry *ep, uint32_t blk)
{
	ep->h_blk = blk;
}

static void
htree_set_count(struct htree_entry *ep, uint16_t cnt)
{
	((struct htree_count *)(ep))->h_entries_num = cnt;
}

static void
htree_set_hash(struct htree_entry *ep, uint32_t hash)
{
	ep->h_hash = hash;
}

static void
htree_set_limit(struct htree_entry *ep, uint16_t limit)
{
	((struct htree_count *)(ep))->h_entries_max = limit;
}

static uint32_t
htree_node_limit(struct htbc_inode *ip)
{
	struct htbc_hi_mfs *fs;
	uint32_t space;

	fs = ip->hi_mfs;
	space = fs->hi_bsize - EXT2_DIR_REC_LEN(0);

	return space / sizeof(struct htree_entry);
}

static int
htree_append_block(struct vnode *vp, char *data, struct componentname *cnp, uint32_t blksize)
{
	struct iovec aiov;
	struct uio auio;
	struct htbc_inode *dp = VTOI(vp);
	uint64_t cursize, newsize;
	int error;

	cursize = roundup(dp->hi_size, blksize);
	newsize = cursize + blksize;

	auio.uio_offset = cursize;
	auio.uio_resid = blksize;
	aiov.iov_len = blksize;
	aiov.iov_base = data;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_WRITE;
	auio.uio_vmspace = vmspace_kernel();
	error = VOP_WRITE(vp, &auio, IO_SYNC, cnp->cn_cred);
	if (!error)
		dp->hi_size = newsize;

	return error;
}

static int
htree_writebuf(struct htree_lookup_info *info)
{
	int i, error;

	for (i = 0; i < info->h_levels_num; i++) {
		struct buf *bp = info->h_levels[i].h_bp;
		error = bwrite(bp);
		if (error)
			return error;
	}

	return 0;
}

static void
htree_insert_entry_to_level(struct htree_lookup_level *level, uint32_t hash, uint32_t blk)
{
	struct htree_entry *target;
	int entries_num;

	target = level->h_entry + 1;
	entries_num = htree_get_count(level->h_entries);

	memmove(target + 1, target, (char *)(level->h_entries + entries_num) -
	    (char *)target);
	htree_set_block(target, blk);
	htree_set_hash(target, hash);
	htree_set_count(level->h_entries, entries_num + 1);
}

/*
 * Insert an index entry to the index node.
 */
static void
htree_insert_entry(struct htree_lookup_info *info, uint32_t hash, uint32_t blk)
{
	struct htree_lookup_level *level;

	level = &info->h_levels[info->h_levels_num - 1];
	htree_insert_entry_to_level(level, hash, blk);
}

/*
 * Compare two entry sort descriptors by name hash value.
 * This is used together with qsort.
 */
static int
htree_cmp_sort_entry(const void *e1, const void *e2)
{
	const struct htree_sort_entry *entry1, *entry2;

	entry1 = (const struct htree_sort_entry *)e1;
	entry2 = (const struct htree_sort_entry *)e2;

	if (entry1->h_hash < entry2->h_hash)
		return -1;
	if (entry1->h_hash > entry2->h_hash)
		return 1;
	return 0;
}

/*
 * Append an entry to the end of the directory block.
 */
static void
htree_append_entry(char *block, uint32_t blksize, struct htree_fake_direct *last_entry, struct htree_fake_direct *new_entry)
{
	uint16_t entry_len;

	entry_len = EXT2_DIR_REC_LEN(last_entry->h_namlen);
	last_entry->h_reclen = entry_len;
	last_entry = (struct htree_fake_direct *)((char *)last_entry + entry_len);
	new_entry->h_reclen = block + blksize - (char *)last_entry;
	memcpy(last_entry, new_entry, EXT2_DIR_REC_LEN(new_entry->h_namlen));
}

/*
 * Move half of entries from the old directory block to the new one.
 */
static int
htree_split_dirblock(char *block1, char *block2, uint32_t blksize, uint32_t *hash_seed, uint8_t hash_version, uint32_t *split_hash, struct htree_fake_direct *entry)
{
	int entry_cnt = 0;
	int size = 0;
	int i, k;
	uint32_t offset;
	uint16_t entry_len = 0;
	uint32_t entry_hash;
	struct htree_fake_direct *ep, *last;
	char *dest;
	struct htree_sort_entry *sort_info, dummy;

	ep = (struct htree_fake_direct *)block1;
	dest = block2;
	sort_info = (struct htree_sort_entry *)
	    ((char *)block2 + blksize);

	/*
	 * Calculate name hash value for the entry which is to be added.
	 */
	htbc_tree_hash(entry->h_name, entry->h_namlen, hash_seed,
	    hash_version, &entry_hash, NULL);

	/*
	 * Fill in directory entry sort descriptors.
	 */
	while ((char *)ep < block1 + blksize) {
		if (ep->h_ino && ep->h_namlen) {
			entry_cnt++;
			sort_info--;
			sort_info->h_size = ep->h_reclen;
			sort_info->h_offset = (char *)ep - block1;
			htbc_htree_hash(ep->h_name, ep->h_namlen,
			    hash_seed, hash_version,
			    &sort_info->h_hash, NULL);
		}
		ep = (struct htree_fake_direct *)
		    ((char *)ep + ep->h_reclen);
	}

	/*
	 * Sort directory entry descriptors by name hash value.
	 */
	kheapsort(sort_info, entry_cnt, sizeof(struct htree_sort_entry),
	    htree_cmp_sort_entry, &dummy);

	/*
	 * Count the number of entries to move to directory block 2.
	 */
	for (i = entry_cnt - 1; i >= 0; i--) {
		if (sort_info[i].h_size + size > blksize / 2)
			break;
		size += sort_info[i].h_size;
	}

	*split_hash = sort_info[i + 1].h_hash;

	/*
	 * Set collision bit.
	 */
	if (*split_hash == sort_info[i].h_hash)
		*split_hash += 1;

	/*
	 * Move half of directory entries from block 1 to block 2.
	 */
	for (k = i + 1; k < entry_cnt; k++) {
		ep = (struct htree_fake_direct *)((char *)block1 +
		    sort_info[k].h_offset);
		entry_len = EXT2_DIR_REC_LEN(ep->h_namlen);
		memcpy(dest, ep, entry_len);
		((struct htree_fake_direct *)dest)->h_reclen = entry_len;
		/* Mark directory entry as unused. */
		ep->h_ino = 0;
		dest += entry_len;
	}
	dest -= entry_len;

	/* Shrink directory entries in block 1. */
	last = (struct htree_fake_direct *)block1;
	entry_len = 0;
	for (offset = 0; offset < blksize;) {
		ep = (struct htree_fake_direct *)(block1 + offset);
		offset += ep->h_reclen;
		if (ep->h_ino) {
			last = (struct htree_fake_direct *)
			   ((char *)last + entry_len);
			entry_len = EXT2_DIR_REC_LEN(ep->h_namlen);
			memcpy((void *)last, (void *)ep, entry_len);
			last->h_reclen = entry_len;
		}
	}

	if (entry_hash >= *split_hash) {
		/* Add entry to block 2. */
		htree_append_entry(block2, blksize,
		    (struct htree_fake_direct *)dest, entry);

		/* Adjust length field of last entry of block 1. */
		last->h_reclen = block1 + blksize - (char *)last;
	} else {
		/* Add entry to block 1. */
		htree_append_entry(block1, blksize, last, entry);

		/* Adjust length field of last entry of block 2. */
		((struct htree_fake_direct *)dest)->h_reclen =
		    block2 + blksize - dest;
	}

	return 0;
}

/*
 * Create an HTree index for a directory having entries which are no more
 * accommodable in a single dir-block.
 */
int
htree_create_index(struct vnode *vp, struct componentname *cnp, struct htree_fake_direct *new_entry)
{
	struct buf *bp = NULL;
	struct htbc_inode *dp;
	struct htbc_hi_fs *fs;
	struct htbc_hi_mfs 	*m_fs;
	struct htree_fake_direct *ep, *dotdot;
	struct htree_root *root;
	struct htree_lookup_info info;
	uint32_t blksize, dirlen, split_hash;
	uint8_t hash_version;
	char *buf1 = NULL;
	char *buf2 = NULL;
	int error = 0;

	dp = VTOI(vp);
	fs = &(dp->hi_mfs->hi_fs);
	m_fs = dp->hi_mfs;
	blksize = m_fs->hi_bsize;

	buf1 = malloc(blksize, M_TEMP, M_WAITOK | M_ZERO);
	buf2 = malloc(blksize, M_TEMP, M_WAITOK | M_ZERO);

	if ((error = htbc_blkatoff(vp, 0, NULL, &bp)) != 0)
		goto out;

	root = (struct htree_root *)bp->b_data;
	dotdot = (struct htree_fake_direct *)((char *)&(root->h_dotdot));
	ep = (struct htree_fake_direct *)((char *)dotdot + dotdot->h_reclen);
	dirlen = (char *)root + blksize - (char *)ep;
	memcpy(buf1, ep, dirlen);
	ep = (struct htree_fake_direct *)buf1;
	while ((char *)ep < buf1 + dirlen)
		ep = (struct htree_fake_direct *)((char *)ep + ep->h_reclen);
	ep->h_reclen = buf1 + blksize - (char *)ep;
	/* XXX It should be made dp->i_flag |= IN_E3INDEX; */
	dp->hi_sflags |= EXT2_INDEX;

	/*
	 * Initialize index root.
	 */
	dotdot->h_reclen = blksize - EXT2_DIR_REC_LEN(1);
	memset(&root->h_info, 0, sizeof(root->h_info));
	root->h_info.h_hash_version = fs->hi_def_hash_version;
	root->h_info.h_info_len = sizeof(root->h_info);
	htree_set_block(root->h_entries, 1);
	htree_set_count(root->h_entries, 1);
	htree_set_limit(root->h_entries, htree_root_limit(dp, sizeof(root->h_info)));

	memset(&info, 0, sizeof(info));
	info.h_levels_num = 1;
	info.h_levels[0].h_entries = root->h_entries;
	info.h_levels[0].h_entry = root->h_entries;

	hash_version = root->h_info.h_hash_version;
	if (hash_version <= HTREE_TEA)
		hash_version += m_fs->hi_uhash;
	htree_split_dirblock(buf1, buf2, blksize, fs->hi_hash_seed,
	    hash_version, &split_hash, new_entry);
	htree_insert_entry(&info, split_hash, 2);

	/*
	 * Write directory block 0.
	 */
	if ((vp)->v_mount->mnt_iflag & IO_SYNC)
		(void)bwrite(bp);
	else
		bdwrite(bp);

	dp->hi_flag |= IN_CHANGE | IN_UPDATE;

	/*
	 * Write directory block 1.
	 */
	error = htree_append_block(vp, buf1, cnp, blksize);
	if (error)
		goto out1;

	/*
	 * Write directory block 2.
	 */
	error = htree_append_block(vp, buf2, cnp, blksize);
	goto out1;
out:
	if (bp != NULL)
		brelse(bp, 0);
out1:
	free(buf1, M_TEMP);
	free(buf2, M_TEMP);
	return error;
}

/*
 * Add an entry to the directory using htree index.
 */
int
htree_add_entry(struct vnode *dvp, struct htree_fake_direct *entry, struct componentname *cnp, size_t newentrysize)
{
	struct htree_entry *entries, *leaf_node;
	struct htree_lookup_info info;
	struct buf *bp = NULL;
	struct htbc_fs *fs;
	struct htbc_hi_mfs *m_fs;
	struct htbc_inode *ip;
	uint16_t ent_num;
	uint32_t dirhash, split_hash;
	uint32_t blksize, blknum;
	uint64_t cursize, dirsize;
	uint8_t hash_version;
	char *newdirblock = NULL;
	char *newidxblock = NULL;
	struct htree_node *dst_node;
	struct htree_entry *dst_entries;
	struct htree_entry *root_entires;
	struct buf *dst_bp = NULL;
	int error, write_bp = 0, write_dst_bp = 0, write_info = 0;

	ip = VTOI(dvp);
	m_fs = ip->hi_mfs;
	fs = &(m_fs->hi_fs);
	blksize = m_fs->hi_bsize;

	if (ip->hi_count != 0)
		return ext2fs_add_entry(dvp, entry, &(ip), newentrysize);

	/* Target directory block is full, split it */
	memset(&info, 0, sizeof(info));
	error = htree_find_leaf(ip, entry->h_name, entry->h_namlen,
	    &dirhash, &hash_version, &info);
	if (error)
		return error;
	entries = info.h_levels[info.h_levels_num - 1].h_entries;
	ent_num = htree_get_count(entries);
	if (ent_num == htree_get_limit(entries)) {
		/* Split the index node. */
		root_entires = info.h_levels[0].h_entries;
		newidxblock = malloc(blksize, M_TEMP, M_WAITOK | M_ZERO);
		dst_node = (struct htree_node *)newidxblock;
		dst_entries = dst_node->h_entries;
		memset(&dst_node->h_fake_dirent, 0,
		    sizeof(dst_node->h_fake_dirent));
		dst_node->h_fake_dirent.h_reclen = blksize;

		cursize = roundup(ip->hi_size, blksize);
		dirsize = cursize + blksize;
		blknum = dirsize / blksize - 1;

		error = htree_append_block(dvp, newidxblock,
		    cnp, blksize);
		if (error)
			goto finish;
		error = htbc_blkatoff(dvp, cursize, NULL, &dst_bp);
		if (error)
			goto finish;

		dst_node = (struct htree_node *)dst_bp->b_data;
		dst_entries = dst_node->h_entries;

		if (info.h_levels_num == 2) {
			uint16_t src_ent_num, dst_ent_num;

			if (htree_get_count(root_entires) ==
			    htree_get_limit(root_entires)) {
				/* Directory index is full */
				error = EIO;
				goto finish;
			}

			src_ent_num = ent_num / 2;
			dst_ent_num = ent_num - src_ent_num;
			split_hash = htree_get_hash(entries + src_ent_num);

			/* Move half of index entries to the new index node */
			memcpy(dst_entries, entries + src_ent_num,
			    dst_ent_num * sizeof(struct htree_entry));
			htree_set_count(entries, src_ent_num);
			htree_set_count(dst_entries, dst_ent_num);
			htree_set_limit(dst_entries, htree_node_limit(ip));

			if (info.h_levels[1].h_entry >= entries + src_ent_num) {
				struct buf *tmp = info.h_levels[1].h_bp;
				info.h_levels[1].h_bp = dst_bp;
				dst_bp = tmp;

				info.h_levels[1].h_entry =
				    info.h_levels[1].h_entry -
				    (entries + src_ent_num) +
				    dst_entries;
				info.h_levels[1].h_entries = dst_entries;
			}
			htree_insert_entry_to_level(&info.h_levels[0],
			    split_hash, blknum);

			/* Write new index node to disk */
			error = bwrite(dst_bp);
			ip->hi_flag |= IN_CHANGE | IN_UPDATE;
			if (error)
				goto finish;

			write_dst_bp = 1;
		} else {
			/* Create second level for htree index */

			struct htree_root *idx_root;

			memcpy(dst_entries, entries,
			    ent_num * sizeof(struct htree_entry));
			htree_set_limit(dst_entries, htree_node_limit(ip));

			idx_root = (struct htree_root *)
			    info.h_levels[0].h_bp->b_data;
			idx_root->h_info.h_ind_levels = 1;

			htree_set_count(entries, 1);
			htree_set_block(entries, blknum);

			info.h_levels_num = 2;
			info.h_levels[1].h_entries = dst_entries;
			info.h_levels[1].h_entry = info.h_levels[0].h_entry -
			    info.h_levels[0].h_entries + dst_entries;
			info.h_levels[1].h_bp = dst_bp;
			dst_bp = NULL;
		}
	}

	leaf_node = info.h_levels[info.h_levels_num - 1].h_entry;
	blknum = htree_get_block(leaf_node);
	error = htbc_blkatoff(dvp, blknum * blksize, NULL, &bp);
	if (error)
		goto finish;

	/* Split target directory block */
	newdirblock = malloc(blksize, M_TEMP, M_WAITOK | M_ZERO);
	htree_split_dirblock((char *)bp->b_data, newdirblock, blksize,
	    fs->h_hash_seed, hash_version, &split_hash, entry);
	cursize = roundup(ip->hi_size, blksize);
	dirsize = cursize + blksize;
	blknum = dirsize / blksize - 1;

	/* Add index entry for the new directory block */
	htree_insert_entry(&info, split_hash, blknum);

	/* Write the new directory block to the end of the directory */
	error = htree_append_block(dvp, newdirblock, cnp, blksize);

	if (error)
		goto finish;

	/* Write the target directory block */
	error = bwrite(bp);
	ip->hi_flag |= IN_CHANGE | IN_UPDATE;

	if (error)
		goto finish;
	write_bp = 1;

	/* Write the index block */
	error = htree_writebuf(&info);
	if (!error)
		write_info = 1;

finish:
	if (dst_bp != NULL && !write_dst_bp)
		brelse(dst_bp, 0);
	if (bp != NULL && !write_bp)
		brelse(bp, 0);
	if (newdirblock != NULL)
		free(newdirblock, M_TEMP);
	if (newidxblock != NULL)
		free(newidxblock, M_TEMP);
	if (!write_info)
		htree_release(&info);
	return error;
}

static int
htree_check_next(struct htbc_inode *ip, uint32_t hash, const char *name, struct htree_lookup_info *info)
{
	struct vnode *vp = ITOV(ip);
	struct htree_lookup_level *level;
	struct buf *bp;
	uint32_t next_hash;
	int idx = info->h_levels_num - 1;
	int levels = 0;

	for (;;) {
		level = &info->h_levels[idx];
		level->h_entry++;
		if (level->h_entry < level->h_entries +
		    htree_get_count(level->h_entries))
			break;
		if (idx == 0)
			return 0;
		idx--;
		levels++;
	}

	next_hash = htree_get_hash(level->h_entry);
	if ((hash & 1) == 0) {
		if (hash != (next_hash & ~1))
			return 0;
	}

	while (levels > 0) {
		levels--;
		if (htbc_blkatoff(vp, htree_get_block(level->h_entry) *
		    ip->hi_mfs->hi_bsize, NULL, &bp) != 0)
			return 0;
		level = &info->h_levels[idx + 1];
		brelse(level->h_bp, 0);
		level->h_bp = bp;
		level->h_entry = level->h_entries =
		    ((struct htree_node *)bp->b_data)->h_entries;
	}

	return 1;
}

static int
htree_find_leaf(struct htbc_inode *ip, const char *name, int namelen, uint32_t *hash, uint8_t *hash_ver, struct htree_lookup_info *info)
{
	struct vnode *vp;
	struct htbc_fs *fs;
	struct htbc_hi_mfs *m_fs; /* F, G, and H are MD4 functions */
	struct buf *bp = NULL;
	struct htree_root *rootp;
	struct htree_entry *entp, *start, *end, *middle, *found;
	struct htree_lookup_level *level_info;
	uint32_t hash_major = 0, hash_minor = 0;
	uint32_t levels, cnt;
	uint8_t hash_version;

	if (name == NULL || info == NULL)
		return -1;

	vp = ITOV(ip);
	fs = &(ip->hi_mfs->hi_fs);
	m_fs = ip->hi_mfs;

	if (htbc_blkatoff(vp, 0, NULL, &bp) != 0)
		return -1;

	info->h_levels_num = 1;
	info->h_levels[0].h_bp = bp;
	rootp = (struct htree_root *)bp->b_data;
	if (rootp->h_info.h_hash_version != HTREE_LEGACY &&
	    rootp->h_info.h_hash_version != HTREE_HALF_MD4 &&
	    rootp->h_info.h_hash_version != HTREE_TEA)
		goto error;

	hash_version = rootp->h_info.h_hash_version;
	if (hash_version <= HTREE_TEA)
		hash_version += m_fs->hi_uhash;
	*hash_ver = hash_version;

	htbc_htree_hash(name, namelen, fs->h_hash_seed,
	    hash_version, &hash_major, &hash_minor);
	*hash = hash_major;

	if ((levels = rootp->h_info.h_ind_levels) > 1)
		goto error;

	entp = (struct htree_entry *)(((char *)&rootp->h_info) +
	    rootp->h_info.h_info_len);

	if (htree_get_limit(entp) !=
	    htree_root_limit(ip, rootp->h_info.h_info_len))
		goto error;

	for (;;) {
		cnt = htree_get_count(entp);
		if (cnt == 0 || cnt > htree_get_limit(entp))
			goto error;

		start = entp + 1;
		end = entp + cnt - 1;
		while (start <= end) {
			middle = start + (end - start) / 2;
			if (htree_get_hash(middle) > hash_major)
				end = middle - 1;
			else
				start = middle + 1;
		}
		found = start - 1;

		level_info = &(info->h_levels[info->h_levels_num - 1]);
		level_info->h_bp = bp;
		level_info->h_entries = entp;
		level_info->h_entry = found;
		if (levels == 0)
			return 0;
		levels--;
		if (htbc_blkatoff(vp,
		    htree_get_block(found) * m_fs->hi_bsize,
		    NULL, &bp) != 0)
			goto error;
		entp = ((struct htree_node *)bp->b_data)->h_entries;
		info->h_levels_num++;
		info->h_levels[info->h_levels_num - 1].h_bp = bp;
	}

error:
	htree_release(info);
	return -1;
}

/*
 * Try to lookup a directory entry in HTree index
 */
int
htree_lookup(struct htbc_inode *ip, const char *name, int namelen, struct buf **bpp, int *entryoffp, int32_t *offp, int32_t *prevoffp, int32_t *endusefulp, struct htbc_hi_searchslot *ss)
{
	struct vnode *vp;
	struct htree_lookup_info info;
	struct htree_entry *leaf_node;
	struct htbc_hi_mfs *m_fs;
	struct buf *bp;
	uint32_t blk;
	uint32_t dirhash;
	uint32_t bsize;
	uint8_t hash_version;
	int search_next;

	m_fs = ip->hi_mfs;
	bsize = m_fs->hi_bsize;
	vp = ITOV(ip);

	/* TODO: print error msg because we don't lookup '.' and '..' */

	memset(&info, 0, sizeof(info));
	if (htree_find_leaf(ip, name, namelen, &dirhash,
	    &hash_version, &info)) {
		return -1;
	}

	do {
		leaf_node = info.h_levels[info.h_levels_num - 1].h_entry;
		blk = htree_get_block(leaf_node);
		if (htbc_blkatoff(vp, blk * bsize, NULL, &bp) != 0) {
			htree_release(&info);
			return -1;
		}

		*offp = blk * bsize;
		*entryoffp = 0;
		*prevoffp = blk * bsize;
		*endusefulp = blk * bsize;

		if (ss->hi_slotstatus == NONE) {
			ss->hi_slotoffset = -1;
			ss->hi_slotfreespace = 0;
		}

		int found;
		if (ext2fs_search_dirblock(ip, bp->b_data, &found,
		    name, namelen, entryoffp, offp, prevoffp,
		    endusefulp, ss) != 0) {
			brelse(bp, 0);
			htree_release(&info);
			return -1;
		}

		if (found) {
			*bpp = bp;
			htree_release(&info);
			return 0;
		}

		brelse(bp, 0);
		search_next = htree_check_next(ip, dirhash, name, &info);
	} while (search_next);

	htree_release(&info);
	return ENOENT;
}
