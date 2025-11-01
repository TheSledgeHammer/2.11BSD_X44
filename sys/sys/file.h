/*	$NetBSD: file.h,v 1.48 2003/09/22 13:00:04 christos Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)file.h	8.3 (Berkeley) 1/9/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)file.h	1.3 (2.11BSD GTE) 1/19/95
 */

#ifndef	_SYS_FILE_H_
#define	_SYS_FILE_H_

#include <sys/fcntl.h>
#include <sys/unistd.h>

//#ifdef _KERNEL
#include <sys/queue.h>
#include <sys/lock.h>

struct proc;
struct uio;
struct stat;
struct knote;

/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct file {
	LIST_ENTRY(file) 	f_list;			/* list of active files */
	int					f_iflags;		/* internal flags */
	int					f_flag;			/* see below */
	short				f_type;			/* descriptor type */
	short				f_count;		/* reference count */
	short				f_msgcount;		/* references from message queue */
	int					f_usecount;		/* number active users */
	union {
		void			*f_Data;
		struct socket 	*f_Socket;
		struct mpx		*f_Mpx;
	} f_un;

	struct fileops		*f_ops;
	off_t				f_offset;
	struct ucred 		*f_cred;		/* credentials associated with descriptor */
	struct lock_object 	f_slock;
#define f_data			f_un.f_Data
#define f_socket		f_un.f_Socket
#define f_mpx			f_un.f_Mpx
};

struct fileops {
	int	(*fo_rw)		(struct file *, struct uio *, struct ucred *);
	int (*fo_read)		(struct file *, struct uio *, struct ucred *);
	int (*fo_write)		(struct file *, struct uio *, struct ucred *);
	int	(*fo_ioctl)		(struct file *, u_long, void *, struct proc *);
	int	(*fo_poll)		(struct file *, int, struct proc *);
	int	(*fo_stat)		(struct file *, struct stat *, struct proc *);
	int	(*fo_close)		(struct file *, struct proc *);
	int (*fo_kqfilter)	(struct file *, struct knote *);
};

struct filelist;
LIST_HEAD(filelist, file);
extern struct filelist  filehead;	/* head of list of open files */
extern int 				maxfiles;	/* kernel limit on number of open files */
extern int 				nfiles;		/* actual number of open files */

/*
 * Access call.
 */
//#define	F_OK		0	/* does file exist */
//#define	X_OK		1	/* is it executable by caller */
//#define	W_OK		2	/* writable by caller */
//#define	R_OK		4	/* readable by caller */

#ifndef _POSIX_SOURCE
/* whence values for lseek(2); renamed by POSIX 1003.1 */
/*
 * Lseek call.
 */
#define	L_SET		0	/* absolute offset */
#define	L_INCR		1	/* relative to current offset */
#define	L_XTND		2	/* relative to end of file */
#endif

#define	GETF(fp, fd) { 													\
	if ((unsigned)(fd) >= NOFILE || ((fp) = u.u_ofile[fd]) == NULL) { 	\
		u.u_error = EBADF; 												\
		return (u.u_error);												\
	} 																	\
}

#define	FIF_WANTCLOSE	0x01	/* a close is waiting for usecount */
#define	FIF_LARVAL		0x02	/* not fully constructed; don't use */

#define	FILE_IS_USABLE(fp)	(((fp)->f_iflags &							\
		(FIF_WANTCLOSE|FIF_LARVAL)) == 0)

#define	FILE_SET_MATURE(fp)	do {										\
	(fp)->f_iflags &= ~FIF_LARVAL;										\
} while (/*CONSTCOND*/0)

#ifdef DIAGNOSTIC
#define	FILE_USE_CHECK(fp, str) do {									\
	if ((fp)->f_usecount < 0)											\
		panic(str);														\
} while (/* CONSTCOND */ 0)
#else
#define	FILE_USE_CHECK(fp, str)		/* nothing */
#endif

/*
 * FILE_USE() must be called with the file lock held.
 * (Typical usage is: `fp = fd_getfile(..); FILE_USE(fp);'
 * and fd_getfile() returns the file locked)
 */
#define	FILE_USE(fp) do {												\
	(fp)->f_usecount++;													\
	FILE_USE_CHECK((fp), "f_usecount overflow");						\
	simple_unlock(&(fp)->f_slock);										\
} while (/* CONSTCOND */ 0)

#define	FILE_UNUSE(fp) do {												\
	simple_lock(&(fp)->f_slock);										\
	if ((fp)->f_iflags & FIF_WANTCLOSE) {								\
		simple_unlock(&(fp)->f_slock);									\
		/* Will drop usecount */										\
		(void) closef((fp));											\
		break;															\
	} else {															\
		(fp)->f_usecount--;												\
		FILE_USE_CHECK((fp), "f_usecount underflow");					\
	}																	\
	simple_unlock(&(fp)->f_slock);										\
} while (/* CONSTCOND */ 0)

#define	DTYPE_VNODE		1	/* file */
#define	DTYPE_SOCKET	2	/* communications endpoint */
#define	DTYPE_PIPE		3	/* I don't want to hear it, okay? */
#define	DTYPE_KQUEUE	4	/* event queue */
#define DTYPE_CRYPTO 	5	/* crypto */
#define	DTYPE_MISC		6	/* misc file descriptor type */

int                 fset(struct file *, int, int);
int                 fgetown(struct file *, int *);
int                 fsetown(struct file *, int);
int                 fgetlk(struct file *, int);
int                 fsetlk(struct file *, int, int);
#endif /* _KERNEL */
#endif	/* _SYS_FILE_H_ */
