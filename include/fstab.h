/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)fstab.h	5.1 (Berkeley) 5/30/85
 */

#ifndef _FSTAB_H_
#define _FSTAB_H_

/*
 * File system table, see fstab (5)
 *
 * Used by dump, mount, umount, swapon, fsck, df, ...
 *
 * The fs_spec field is the block special name.  Programs
 * that want to use the character special name must create
 * that name by prepending a 'r' after the right most slash.
 * Quota files are always named "quotas", so if type is "rq",
 * then use concatenation of fs_file and "quotas" to locate
 * quota file.
 */
#define	FSTAB			"/etc/fstab"
#define _PATH_FSTAB 	FSTAB

#define	FSTAB_RW	"rw"	/* read/write device */
#define	FSTAB_RQ	"rq"	/* read/write with quotas */
#define	FSTAB_RO	"ro"	/* read-only device */
#define	FSTAB_SW	"sw"	/* swap device */
#define	FSTAB_XX	"xx"	/* ignore totally */

struct fstab {
	char	*fs_spec;		/* block special device name */
	char	*fs_file;		/* file system path prefix */
	char	*fs_vfstype;	/* File system type, ufs, nfs */
	char	*fs_mntops;		/* Mount options ala -o */
	char	*fs_type;		/* FSTAB_* */
	int		fs_freq;		/* dump frequency, in days */
	int		fs_passno;		/* pass number on parallel dump */
};

/* getdevpath(3) */
#define _HAVE_GETDEVPATH	1	/* allow code conditionalization */
#define GETDEVPATH_RAWDEV	0x0001

#include <sys/cdefs.h>

__BEGIN_DECLS
struct	fstab 	*getfsent(void);
struct	fstab 	*getfsspec(const char *);
struct	fstab 	*getfsfile(const char *);
struct	fstab 	*getfstype(void);
char			*getdevpath(const char *, int);
const char      *getfstab(void);
void            setfstab(const char *);
int				setfsent(void);
void			endfsent(void);
__END_DECLS

#endif /* !_FSTAB_H_ */
