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

#ifndef SYS_HTBC_H_
#define SYS_HTBC_H_

#include <miscfs/specfs/specdev.h>

#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/buf.h>

/****************************************************************/
/* HTBC layout */

struct htbc_hc_null {
	uint32_t	hc_type;
	int32_t		hc_len;
	uint8_t		hc_spare[0];
};

struct htbc_hc_header {
	uint32_t	hc_type;
	int32_t		hc_len;
	uint32_t	hc_checksum;
	uint32_t	hc_generation;
	int32_t		hc_fsid[2];
	uint64_t	hc_time;
	uint32_t	hc_timensec;
	uint32_t	hc_version;

	int64_t		hc_head;
	int64_t		hc_tail;
};

/* list of blocks (on disk) */
struct htbc_hc_blocklist {
	u_int32_t 	hc_type;
	int32_t 	hc_len;
	int32_t		hc_blkcount;
	struct {
		int64_t	hc_daddr;
		int32_t	hc_unused;
		int32_t	hc_dlen;
	} hc_blocks[0];
};

/* list of inodes (on disk) */
struct htbc_hc_inodelist {
	uint32_t	hc_type;
	int32_t		hc_len;
	int32_t		hc_inocnt;
	int32_t		hc_clear;

	struct {
		uint32_t hc_inumber;
		uint32_t hc_imode;
	} hc_inodes[0];
};

struct htbc_entry {
	struct htbc 				*he_htbc;
	CIRCLEQ_ENTRY(htbc_entry) 	he_entries;
	size_t 						he_bufcount;
	size_t 						he_reclaimable_bytes;
	int							he_error;
};

/****************************************************************/
/* HTBC HTree Inode Layout (From NetBSD ext2fs) */

/* Work in Progress: Needs changes to operate on other Filesystems, primarily become vnode based */
struct htbc_inode_ext {
	daddr_t 					hi_last_lblk;			/* last logical block allocated */
	daddr_t 					hi_last_blk;			/* last block allocated on disk */
	struct htbc_extent_cache 	hi_ext_cache; 			/* cache for htbc extent */
};

struct htbc_inode {
	u_int32_t					hi_blocks;
	u_int64_t   				hi_size;				/* File byte count. */
	u_int32_t   				hi_flag;	    		/* flags, see below */
    u_int32_t   				hi_sflags;				/* Status flags (chflags) */
    struct htbc_hi_mfs 			*hi_mfs;	    		/* htbc_hi_mfs */
    int32_t	  					hi_count;				/* Size of free slot in directory. */
    struct vnode 				*hi_vnode;
};

struct htbc_hi_mfs {
	struct htbc_hi_fs   		hi_fs;
    int8_t	        			hi_uhash;				/* if hash should be signed, 0 if not */
	int32_t	        			hi_bsize;				/* block size */
	int32_t 					hi_bshift;				/* ``lblkno'' calc of logical blkno */
	int32_t 					hi_bmask;				/* ``blkoff'' calc of blk offsets */
	int64_t 					hi_qbmask;				/* ~fs_bmask - for use with quad size */
	int32_t						hi_fsbtodb;				/* fsbtodb and dbtofsb shift constant */
    struct htbc_inode_ext 		hi_ext;
};

struct htbc_hi_fs {
	uint32_t  					hi_hash_seed[4];		/* HTREE hash seed */
    char      					hi_def_hash_version;	/* Default hash version to use */
};

struct htbc_hi_searchslot {
	enum htbc_hi_slotstatus		hi_slotstatus;
	int32_t 					hi_slotoffset;			/* offset of area with free space */
	int 						hi_slotsize;			/* size of area at slotoffset */
	int 						hi_slotfreespace;		/* amount of space free in slot */
	int 						hi_slotneeded;			/* sizeof the entry we are seeking */
	int32_t						hi_slotcount;			/* Size of free slot in directory. */
};

