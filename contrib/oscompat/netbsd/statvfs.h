/*	$NetBSD: statvfs.h,v 1.5 2004/10/06 04:30:04 lukem Exp $	 */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * XXX This header file exists to support modern BSD/unix-like OS's and libraries.
 * The 2.11BSD kernel does not require or use statvfs. 
 * Note: Including it in the kernel would be unnecessary.
 */

#ifndef _SYS_STATVFS_H_
#define _SYS_STATVFS_H_

#include <sys/cdefs.h>

#include <sys/stdint.h>
#include <machine/ansi.h>
#include <sys/ansi.h>

#define _VFS_NAMELEN_MAX 	255	/* maximum filename length */
#define _VFS_MFSNAMELEN 	16	/* length of fs type name, including null */
#define _VFS_MNAMELEN		90	/* length of buffer for returned name */

#ifndef	fsblkcnt_t
typedef	__fsblkcnt_t	fsblkcnt_t;	/* fs block count (statvfs) */
#define	fsblkcnt_t	__fsblkcnt_t
#endif

#ifndef	fsfilcnt_t
typedef	__fsfilcnt_t	fsfilcnt_t;	/* fs file count */
#define	fsfilcnt_t	__fsfilcnt_t
#endif

#ifndef	uid_t
typedef	__uid_t		uid_t;		/* user id */
#define	uid_t		__uid_t
#endif

struct statvfs {
	unsigned long	f_flag;			/* copy of mount exported flags */
	unsigned long	f_bsize;		/* file system block size */
	unsigned long	f_frsize;		/* fundamental file system block size */
	unsigned long	f_iosize;		/* optimal file system block size */

	fsblkcnt_t	f_blocks;		/* number of blocks in file system, */
									/*   (in units of f_frsize) */
	fsblkcnt_t	f_bfree;		/* free blocks avail in file system */
	fsblkcnt_t	f_bavail;		/* free blocks avail to non-root */
	fsblkcnt_t	f_bresvd;		/* blocks reserved for root */

	fsfilcnt_t	f_files;		/* total file nodes in file system */
	fsfilcnt_t	f_ffree;		/* free file nodes in file system */
	fsfilcnt_t	f_favail;		/* free file nodes avail to non-root */
	fsfilcnt_t	f_fresvd;		/* file nodes reserved for root */

	fsid_t		f_fsidx;		/* NetBSD compatible fsid */
	unsigned long	f_fsid;			/* Posix compatible fsid */
	unsigned long	f_namemax;		/* maximum filename length */
	uid_t		f_owner;		/* user that mounted the file system */

	uint32_t	f_spare[4];		/* spare space */

	char		f_fstypename[_VFS_MNAMELEN]; 	/* fs type name */
	char		f_mntonname[_VFS_MNAMELEN];  	/* directory on which mounted */
	char		f_mntfromname[_VFS_MNAMELEN];  	/* mounted file system */
};

/*
 * f_flag definitions
 */
#define	ST_RDONLY	MNT_RDONLY	/* fs is read-only */
#define	ST_NOSUID	MNT_NOSUID	/* ST_ISUID or ST_ISGID not supported */

__BEGIN_DECLS
int	fstatvfs(int, struct statvfs *);
int	statvfs(const char *, struct statvfs *);
__END_DECLS

#endif /* _SYS_STATVFS_H_ */
