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

#ifndef _UFS_EFS_EFS_H
#define _UFS_EFS_EFS_H

/* BSD EFS: An Extent-Based Filesystem */

/* 32-Bit */
#define	EFS1_MAGIC
#define	EFS1_VERSION  	1

/* 64-Bit */
#define	EFS2_MAGIC
#define	EFS2_VERSION  	2

/* efs superblock */
struct efs {
	u_int32_t 		efs_magic;		/* magic number */
	u_int32_t 		efs_version;	/* version number */

	u_int32_t 		efs_size;		/* number of blocks */
	u_int32_t 		efs_esize;		/* number of blocks per extent */
	u_int32_t 		efs_dsize;		/* number of disk blocks in fs */
	u_int32_t 		efs_bsize;		/* block size */
	u_int32_t 		efs_fsize;		/* frag size */
	u_int32_t 		efs_frags;		/* number of frags */

	u_int32_t		efs_bfree;		/* number of free disk blocks */
	u_int32_t		efs_bmask;
	u_int32_t		efs_bshift;
	u_int32_t		efs_fshift;


	struct vnode	*efs_vnode;
};

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
