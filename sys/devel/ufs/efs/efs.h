/*
 * The 3-Clause BSD License:
 * Copyright (c) 2024 Martin Kelly
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
 */
/*	$NetBSD: efs_sb.h,v 1.1 2007/06/29 23:30:29 rumble Exp $	*/

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

#ifndef _UFS_EFS_EFS_H
#define _UFS_EFS_EFS_H

/* BSD EFS: An Extent-Based Filesystem */

#define EFS_SBSIZE				8192		/* superblock size */
#define EFS_CHECKSUM_SIZE		(EFS_SBSIZE - 4)

#define EFS_BB_SHFT	            9
#define EFS_BB_SIZE	            (1 << EFS_BB_SHFT)

#define EFS_DIRECTEXTENTS 		12

/* 32-Bit */
#define	EFS1_MAGIC
#define	EFS1_VERSION  			1

#define EFS1_DINODE_SIZE		sizeof(struct ufs1_dinode)
#define EFS1_DINODES_PER_BB		(EFS_BB_SIZE / EFS1_DINODE_SIZE)

/* 64-Bit */
#define	EFS2_MAGIC
#define	EFS2_VERSION  			2

#define EFS2_DINODE_SIZE		sizeof(struct ufs2_dinode)
#define EFS2_DINODES_PER_BB		(EFS_BB_SIZE / EFS2_DINODE_SIZE)

/* efs superblock */
struct efs {
	u_int32_t 			efs_magic;		/* magic number */
	u_int32_t 			efs_version;	/* version number */

	u_int32_t 			efs_size;		/* number of blocks */
	u_int32_t 			efs_esize;		/* number of blocks per extent */
	u_int32_t 			efs_dsize;		/* number of disk blocks in fs */
	u_int32_t 			efs_bsize;		/* block size */
	u_int32_t 			efs_fsize;		/* frag size */
	u_int32_t 			efs_frags;		/* number of frags */

	u_int32_t			efs_bfree;		/* number of free disk blocks */
	u_int32_t			efs_bmask;
	u_int32_t			efs_bshift;
	u_int32_t			efs_fshift;

	/* Cylinder Groups */
	int32_t				efs_firstcg;	/* first cg offset (in bb) */
	int32_t				efs_cgfsize;	/* cg size (in bb) */
	int16_t				efs_cgisize;	/* inodes/cg (in bb) */
	int16_t				efs_sectors;	/* geom: sectors/track */
	int16_t				efs_heads;		/* geom: heads/cylinder (unused) */
	int16_t				efs_ncg;		/* num of cg's in the filesystem */

	u_int32_t 		 	efs_inopb;		/* inodes per block */

	u_int32_t 			efs_nindir;		/* indirect pointers per block */

	struct vnode	  	*efs_vnode;

	struct efs_bitmap	efs_extents[EFS_DIRECTEXTENTS];
	int32_t				efs_bmsize;		/* bitmap size (in bytes) */
	int32_t				efs_bmblock;	/* bitmap offset (grown fs) [1] */
};

/* NINDIR is the number of indirects in a file system block. */
#define	NINDIR(fs)	((fs)->efs_nindir)

/* INOPB is the number of inodes in a secondary storage block. */
#define	INOPB(fs)	((fs)->efs_inopb)

#ifdef notyet
/* ufs */
struct inode {
	union {							/* Associated filesystem. */
		struct efs	*efs:			/* EFS */
	} inode_u;
};

struct ufs1_dinode {
	union {
		int32_t	 	numextents;		/* 28: number of extents in file [3] */
	} di_u;
};

struct ufs2_dinode {
	union {
		int32_t	 	numextents;		/* 28: number of extents in file [3] */
	} di_u;
};
#endif

#endif /* _UFS_EFS_EFS_H */
