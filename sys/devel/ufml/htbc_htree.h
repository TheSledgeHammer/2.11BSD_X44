/*	$NetBSD: ext2fs_htree.h,v 1.1 2016/06/24 17:21:30 christos Exp $	*/

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
 * $FreeBSD: head/sys/fs/ext2fs/htree.h 262623 2014-02-28 21:25:32Z pfg $
 */
#ifndef _FS_HTREE_H_
#define	_FS_HTREE_H_

#define	HTREE_LEGACY				0
#define	HTREE_HALF_MD4				1
#define	HTREE_TEA					2
#define	HTREE_LEGACY_UNSIGNED		3
#define	HTREE_HALF_MD4_UNSIGNED		4
#define	HTREE_TEA_UNSIGNED			5

#define	HTREE_EOF 					0x7FFFFFFF

/* To Fix:
 * ext2fs_add_entry (line 483)
 * ext2fs_blkatoff
 */

struct htree_fake_inode_ext {
	daddr_t h_last_lblk;	/* last logical block allocated */
	daddr_t h_last_blk;		/* last block allocated on disk */
	struct ext4_extent_cache i_ext_cache; /* cache for ext4 extent */
};

struct htree_fake_inode {
		u_int32_t	h_blocks;
    	u_int64_t   h_size;					/* File byte count. */
    	u_int32_t   h_flag;	    			/* flags, see below */
        u_int32_t   h_sflags;				/* 32: Status flags (chflags) */
        struct	    htree_mfs *h_mfs;	    /* h_tree_mfs */
        int32_t	  	h_count;				/* Size of free slot in directory. */
};

struct htree_mfs {
	struct htree_fake_fs   		h_fs;
    int8_t	        			h_uhash;	/* 3 if hash should be signed, 0 if not */
	int32_t	        			h_bsize;	/* block size */
    struct htree_fake_inode_ext h_ext;
};

struct htree_fake_fs {
	uint32_t  	h_hash_seed[4];				/* HTREE hash seed */
    char      	h_def_hash_version;			/* Default hash version to use */
};

struct htree_searchslot {
	enum htree_slotstatus 	h_slotstatus;
	int32_t 				h_slotoffset;				/* offset of area with free space */
	int 					h_slotsize;					/* size of area at slotoffset */
	int 					h_slotfreespace;			/* amount of space free in slot */
	int 					h_slotneeded;				/* sizeof the entry we are seeking */
	int32_t					h_slotcount;				/* Size of free slot in directory. */
};

enum htree_slotstatus {
	NONE,
	COMPACT,
	FOUND
};

struct htree_fake_direct {
	uint32_t 	h_ino;						/* inode number of entry */
	uint16_t 	h_reclen;					/* length of this record */
	uint8_t  	h_namlen;					/* length of string in d_name */
	uint8_t  	h_type;						/* file type */
	char        h_name[EXT2FS_MAXNAMLEN];	/* name with length<=EXT2FS_MAXNAMLEN */
};

struct htree_count {
	uint16_t h_entries_max;
	uint16_t h_entries_num;
};

struct htree_entry {
	uint32_t h_hash;
	uint32_t h_blk;
};

struct htree_root_info {
	uint32_t h_reserved1;
	uint8_t  h_hash_version;
	uint8_t  h_info_len;
	uint8_t  h_ind_levels;
	uint8_t  h_reserved2;
};

struct htree_root {
	struct htree_fake_direct h_dot;
	char h_dot_name[4];
	struct htree_fake_direct h_dotdot;
	char h_dotdot_name[4];
	struct htree_root_info h_info;
	struct htree_entry h_entries[0];
};

struct htree_node {
	struct htree_fake_direct h_fake_dirent;
	struct htree_htree_entry h_entries[0];
};

struct htree_lookup_level {
	struct buf *h_bp;
	struct htree_entry *h_entries;
	struct htree_entry *h_entry;
};

struct htree_lookup_info {
	struct htree_lookup_level h_levels[2];
	uint32_t h_levels_num;
};

struct htree_sort_entry {
	uint16_t h_offset;
	uint16_t h_size;
	uint32_t h_hash;
};

static off_t	htree_get_block(struct htree_entry *ep);
static void 	htree_release(struct htree_lookup_info *info);
static uint16_t htree_get_limit(struct htree_entry *ep);
static uint32_t htree_root_limit(struct htree_fake_inode *ip, int len);
static uint16_t htree_get_count(struct htree_entry *ep);
static uint32_t htree_get_hash(struct htree_entry *ep);
static void 	htree_set_block(struct htree_entry *ep, uint32_t blk);
static void 	htree_set_count(struct htree_entry *ep, uint16_t cnt);
static void 	htree_set_hash(struct htree_entry *ep, uint32_t hash);
static void 	htree_set_limit(struct htree_entry *ep, uint16_t limit);
static uint32_t htree_node_limit(struct htree_fake_inode *ip);
static int 		htree_append_block(struct vnode *vp, char *data, struct componentname *cnp, uint32_t blksize);
static int 		htree_writebuf(struct htree_lookup_info *info);
static void 	htree_insert_entry_to_level(struct htree_lookup_level *level, uint32_t hash, uint32_t blk);
static void 	htree_insert_entry(struct htree_lookup_info *info, uint32_t hash, uint32_t blk);
static int 		htree_cmp_sort_entry(const void *e1, const void *e2);
static void 	htree_append_entry(char *block, uint32_t blksize, struct htree_fake_direct *last_entry, struct htree_fake_direct *new_entry);
static int 		htree_split_dirblock(char *block1, char *block2, uint32_t blksize, uint32_t *hash_seed, uint8_t hash_version, uint32_t *split_hash, struct htree_fake_direct *entry);
int 			htree_create_index(struct vnode *vp, struct componentname *cnp, struct htree_fake_direct *new_entry);
int 			htree_add_entry(struct vnode *dvp, struct htree_fake_direct *entry, struct componentname *cnp, size_t newentrysize);
static int 		htree_check_next(struct htree_fake_inode *ip, uint32_t hash, const char *name, struct htree_lookup_info *info);
static int 		htree_find_leaf(struct htree_fake_inode *ip, const char *name, int namelen, uint32_t *hash, uint8_t *hash_ver, struct htree_lookup_info *info);
int 			htree_lookup(struct htree_fake_inode *ip, const char *name, int namelen, struct buf **bpp, int *entryoffp, int32_t *offp, int32_t *prevoffp, int32_t *endusefulp, struct htree_searchslot *ss);
#endif /* !_FS_EXT2FS_HTREE_H_ */
