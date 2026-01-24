/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	@(#)stat.h	8.12 (Berkeley) 6/16/95
 */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)stat.h	7.1.5 (2.11BSD) 1996/09/20
 */

#ifndef	_SYS_STAT_H_
#define	_SYS_STAT_H_

#include <sys/time.h>
#include <sys/types.h>		/* XXX */

#ifndef _POSIX_SOURCE
struct ostat {
	u_int16_t 		st_dev;			/* inode's device */
	ino_t	  		st_ino;			/* inode's number */
	mode_t	  		st_mode;		/* inode protection mode */
	nlink_t	  		st_nlink;		/* number of hard links */
	u_int16_t 		st_uid;			/* user ID of the file's owner */
	u_int16_t 		st_gid;			/* group ID of the file's group */
	u_int16_t 		st_rdev;		/* device type */
	int32_t	  		st_size;		/* file size, in bytes */
	struct	timespec 	st_atimespec;		/* time of last access */
	struct	timespec 	st_mtimespec;		/* time of last data modification */
	struct	timespec 	st_ctimespec;		/* time of last file status change */
	struct	timespec  	st_birthtimespec;	/* time of creation */
	int32_t	  		st_blksize;		/* optimal blocksize for I/O */
	int32_t	  		st_blocks;		/* blocks allocated for file */
	u_int32_t 		st_flags;		/* user defined flags for file */
	u_int32_t 		st_gen;			/* file generation number */
};
#endif /* !_POSIX_SOURCE */

struct stat {
	dev_t			st_dev;			/* inode's device */
	ino_t			st_ino;			/* inode's number */
	mode_t	 		st_mode;		/* inode protection mode */
	nlink_t			st_nlink;		/* number of hard links */
	uid_t			st_uid;			/* user ID of the file's owner */
	gid_t			st_gid;			/* group ID of the file's group */
	dev_t			st_rdev;		/* device type */
	off_t			st_size;		/* file size, in bytes */
#ifdef _KERNEL
	struct	timespec	st_atime;		/* time of last access */
	struct	timespec	st_mtime;		/* time of last data modification */
	struct	timespec 	st_ctime;		/* time of last file status change */
	struct	timespec  	st_birthtime;		/* time of creation */
#else /* !_KERNEL */
#ifndef _POSIX_SOURCE 
	struct  timespec    	st_atimespec;		/* time of last access */
	struct  timespec    	st_mtimespec;		/* time of last data modification */
	struct  timespec    	st_ctimespec;		/* time of last file status change */
	struct  timespec    	st_birthtimespec;	/* time of creation */
#else /* _POSIX_SOURCE */
	time_t	            	st_atime;		/* time of last access */
	long	            	st_atimensec;		/* nsec of last access */
	time_t	            	st_mtime;		/* time of last data modification */
	long	            	st_mtimensec;		/* nsec of last data modification */
	time_t	            	st_ctime;		/* time of last file status change */
	long	            	st_ctimensec;		/* nsec of last file status change */
	time_t	            	st_birthtime;		/* time of creation */
	long	            	st_birthtimensec;	/* nsec of time of creation */
#endif /* !_POSIX_SOURCE */
#endif /* _KERNEL */
	unsigned long		st_blksize;		/* optimal blocksize for I/O */
	quad_t			st_blocks;		/* blocks allocated for file */
	unsigned long		st_flags;		/* user defined flags for file */
	unsigned long		st_gen;			/* file generation number */
	int			st_spare1;
	int			st_spare2;
	long			st_spare3;
	quad_t			st_spare4[3];
};

#ifdef _KERNEL
/* timespec seconds (tv_sec) */
/* New macros */
#define st_atimesec 		st_atime.tv_sec
#define st_mtimesec 		st_mtime.tv_sec
#define st_ctimesec 		st_ctime.tv_sec
#define st_birthtimesec     	st_birthtime.tv_sec

/* Old macros (To be replaced) */
#define st_atim 		st_atimesec
#define st_mtim 		st_mtimesec
#define st_ctim 		st_ctimesec
#define st_birthtim         	st_birthtimesec

/* timespec nanoseconds (tv_nsec) */
#define st_atimensec	    	st_atime.tv_nsec
#define st_mtimensec	    	st_mtime.tv_nsec
#define st_ctimensec	    	st_ctime.tv_nsec
#define st_birthtimensec    	st_birthtime.tv_nsec

#else /* !_KERNEL */

#ifndef _POSIX_SOURCE
#define st_atime            	st_atimespec.tv_sec
#define st_atimensec	    	st_atimespec.tv_nsec
#define st_mtime            	st_mtimespec.tv_sec
#define st_mtimensec	    	st_mtimespec.tv_nsec
#define st_ctime            	st_ctimespec.tv_sec
#define st_ctimensec		st_ctimensec.tv_nsec
#define	st_birthtime		st_birthtimespec.tv_sec
#define st_birthtimensec    	st_birthtimespec.tv_nsec
#endif  /* !_POSIX_SOURCE */
#endif /* _KERNEL */

