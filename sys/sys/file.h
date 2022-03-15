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

#ifdef _KERNEL
#include <sys/queue.h>
#include <sys/lock.h>

struct proc;
struct uio;
struct knote;

/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct file {
	//LIST_ENTRY(file) 	f_list;/* list of active files */
	struct file 		*f_filef;		/* list of active files */
	struct file 		**f_fileb;		/* list of active files */
	int					f_iflags;		/* internal flags */
	int					f_flag;			/* see below */
	short				f_type;			/* descriptor type */
	short				f_count;		/* reference count */
	short				f_msgcount;		/* references from message queue */
	int					f_usecount;		/* number active users */
	union {
		void			*f_Data;
		struct socket 	*f_Socket;
	} f_un;

	struct fileops		*f_ops;
	off_t				f_offset;
	struct ucred 		*f_cred;		/* credentials associated with descriptor */
	struct lock_object 	f_slock;
#define f_data			f_un.f_Data
#define f_socket		f_un.f_Socket
};

struct fileops {
	int	(*fo_rw)		(struct file *, struct uio *, struct ucred *);
	int (*fo_read)		(struct file *, struct uio *, struct ucred *);
	int (*fo_write)		(struct file *, struct uio *, struct ucred *);
	int	(*fo_ioctl)		(struct file *, int, caddr_t, struct proc *);
	int	(*fo_select) 	(struct file *, int, struct proc *);
	int	(*fo_poll)		(struct file *, int, struct proc *);
	int	(*fo_close)		(struct file *, struct proc *);
	int (*fo_kqfilter)	(struct file *, struct knote *);
};

extern struct file 	*filehead;	/* head of list of open files */
extern int 			maxfiles;	/* kernel limit on number of open files */
extern int 			nfiles;		/* actual number of open files */

/*
 * Access call.
 */
#define	F_OK		0	/* does file exist */
#define	X_OK		1	/* is it executable by caller */
#define	W_OK		2	/* writable by caller */
#define	R_OK		4	/* readable by caller */

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


#define	FILE_IS_USABLE(fp)	(((fp)->f_iflags &			\
		(FIF_WANTCLOSE|FIF_LARVAL)) == 0)

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

#define	FILE_UNUSE(fp, p) do {											\
	simple_lock(&(fp)->f_slock);										\
	if ((fp)->f_iflags & FIF_WANTCLOSE) {								\
		simple_unlock(&(fp)->f_slock);									\
		/* Will drop usecount */										\
		(void) closef((fp), (p));										\
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
#endif
#endif	/* _SYS_FILE_H_ */
