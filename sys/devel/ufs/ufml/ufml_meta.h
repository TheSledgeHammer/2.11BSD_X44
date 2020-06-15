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

#include <sys/ufs/ufs/inode.h>
#include <sys/ufs/mfs/mfsnode.h>

struct ufml_metadata {
	struct ufml_node	*ufml_node;

	char 				*ufml_name; 		/* name */
	u_quad_t			ufml_size; 			/* size */
	u_long				ufml_flags;			/* flags defined for file */
	struct timespec		ufml_atime;			/* time of last access */
	struct timespec		ufml_mtime;			/* time of last modification */
	struct timespec		ufml_ctime;			/* time file changed */

	enum ufml_fstype 	ufml_filesystem; 	/* Supported fs types (for create) */
	enum ufml_archtype 	ufml_archive;		/* archive types (for create) */
	enum ufml_comptype 	ufml_compress;		/* compression types (for create) */
	enum ufml_enctype 	ufml_encrypt;		/* encryption types (for create) */
};

/* UFML Supported File system Types */
enum ufml_fstype { UFML_UFS, UFML_FFS, UFML_MFS, UFML_LFS };

/* UFML Supported Archive Formats */
enum ufml_archtype { UFML_AR, UFML_CPIO, UFML_TAR };

/* UFML Supported Compression Formats */
enum ufml_comptype { UFML_BZIP2, UFML_GZIP, UFML_LZIP, UFML_LZMA, UFML_XZ };

/* UFML Supported Encryption Algorithms */
enum ufml_enctype { UFML_TWOFISH };

/* Macros to other supported filesystems */
#define UFMLTOUFS(vp)	(VTOI(vp))			/* UFS */
#define UFMLTOFFS(vp)	(VTOI(vp)->i_fs)	/* FFS */
#define UFMLTOMFS(vp)	(VTOMFS(vp))		/* MFS */
#define UFMLTOLFS(vp)	(VTOI(vp)->i_lfs)	/* LFS */

#endif /* UFS_UFML_META_H_ */