#define	S_IFMT	 0170000	/* type of file */
#define	S_IFDIR	 0040000	/* directory */
#define	S_IFMPC	 0030000	/* multiplexed char special */
#define	S_IFCHR	 0020000	/* character special */
#define	S_IFBLK	 0060000	/* block special */
#define	S_IFREG	 0100000	/* regular */
#define	S_IFLNK	 0120000	/* symbolic link */
#define	S_IFSOCK 0140000	/* socket */
#define	S_IFWHT  0160000	/* whiteout */
#define	S_ISUID	 0004000	/* set user id on execution */
#define	S_ISGID	 0002000	/* set group id on execution */
#define	S_ISVTX	 0001000	/* save swapped text even after use */
#define	S_IREAD	 0000400	/* read permission, owner */
#define	S_IWRITE 0000200	/* write permission, owner */
#define	S_IEXEC	 0000100	/* execute/search permission, owner */
#define	S_ISTXT	 0010000	/* sticky bit */

/*
 * Definitions of flags in mode that are 4.4 compatible.
 */

#define S_IFIFO 0010000		/* named pipe (fifo) - Not used by 2.11BSD */

#define S_IRWXU 0000700		/* RWX mask for owner */
#define S_IRUSR 0000400		/* R for owner */
#define S_IWUSR 0000200		/* W for owner */
#define S_IXUSR 0000100		/* X for owner */

#define S_IRWXG 0000070		/* RWX mask for group */
#define S_IRGRP 0000040		/* R for group */
#define S_IWGRP 0000020		/* W for group */
#define S_IXGRP 0000010		/* X for group */

#define S_IRWXO 0000007		/* RWX mask for other */
#define S_IROTH 0000004		/* R for other */
#define S_IWOTH 0000002		/* W for other */
#define S_IXOTH 0000001		/* X for other */

#define	S_ISDIR(m)	((m & S_IFMT) == S_IFDIR)	/* directory */
#define	S_ISCHR(m)	((m & S_IFMT) == S_IFCHR)	/* char special */
#define	S_ISBLK(m)	((m & S_IFMT) == S_IFBLK)	/* block special */
#define	S_ISREG(m)	((m & S_IFMT) == S_IFREG)	/* regular file */
#define	S_ISFIFO(m)	((m & S_IFMT) == S_IFIFO || \
			 	 	 (m & S_IFMT) == S_IFMT)	/* fifo or socket */

#define	S_ISLNK(m)	((m & S_IFMT) == S_IFLNK)	/* symbolic link */
#define	S_ISSOCK(m)	((m & S_IFMT) == S_IFSOCK)	/* socket */
#define	S_ISWHT(m)	((m & S_IFMT) == S_IFWHT)	/* whiteout */

#define	ACCESSPERMS	(S_IRWXU|S_IRWXG|S_IRWXO)	/* 0777 */
												/* 7777 */
#define	ALLPERMS	(S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO)
												/* 0666 */
#define	DEFFILEMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#define S_BLKSIZE	512		/* block size used in the stat struct */

/*
 * Definitions of flags stored in file flags word.  Different from 4.4 because
 * 2.11BSD only could afford a u_short for the flags.  It is not a great
 * inconvenience since there are still 5 bits in each byte available for
 * future use.
 *
 * Super-user and owner changeable flags.
 */
#define	UF_SETTABLE		0x00ff		/* mask of owner changeable flags */
#define	UF_NODUMP		0x0001		/* do not dump file */
#define	UF_IMMUTABLE		0x0002		/* file may not be changed */
#define	UF_APPEND		0x0004		/* writes to file may only append */
#define UF_OPAQUE		0x0008		/* directory is opaque wrt. union */

#define	UF_SYSTEM		0x0080		/* Windows system file bit */
#define	UF_SPARSE		0x0100		/* sparse file */
#define	UF_OFFLINE		0x0200		/* file is offline */
#define	UF_REPARSE		0x0400		/* Windows reparse point file bit */
#define	UF_ARCHIVE		0x0800		/* file needs to be archived */
#define	UF_READONLY		0x1000		/* Windows readonly file bit */
/* This is the same as the MacOS X definition of UF_HIDDEN. */
#define	UF_HIDDEN		0x8000		/* file is hidden */

/*
 * Super-user changeable flags.
 */
#define	SF_SETTABLE		0xff00		/* mask of superuser changeable flags */
#define	SF_ARCHIVED		0x0100		/* file is archived */
#define	SF_IMMUTABLE	0x0200		/* file may not be changed */
#define	SF_APPEND		0x0400		/* writes to file may only append */

#ifdef _KERNEL
/*
 * Shorthand abbreviations of above.
 */
#define	OPAQUE			(UF_OPAQUE)
#define	APPEND			(UF_APPEND | SF_APPEND)
#define	IMMUTABLE		(UF_IMMUTABLE | SF_IMMUTABLE)
#endif

#ifndef _KERNEL
#include <sys/cdefs.h>

__BEGIN_DECLS
int		chmod(const char *, mode_t);
int		fstat(int, struct stat *);
int		mkdir(const char *, mode_t);
int		mkfifo(const char *, mode_t);
int		stat(const char *, struct stat *);
mode_t	umask(mode_t);
//#ifndef _POSIX_SOURCE
int		chflags(const char *, u_long);
int		fchflags(int, u_long);
int		fchmod(int, mode_t);
int		lstat(const char *, struct stat *);
//#endif
__END_DECLS
#endif
#endif /* !_SYS_STAT_H_ */
