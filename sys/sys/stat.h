/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)stat.h	7.1.5 (2.11BSD) 1996/09/20
 */

#ifndef	_STAT_H_
#define	_STAT_H_

#include <sys/time.h>

#ifndef _POSIX_SOURCE
struct ostat {
	u_int16_t st_dev;				/* inode's device */
	ino_t	  st_ino;				/* inode's number */
	mode_t	  st_mode;				/* inode protection mode */
	nlink_t	  st_nlink;				/* number of hard links */
	u_int16_t st_uid;				/* user ID of the file's owner */
	u_int16_t st_gid;				/* group ID of the file's group */
	u_int16_t st_rdev;				/* device type */
	int32_t	  st_size;				/* file size, in bytes */
	struct	timespec st_atimespec;	/* time of last access */
	struct	timespec st_mtimespec;	/* time of last data modification */
	struct	timespec st_ctimespec;	/* time of last file status change */
	int32_t	  st_blksize;			/* optimal blocksize for I/O */
	int32_t	  st_blocks;			/* blocks allocated for file */
	u_int32_t st_flags;				/* user defined flags for file */
	u_int32_t st_gen;				/* file generation number */
};
#endif /* !_POSIX_SOURCE */

struct	stat {
	dev_t				st_dev;		/* inode's device */
	ino_t				st_ino;		/* inode's number */
	mode_t	 			st_mode;	/* inode protection mode */
	nlink_t				st_nlink;	/* number of hard links */
	uid_t				st_uid;		/* user ID of the file's owner */
	gid_t				st_gid;		/* group ID of the file's group */
	dev_t				st_rdev;	/* device type */
	off_t				st_size;	/* file size, in bytes */
	struct	timespec	st_atime;	/* time of last access */
	struct	timespec	st_mtime;	/* time of last data modification */
	struct	timespec 	st_ctime;	/* time of last file status change */
	unsigned long		st_blksize;	/* optimal blocksize for I/O */
	quad_t				st_blocks;	/* blocks allocated for file */
	unsigned long		st_flags;	/* user defined flags for file */
	unsigned long		st_gen;		/* file generation number */
	int					st_spare1;
	int					st_spare2;
	long				st_spare3;
	quad_t				st_spare4[3];
};

#define st_atime st_atime.ts_sec
#define st_mtime st_mtime.ts_sec
#define st_ctime st_ctime.ts_sec

#define	S_IFMT	 0170000		/* type of file */
#define	S_IFDIR	 0040000		/* directory */
#define	S_IFCHR	 0020000		/* character special */
#define	S_IFBLK	 0060000		/* block special */
#define	S_IFREG	 0100000		/* regular */
#define	S_IFLNK	 0120000		/* symbolic link */
#define	S_IFSOCK 0140000		/* socket */
#define	S_ISUID	 0004000		/* set user id on execution */
#define	S_ISGID	 0002000		/* set group id on execution */
#define	S_ISVTX	 0001000		/* save swapped text even after use */
#define	S_IREAD	 0000400		/* read permission, owner */
#define	S_IWRITE 0000200		/* write permission, owner */
#define	S_IEXEC	 0000100		/* execute/search permission, owner */

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

#define	S_ISDIR(m)	((m & 0170000) == 0040000)	/* directory */
#define	S_ISCHR(m)	((m & 0170000) == 0020000)	/* char special */
#define	S_ISBLK(m)	((m & 0170000) == 0060000)	/* block special */
#define	S_ISREG(m)	((m & 0170000) == 0100000)	/* regular file */


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
#define	UF_IMMUTABLE	0x0002		/* file may not be changed */
#define	UF_APPEND		0x0004		/* writes to file may only append */
/*
 * Super-user changeable flags.
 */
#define	SF_SETTABLE		0xff00		/* mask of superuser changeable flags */
#define	SF_ARCHIVED		0x0100		/* file is archived */
#define	SF_IMMUTABLE	0x0200		/* file may not be changed */
#define	SF_APPEND		0x0400		/* writes to file may only append */

#ifdef KERNEL
/*
 * Shorthand abbreviations of above.
 */
#define	APPEND		(UF_APPEND | SF_APPEND)
#define	IMMUTABLE	(UF_IMMUTABLE | SF_IMMUTABLE)
#endif


#ifndef KERNEL
#include <sys/cdefs.h>

__BEGIN_DECLS
int		chmod (const char *, mode_t);
int		fstat (int, struct stat *);
int		mkdir (const char *, mode_t);
int		mkfifo (const char *, mode_t);
int		stat (const char *, struct stat *);
mode_t	umask (mode_t);
#ifndef _POSIX_SOURCE
int		chflags (const char *, u_long);
int		fchflags (int, u_long);
int		fchmod (int, mode_t);
int		lstat (const char *, struct stat *);
#endif
__END_DECLS
#endif
#endif /* !_STAT_H_ */
