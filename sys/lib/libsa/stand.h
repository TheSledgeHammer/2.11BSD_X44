/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)stand.h	8.1 (Berkeley) 6/11/93
 */

#ifndef	_LIBSA_STAND_H
#define	_LIBSA_STAND_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <sys/user.h>

#include <lib/libkern/libkern.h>

#include "saioctl.h"
#include "saerrno.h"

#define	UNIX	"/vmunix"

#ifndef NULL
#define	NULL	0
#endif

struct open_file;

/*
 * This structure is used to define file system operations in a file system
 * independent way.
 */
struct fs_ops {
	int					(*open)(char *, struct open_file *);
	int					(*close)(struct open_file *);
	int					(*read)(struct open_file *, char *, u_int, u_int *);
	int					(*write)(struct open_file *, char *, u_int, u_int *);
	off_t				(*seek)(struct open_file *, off_t, int);
	int					(*stat)(struct open_file *, struct stat *);
	int					(*readdir)(struct open_file *, struct dirent *);
};
extern struct fs_ops 	file_system[];
extern struct fs_ops	*exclusive_file_system;

/*
 * libstand-supplied filesystems
 */
extern struct fs_ops 	ufs_fsops;
extern struct fs_ops 	lfs_fsops;
extern struct fs_ops 	tftp_fsops;
extern struct fs_ops 	nfs_fsops;
extern struct fs_ops 	cd9660_fsops;
extern struct fs_ops 	dosfs_fsops;

/* where values for lseek(2) */
#define	SEEK_SET		0	/* set file offset to offset */
#define	SEEK_CUR		1	/* set file offset to current plus offset */
#define	SEEK_END		2	/* set file offset to EOF plus offset */

/* Device switch */
struct devsw {
	const char			dv_name[8];
	int					dv_type;							/* opaque type constant, arch-dependant */
    int					(*dv_init)(void);					/* early probe call */
	int					(*dv_strategy)(void *, int, daddr_t, u_int, char *, u_int *);
	int					(*dv_open)(struct open_file *, ...);
	int					(*dv_close)(struct open_file *);
	int					(*dv_ioctl)(struct open_file *, int, void *);
    int					(*dv_print)(int);			/* print device information */
	void				(*dv_cleanup)(void);
};
#define DEVT_NONE		0
#define DEVT_DISK		1
#define DEVT_NET		2
#define DEVT_CD			3
#define DEVT_ZFS		4
#define DEVT_FD			5

extern struct devsw 	devsw[];		/* device array */
extern int 				ndevs;			/* number of elements in devsw[] */

/*
 * Generic device specifier; architecture-dependent
 * versions may be larger, but should be allowed to
 * overlap.
 */
struct devdesc {
    struct devsw		*d_dev;
    int					d_unit;
    void				*d_opendata;
};

struct open_file {
	int					f_flags;	/* see F_* below */
	struct devsw		*f_dev;		/* pointer to device operations */
	void				*f_devdata;	/* device specific data */
	struct fs_ops		*f_ops;		/* pointer to file system operations */
	void				*f_fsdata;	/* file system specific data */
#define SOPEN_RASIZE	512
};

#define	SOPEN_MAX		64
extern struct open_file files[SOPEN_MAX];

/* f_flags values */
#define	F_READ			0x0001			/* file opened for reading */
#define	F_WRITE			0x0002			/* file opened for writing */
#define	F_RAW			0x0004			/* raw device open - no file system */
#define F_NODEV			0x0008			/* network open - no device */
#define	F_MASK			0xFFFF
/* Mode modifier for strategy() */
#define	F_NORA			(0x01 << 16)	/* Disable Read-Ahead */

/* sbrk emulation */
#define	O_RDONLY		0x0
#define O_WRONLY		0x1
#define O_RDWR			0x2
#define O_ACCMODE		0x3

/* alloc.c */
void    				*alloc(size_t);
void    				free(void *, size_t);
void    				*calloc(size_t, size_t);

/* bzero.c */
void    				bzero(void *, size_t);

/* close.c */
int     				close(int);

/* dev.c */
int     				nodev(struct iob *);
int    	 				noioctl(struct iob *, int, caddr_t);

/* disklabel.c */
struct disklabel;
char    				*getdisklabel(const char *, struct disklabel *);
u_short 				dkcksum(struct disklabel *);

/* environment.c */
#define EV_DYNAMIC		(1<<0)		/* value was dynamically allocated, free if changed/unset */
#define EV_VOLATILE		(1<<1)		/* value is volatile, make a copy of it */
#define EV_NOHOOK		(1<<2)		/* don't call hook when setting */

struct env_var;
typedef char *(ev_format_t)(struct env_var *);
typedef int (ev_sethook_t)(struct env_var *, int, const void *);
typedef int (ev_unsethook_t)(struct env_var *);

struct env_var {
    char				*ev_name;
    int					ev_flags;
    void				*ev_value;
    ev_sethook_t		*ev_sethook;
    ev_unsethook_t		*ev_unsethook;
    struct env_var		*ev_next, *ev_prev;
};

extern struct env_var 	*environ;
extern ev_sethook_t		env_noset;			/* refuse set operation */
extern ev_unsethook_t 	env_nounset;		/* refuse unset operation */

struct env_var 			*env_getenv(const char *);
int 					env_setenv(const char *, int, const void *, ev_sethook_t, ev_unsethook_t);
char 					*getenv(const char *);
int 					setenv(const char *, const char *, int);
int 					putenv(const char *);
int 					unsetenv(const char *);

/* getfile.c */
int     				getfile(char *, int);

/* gets.c */
void    				gets(char *);
void					ngets(char *, int);
int						fgetstr(char *, int, int );

/* ioctl.c */
int     				ioctl(int, int, char *);

/* ls.c */
void    				ls(int);

/* lseek.c */
off_t   				lseek(int, off_t, int);

/* open.c */
int     				open(char *, int);

/* printf.c */
void    				printf(const char *, ...);
void    				kprintn(u_long, int);

/* read.c */
int     				read(int, char *, u_int);
struct dirent 			*readdirfd(int);

/* sbrk.c */
void    				setheap(void *, void *);
char    				*getheap(size_t *);
char    				*sbrk(intptr_t);

/* snprintf.c */
int     				snprintf(char *, size_t, const char *, ...);

/* stat.c */
int     				fstat(int, struct stat *);
int     				stat(const char *, struct stat *);

/* twiddle.c */
void    				twiddle(void);
int						twiddle_set(struct env_var *, int, const void *);
void 					twiddle_divisor(u_int);

/* write.c */
int     				write(int, char *, u_int);

/* getopt.c */
extern char				*optarg;			/* getopt(3) external variables */
extern int				optind, opterr, optopt, optreset;
int						getopt(int, char * const *, const char *);

/* pager.c */
void					pager_open(void);
void					pager_close(void);
int						pager_output(const char *);
int						pager_file(const char *);

/* panic.c */
void                    panic(const char *,...);

/* strdup.c */
extern char 			*strdup(const char *);

/* strspn.c */
extern size_t 			strspn(const char *, const char *);

/* strtol.c */
extern long				strtol(const char *, char **, int);

#endif	/* _LIBSA_STAND_H */
