/*	$NetBSD: ext2fs_htree.h,v 1.1 2016/06/24 17:21:30 christos Exp $	*/

/*
 * Copyright (c) 2020 Martin Kelly
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

/*
 * HTBC (aka HTree Blockchain):
 * Design & Goals:
 * The HTree Blockchain is forked from BSD's ext2fs htree and extent implementation.
 * Provide a VFS layer blockchain that can augment/improve existing filesystem/s;
 * HTBC can be easily defined as having two layers, similar to NetBSD's WAPBL or Linux's JBD for EXT3/4.
 * The VFS layer and the FS layer.
 *
 * VFS Layer: (To Be Implemented)
 * HTBC Log Component:
 *	- Provide the facilities for log-based filesystems.
 *
 *	HTBC Journal Component:
 *	- Provide the facilities for journal-based filesystems.
 *
 * LFS: HTBC-Based Log: (To Be Implemented)
 * Provide
 * - Caching
 * - Defragment
 * - Checksums
 *
 * UFS/FFS: HTBC-Based Journal: (To Be Implemented)
 * Provide
 * - JFS/XFS like Journal abilities
 * - External Journal
 * - Log
 * - Metadata only
 * - Checksums
 * - Caching
 */

#ifndef SYS_HTBC_H_
#define SYS_HTBC_H_

#include <miscfs/specfs/specdev.h>

#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/buf.h>

/****************************************************************/
/* HTBC Blockchain Layout */

struct htbc_hchain {
	CIRCLEQ_ENTRY(htbc_hchain)	hc_entry;			/* a list of chain entries */
	struct htree_root			*hc_hroot;			/* htree root per block */
	struct ht_transaction		*hc_trans;			/* pointer to transaction info */

	const char					*hc_name;			/* name of chain entry */
	int							hc_len;

	struct lock_object			hc_lock;			/* lock on chain */
	int							hc_flags;			/* flags */
	int							hc_refcnt;			/* number of entries in this chain */
};

struct htbc_htransaction {
	int32_t						ht_hash;			/* size of hash for this transaction */
	int32_t						ht_reclen;			/* length of this record */
	uint32_t					ht_flag;			/* flags */
	struct timespec				*ht_timespec;		/* timestamp */
	uint32_t					ht_type;
	int32_t						ht_len;
	uint32_t					ht_checksum;
	uint32_t					ht_generation;
	uint32_t					ht_version;
    int32_t						ht_atime;			/* Last access time. */
    int32_t						ht_atimesec;		/* Last access time. */
    int32_t						ht_mtime;			/* Last modified time. */
    int32_t						ht_mtimesec;		/* Last modified time. */
    int32_t						ht_ctime;			/* Last inode change time. */
    int32_t						ht_ctimesec;		/* Last inode change time. */

	union ht_u_ino {
		ino_t 					u_ino;
		mode_t					u_imode;
	} ht_ino;
	int							ht_inocnt;
	int							ht_dev_bshift;
	int							ht_dev_bmask;
};

#define HASH_VERSION			HTREE_HALF_MD4					/* make configurable */
#define HASH_SEED(hash_seed) 	(htbc_random_hash_seed(hash_seed));
#define HASH_MAJOR 				(prospector32(random()))
#define HASH_MINOR 				(prospector32(random()))

#define NOTRANSACTION			0x01

/****************************************************************/
/* HTBC On Disk Layouts */

/* null entry (on disk) */
struct htbc_hc_null {
	uint32_t					hc_type;
	int32_t						hc_len;
	uint8_t						hc_spare[0];
};

/* list of blocks (on disk) */
struct htbc_hc_blocklist {
	u_int32_t 					hc_type;
	int32_t 					hc_len;
	int32_t						hc_blkcount;
	struct {
		int64_t					hc_daddr;
		int32_t					hc_unused;
		int32_t					hc_dlen;
	} hc_blocks[0];
};

/* list of inodes (on disk) */
struct htbc_hc_inodelist {
	uint32_t					hc_type;
	int32_t						hc_len;
	int32_t						hc_inocnt;
	int32_t						hc_clear; 				/* set if previously listed inodes hould be ignored */

	struct {
		uint32_t 				hc_inumber;
		uint32_t 				hc_imode;
	} hc_inodes[0];
};

/****************************************************************/
/* HTBC HTree Inode Layout */

struct htbc_inode {
    struct vnode 				*hi_vnode;				/* Vnode associated with this inode. */
    struct vnode 				*hi_devvp;				/* Vnode for block I/O. */
    struct htbc_mfs 			*hi_mfs;	    		/* in memory data */

	u_int32_t					hi_blocks;				/* Blocks actually held. */
	u_int64_t   				hi_size;				/* File byte count. */
	u_int32_t   				hi_flag;	    		/* flags, see below */
    u_int32_t   				hi_sflags;				/* Status flags (chflags) */
    int32_t	  					hi_count;				/* Size of free slot in directory. */

    uint32_t  					hi_hash_seed[4];		/* HTREE hash seed */
    char      					hi_def_hash_version;	/* Default hash version to use */

