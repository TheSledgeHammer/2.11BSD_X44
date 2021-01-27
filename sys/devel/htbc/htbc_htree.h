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

#ifndef HTBC_HTREE_H_
#define	HTBC_HTREE_H_

#define	HTREE_MAXNAMLEN				255

#define	HTREE_LEGACY				0
#define	HTREE_HALF_MD4				1
#define	HTREE_TEA					2
#define	HTREE_LEGACY_UNSIGNED		3
#define	HTREE_HALF_MD4_UNSIGNED		4
#define	HTREE_TEA_UNSIGNED			5

#define	HTREE_EOF 					0x7FFFFFFF

struct htree_direct {
	uint32_t 					h_ino;						/* inode number of entry */
	uint16_t 					h_reclen;					/* length of this record */
	uint8_t  					h_namlen;					/* length of string in d_name */
	uint8_t  					h_type;						/* file type */
	char        				h_name[HTREE_MAXNAMLEN];	/* name with length<=HTREE_MAXNAMLEN */
};

struct htree_count {
	uint16_t 					h_entries_max;
	uint16_t 					h_entries_num;
};

struct htree_entry {
	uint32_t 					h_hash;
	uint32_t 					h_blk;
};

struct htree_root_info {
	uint32_t 					h_reserved1;
	uint8_t  					h_hash_version;
	uint8_t  					h_info_len;
	uint8_t  					h_ind_levels;
	uint8_t 	 				h_reserved2;
};

struct htree_root {
	struct htree_direct 		h_dot;
	char 						h_dot_name[4];
	struct htree_direct 		h_dotdot;
	char 						h_dotdot_name[4];
	struct htree_root_info 		h_info;
	struct htree_entry 			h_entries[0];
};

struct htree_node {
	struct htree_direct 		h_fake_dirent;
	struct htree_htree_entry 	h_entries[0];
};

struct htree_lookup_level {
	struct buf 					*h_bp;
	struct htree_entry 			*h_entries;
	struct htree_entry 			*h_entry;
};

struct htree_lookup_info {
	struct htree_lookup_level 	h_levels[2];
	uint32_t 					h_levels_num;
};

struct htree_lookup_results {
	int32_t	  					hlr_count;				/* Size of free slot in directory. */
	int32_t	  					hlr_endoff;				/* End of useful stuff in directory. */
	int32_t	  					hlr_diroff;				/* Offset in dir, where we found last entry. */
	int32_t	  					hlr_offset;				/* Offset of free space in directory. */
	u_int32_t 					hlr_reclen;				/* Size of found directory entry. */
};

struct htree_sort_entry {
	uint16_t 					h_offset;
	uint16_t 					h_size;
	uint32_t 					h_hash;
};

/* file flags */
#define HTREE_INDEX		0x00001000 /* hash-indexed directory */
#define HTREE_EXTENTS	0x00080000 /* Inode uses extents */

/*
 * HTREE_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
#define	HTREE_DIR_PAD	4
#define	HTREE_DIR_ROUND	(HTREE_DIR_PAD - 1)
#define	HTREE_DIR_REC_LEN(namelen) \
    (((namelen) + 8 + HTREE_DIR_ROUND) & ~HTREE_DIR_ROUND)

/* EXT2FS metadatas are stored in little-endian byte order. These macros
 * helps reading theses metadatas
 */
#if BYTE_ORDER == LITTLE_ENDIAN
#	define h2fs16(x) (x)
#	define fs2h16(x) (x)
#endif

static off_t	htree_get_block(struct htree_entry *ep);
static void 	htree_release(struct htree_lookup_info *info);
static uint16_t htree_get_limit(struct htree_entry *ep);
static uint32_t htree_root_limit(struct htbc_inode *ip, int len);
static uint16_t htree_get_count(struct htree_entry *ep);
static uint32_t htree_get_hash(struct htree_entry *ep);
static void 	htree_set_block(struct htree_entry *ep, uint32_t blk);
static void 	htree_set_count(struct htree_entry *ep, uint16_t cnt);
static void 	htree_set_hash(struct htree_entry *ep, uint32_t hash);
static void 	htree_set_limit(struct htree_entry *ep, uint16_t limit);
static uint32_t htree_node_limit(struct htbc_inode *ip);
static int 		htree_append_block(struct vnode *vp, char *data, struct componentname *cnp, uint32_t blksize);
static int 		htree_writebuf(struct htree_lookup_info *info);
static void 	htree_insert_entry_to_level(struct htree_lookup_level *level, uint32_t hash, uint32_t blk);
static void 	htree_insert_entry(struct htree_lookup_info *info, uint32_t hash, uint32_t blk);
static int 		htree_cmp_sort_entry(const void *e1, const void *e2);
static void 	htree_append_entry(char *block, uint32_t blksize, struct htree_direct *last_entry, struct htree_direct *new_entry);
static int 		htree_split_dirblock(char *block1, char *block2, uint32_t blksize, uint32_t *hash_seed, uint8_t hash_version, uint32_t *split_hash, struct htree_direct *entry);
int 			htree_create_index(struct vnode *vp, struct componentname *cnp, struct htree_direct *new_entry);
int 			htree_add_entry(struct vnode *dvp, struct htree_direct *entry, struct componentname *cnp, size_t newentrysize);
static int 		htree_check_next(struct htbc_inode *ip, uint32_t hash, const char *name, struct htree_lookup_info *info);
static int 		htree_find_leaf(struct htbc_inode *ip, const char *name, int namelen, uint32_t *hash, uint8_t *hash_ver, struct htree_lookup_info *info);
int 			htree_lookup(struct htbc_inode *ip, const char *name, int namelen, struct buf **bpp, int *entryoffp, int32_t *offp, int32_t *prevoffp, int32_t *endusefulp, struct htbc_hi_searchslot *ss);

#endif /* HTBC_HTREE_H_ */
