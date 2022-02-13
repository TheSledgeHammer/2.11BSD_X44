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
 */

#ifndef UFS_UFML_META_H_
#define UFS_UFML_META_H_

#include <ufs/ufs/inode.h>
#include <ufs/mfs/mfsnode.h>

struct ufml_metadata {
	struct ufml_node	*ufml_node;

	char 				*ufml_name; 		/* name */
	u_quad_t			ufml_size; 			/* size */
	u_long				ufml_flag;			/* flag see below */
	struct timespec		ufml_atime;			/* time of last access */
	int32_t				ufml_atimensec;		/* Last access time. */
	struct timespec		ufml_mtime;			/* time of last modification */
	int32_t				ufml_mtimensec;		/* Last modified time. */
	struct timespec		ufml_ctime;			/* time file changed */
	int32_t				ufml_ctimensec;		/* Last inode change time. */
	uint64_t	    	ufml_modrev;	    /* modrev for NFSv4 */
	enum ufml_fstype 	ufml_filesystem; 	/* Supported fs types */
	enum ufml_archtype 	ufml_archive;		/* archive types */
	enum ufml_comptype 	ufml_compress;		/* compression types */
	enum ufml_enctype 	ufml_encrypt;		/* encryption types */
};

/* These flags are kept in ufml_flag. */
#define	IN_ACCESS			0x0001		/* Access time update request. */
#define	IN_CHANGE			0x0002		/* Inode change time update request. */
#define	IN_UPDATE			0x0004		/* Modification time update request. */
#define	IN_MODIFIED			0x0008		/* Inode has been modified. */
#define	IN_RENAME			0x0010		/* Inode is being renamed. */
#define	IN_SHLOCK			0x0020		/* File has shared lock. */
#define	IN_EXLOCK			0x0040		/* File has exclusive lock. */
#define	IN_HASHED			0x0080		/* Inode is on hash list */
#define	IN_LAZYMOD			0x0100		/* Modified, but don't write yet. */

/* UFML Supported File system Types */
enum ufml_fstype { UFML_UFS, UFML_FFS, UFML_MFS, UFML_LFS };

/* UFML Supported Archive Formats */
enum ufml_archtype { UFML_AR, UFML_CPIO, UFML_TAR };

/* UFML Supported Compression Formats */
enum ufml_comptype { UFML_BZIP2, UFML_GZIP, UFML_LZIP, UFML_LZMA, UFML_XZ };

/* UFML Supported Encryption Algorithms */
enum ufml_enctype { UFML_TWOFISH };

/* Macros to other supported filesystems */
#define UFMLTOUFS(vp)		(VTOI(vp))			/* UFS */
#define UFMLTOFFS(vp)		(VTOI(vp)->i_fs)	/* FFS */
#define UFMLTOMFS(vp)		(VTOMFS(vp))		/* MFS */
#define UFMLTOLFS(vp)		(VTOI(vp)->i_lfs)	/* LFS */

#endif /* UFS_UFML_META_H_ */