    /* XXX: */
    ino_t	  					hi_number;				/* The identity of the inode. */
    uint16_t					hi_mode;				/* IFMT, permissions; see below. */
    uint32_t  					hi_icount;				/* Inode count */
    uint32_t  					hi_bcount;				/* blocks count */
	uint32_t  					hi_first_dblock;		/* first data block */
	uint32_t  					hi_log_bsize;			/* block size = 1024*(2^e2fs_log_bsize) */
};

struct htbc_inode_ext {
	daddr_t 					hi_last_lblk;			/* last logical block allocated */
	daddr_t 					hi_last_blk;			/* last block allocated on disk */
	struct htbc_extent_cache 	hi_ext_cache; 			/* cache for htbc extent */
};

struct htbc_mfs {
	struct htbc_inode			hi_inode;
    int8_t	        			hi_uhash;				/* if hash should be signed, 0 if not */
	int32_t	        			hi_bsize;				/* block size */
	int32_t 					hi_bshift;				/* ``lblkno'' calc of logical blkno */
	int32_t 					hi_bmask;				/* ``blkoff'' calc of blk offsets */
	int64_t 					hi_qbmask;				/* ~fs_bmask - for use with quad size */
	int32_t						hi_fsbtodb;				/* fsbtodb and dbtofsb shift constant */
    struct htbc_inode_ext 		hi_ext;
};

struct htbc_searchslot {
	enum htbc_slotstatus		hi_slotstatus;
	int32_t 					hi_slotoffset;			/* offset of area with free space */
	int 						hi_slotsize;			/* size of area at slotoffset */
	int 						hi_slotfreespace;		/* amount of space free in slot */
	int 						hi_slotneeded;			/* sizeof the entry we are seeking */
	int32_t						hi_slotcount;			/* Size of free slot in directory. */
};

enum htbc_slotstatus {
	NONE,
	COMPACT,
	FOUND
};

/* Convert between inode pointers and vnode pointers. */
#define VTOHTI(vp)			((struct htbc_inode *)(vp)->v_data)
#define HTITOV(ip)			((ip)->hi_vnode)

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
	uint32_t 					e_blk;			/* first logical block */
	uint16_t 					e_len;			/* number of blocks */
	uint16_t 					e_start_hi;		/* high 16 bits of physical block */
	uint32_t 					e_start_lo;		/* low 32 bits of physical block */
};

/*
 * Extent index on disk.
 */
struct htbc_extent_index {
	uint32_t 					ei_blk;			/* indexes logical blocks */
	uint32_t 					ei_leaf_lo;		/* points to physical block of the next level */
	uint16_t 					ei_leaf_hi;		/* high 16 bits of physical block */
	uint16_t 					ei_unused;
};

/*
 * Extent tree header.
 */
struct htbc_extent_header {
	uint16_t 					eh_magic;		/* magic number: 0xf30a */
	uint16_t 					eh_ecount;		/* number of valid entries */
	uint16_t 					eh_max;			/* capacity of store in entries */
	uint16_t 					eh_depth;		/* the depth of extent tree */
	uint32_t 					eh_gen;			/* generation of extent tree */
};

/*
 * Save cached extent.
 */
struct htbc_extent_cache {
	daddr_t						ec_start;		/* extent start */
	uint32_t 					ec_blk;			/* logical block */
	uint32_t 					ec_len;
	uint32_t 					ec_type;
};

/*
 * Save path to some extent.
 */
struct htbc_extent_path {
	uint16_t 					ep_depth;
	struct buf 					*ep_bp;
	bool 						ep_is_sparse;
	union {
		struct htbc_extent 		ep_sparse_ext;
		struct htbc_extent 		*ep_ext;
	};
	struct htbc_extent_index 	*ep_index;
	struct htbc_extent_header 	*ep_header;
};

int						htbc_ext_in_cache(struct htbc_inode *, daddr_t, struct htbc_extent *);
void					htbc_ext_put_cache(struct htbc_inode *, struct htbc_extent *, int);
struct htbc_extent_path *htbc_ext_find_extent(struct htbc_mfs *fs, struct htbc_inode *, daddr_t, struct htbc_extent_path *);

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
#define	htbc_blksize(fs, ip, lbn) 	((fs)->hi_bsize)			/* missing parts of equation */

/* place in sys/buf.h */
/*
 * These flags are kept in b_cflags (owned by buffer cache).
 */
#define	BC_AGE		0x00000001	/* Move to age queue when I/O done. */
#define	BC_BUSY		0x00000010	/* I/O in progress. */
#define BC_SCANNED	0x00000020	/* Block already pushed during sync */
#define	BC_INVAL	0x00002000	/* Does not contain valid info. */
#define	BC_NOCACHE	0x00008000	/* Do not cache block after use. */
#define	BC_WANTED	0x00800000	/* Process wants this buffer. */
#define	BC_VFLUSH	0x04000000	/* Buffer is being synced. */

/*
 * These flags are kept in b_oflags (owned by associated object).
 */
#define	BO_DELWRI	0x00000080	/* Delay I/O until buffer reused. */
#define	BO_DONE		0x00000200	/* I/O completed. */

#endif /* SYS_HTBC_H_ */