enum htbc_hi_slotstatus {
	NONE,
	COMPACT,
	FOUND
};

/* Convert between inode pointers and vnode pointers. */
#define VTOHTI(vp)	((struct htbc_inode *)(vp)->v_data)
#define HTITOV(ip)	((ip)->hi_vnode)

/****************************************************************/
/* HTBC Extents */

#define	HTBC_EXT_MAGIC  	0xf30a

#define	HTBC_EXT_CACHE_NO	0
#define	HTBC_EXT_CACHE_GAP	1
#define	HTBC_EXT_CACHE_IN	2

/*
 * HTBC file system extent on disk.
 */
struct htbc_extent {
	uint32_t e_blk;			/* first logical block */
	uint16_t e_len;			/* number of blocks */
	uint16_t e_start_hi;	/* high 16 bits of physical block */
	uint32_t e_start_lo;	/* low 32 bits of physical block */
};

/*
 * Extent index on disk.
 */
struct htbc_extent_index {
	uint32_t ei_blk;		/* indexes logical blocks */
	uint32_t ei_leaf_lo;	/* points to physical block of the next level */
	uint16_t ei_leaf_hi;	/* high 16 bits of physical block */
	uint16_t ei_unused;
};

/*
 * Extent tree header.
 */
struct htbc_extent_header {
	uint16_t eh_magic;		/* magic number: 0xf30a */
	uint16_t eh_ecount;		/* number of valid entries */
	uint16_t eh_max;		/* capacity of store in entries */
	uint16_t eh_depth;		/* the depth of extent tree */
	uint32_t eh_gen;		/* generation of extent tree */
};

/*
 * Save cached extent.
 */
struct htbc_extent_cache {
	daddr_t	ec_start;		/* extent start */
	uint32_t ec_blk;		/* logical block */
	uint32_t ec_len;
	uint32_t ec_type;
};

/*
 * Save path to some extent.
 */
struct htbc_extent_path {
	uint16_t 	ep_depth;
	struct buf 	*ep_bp;
	bool 		ep_is_sparse;
	union {
		struct htbc_extent 	ep_sparse_ext;
		struct htbc_extent 	*ep_ext;
	};
	struct htbc_extent_index 	*ep_index;
	struct htbc_extent_header 	*ep_header;
};

int						htbc_ext_in_cache(struct htbc_inode *, daddr_t, struct htbc_extent *);
void					htbc_ext_put_cache(struct htbc_inode *, struct htbc_extent *, int);
struct htbc_extent_path *htbc_ext_find_extent(struct htbc_hi_mfs *fs, struct htbc_inode *, daddr_t, struct htbc_extent_path *);

/****************************************************************/
/* HTBC htree Hash */

/* F, G, and H are MD4 functions */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/****************************************************************/
/* Generic HTBC functions */

void 	htbc_init(void);
int 	htbc_start(struct htbc **);
int 	htbc_stop(struct htbc *, int);
int		htbc_begin(struct htbc *, const char *, int);
int 	htbc_end(struct htbc *);
void 	htbc_register_inode(struct htbc *, ino_t, mode_t);
void 	htbc_unregister_inode(struct htbc *, ino_t, mode_t);

/* Supply this to provide i/o support */
int 	htbc_write(void *, size_t, struct vnode *, daddr_t);
int 	htbc_read(void *, size_t, struct vnode *, daddr_t);


#define	htbc_fsbtodb(fs, b)  		((daddr_t)(b) << (fs)->hi_fsbtodb)
/* calculates (loc % fs->hi_bsize) */
#define htbc_blkoff(fs, loc)	 	((loc) & (fs)->hi_qbmask)
/* calculates (loc / fs->hi_bsize) */
#define htbc_lblkno(fs, loc)		((loc) >> (fs)->hi_bshift)
#define	htbc_blksize(fs, ip, lbn) 	((fs)->hi_bsize)

#endif /* SYS_HTBC_H_ */
