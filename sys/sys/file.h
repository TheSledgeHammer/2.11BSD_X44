/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)file.h	1.3 (2.11BSD GTE) 1/19/95
 */

#include <sys/fcntl.h>

#ifndef	_SYS_FILE_H_
#define	_SYS_FILE_H_

#ifdef KERNEL
struct proc;
struct uio;
/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct	file {
	struct	file *f_filef;	/* list of active files */
	struct	file **f_fileb;	/* list of active files */
	int		f_flag;			/* see below */
	short	f_type;			/* descriptor type */
	short	f_count;		/* reference count */
	short	f_msgcount;		/* references from message queue */
	union {
		caddr_t	*f_Data;
		struct socket *f_Socket;
	} f_un;

	off_t	f_offset;
	struct ucred *f_cred;	/* credentials associated with descriptor */
	struct fileops *f_ops;
	struct simplelock f_slock;
};

struct fileops {
	int	(*fo_rw)		(struct file *fp, struct uio *uio, struct ucred *cred);
	int	(*fo_ioctl)		(struct file *fp, int com, caddr_t data, struct proc *p);
	int	(*fo_select) 	(struct file *fp, int which, struct proc *p);
	int	(*fo_close)		(struct file *fp, struct proc *p);
} *f_ops;

#define f_data		f_un->f_Data
#define f_socket	f_un.f_Socket

extern struct file *filehead;	/* head of list of open files */
extern int maxfiles;			/* kernel limit on number of open files */
extern int nfiles;				/* actual number of open files */

struct	file *getf();
struct	file *falloc();

/*
 * Access call.
 */
#define	F_OK		0	/* does file exist */
#define	X_OK		1	/* is it executable by caller */
#define	W_OK		2	/* writable by caller */
#define	R_OK		4	/* readable by caller */

/*
 * Lseek call.
 */
#define	L_SET		0	/* absolute offset */
#define	L_INCR		1	/* relative to current offset */
#define	L_XTND		2	/* relative to end of file */

#define	GETF(fp, fd) { \
	if ((unsigned)(fd) >= NOFILE || ((fp) = u.u_ofile[fd]) == NULL) { \
		u.u_error = EBADF; \
		return; \
	} \
}

#define	DTYPE_VNODE		1	/* file */
#define	DTYPE_SOCKET	2	/* communications endpoint */
#define	DTYPE_PIPE		3	/* I don't want to hear it, okay? */
#endif
#endif	/* _SYS_FILE_H_ */
