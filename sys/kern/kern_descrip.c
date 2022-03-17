/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_descrip.c	1.6 (2.11BSD) 1999/9/13
 */
/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
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
 * $NetBSD: kern_descrip.c,v 1.147 2006/11/01 10:17:58 yamt Exp $
 *	@(#)kern_descrip.c	8.8 (Berkeley) 2/14/95
 */

/*
 * TODO:
 * - fd_unused: not-implemented
 * 	  - required for fdrelease
 * 	  	- required for fdcloseexec
 * 	  		- required for execve
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <sys/file.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/filedesc.h>
#include <sys/unistd.h>
#include <sys/resourcevar.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/syslog.h>

/*
 * Descriptor management.
 */
int dup2(); /* syscall */
int dupit(int, struct file *, int);
int fset(struct file *, int, int);
int fgetown(struct file *, int *);
int fsetown(struct file *, int);
int fgetlk(struct file *, int);
int fsetlk(struct file *, int, int);
int fioctl(struct file *, int, caddr_t, struct proc *);

/*
 * System calls on descriptors.
 */
void
getdtablesize()
{
	u.u_r.r_val1 = NOFILE;
	//u.u_r.r_val1 = min((int)u.u_rlimit[RLIMIT_NOFILE].rlim_cur, maxfiles);
}

/*
 * Duplicate a file descriptor.
 */
int
dup()
{
	register struct dup_args {
		syscallarg(int)	fd;
	} *uap = (struct dup_args *) u.u_ap;
	register struct file *fp;
	int j, fd, error;

	if (SCARG(uap, fd) & ~077) {
		SCARG(uap, fd) &= 077;
		return (dup2());
	}

	GETF(fp, SCARG(uap, fd));
	j = ufalloc(0);
	if (j < 0) {
		return (j);
	}
	return (dupit(j, fp, u.u_pofile[SCARG(uap, fd)] &~ UF_EXCLOSE));
}

int
dup2()
{
	register struct dup2_args {
		syscallarg(int)	from;
		syscallarg(int) to;
	} *uap = (struct dup2_args *) u.u_ap;
	register struct file *fp;

	GETF(fp, SCARG(uap, from));
	if (SCARG(uap, to) < 0 || SCARG(uap, to) >= NOFILE) {
		u.u_error = EBADF;
		return (EBADF);
	}
	u.u_r.r_val1 = SCARG(uap, to);
	if (SCARG(uap, from) == SCARG(uap, to))
		return (0);
	if (u.u_ofile[SCARG(uap, to)])
		/*
		 * dup2 must succeed even if the close has an error.
		 */
		(void) closef(u.u_ofile[SCARG(uap, to)]);

	return (dupit(SCARG(uap, to), fp, u.u_pofile[SCARG(uap, from)] & ~ UF_EXCLOSE));
}

int
dupit(fd, fp, flags)
	register int fd;
	register struct file *fp;
	int flags;
{
	register struct filedesc *fdp;

	u.u_ofile[fd] = fp;
	u.u_pofile[fd] = flags;
	fp->f_count++;
	if (fd > u.u_lastfile) {
		u.u_lastfile = fd;
	}
	u.u_r.r_val1 = fd;
	return (0);
}

/*
 * The file control system call.
 */
int
fcntl()
{
	register struct fcntl_args {
		syscallarg(int)	fdes;
		syscallarg(int)	cmd;
		syscallarg(int)	arg;
	} *uap = (struct fcntl_args *)u.u_ap;
	register struct file *fp;
	register int i, error, flg;// = F_POSIX;
	register char *pop;
	u_int newmin;

	if ((fp = getf(SCARG(uap, fdes))) == NULL)
		return (EBADF);
	pop = &u.u_pofile[SCARG(uap, fdes)];
	switch(SCARG(uap, cmd)) {
	case F_DUPFD:
		newmin = (long)SCARG(uap, arg);
		if (newmin >= u.u_rlimit[RLIMIT_NOFILE].rlim_cur || newmin >= maxfiles) {
			return (EINVAL);
		}
		i = SCARG(uap, arg);
		if (i < 0 || i >= NOFILE) {
			u.u_error = EINVAL;
			return (EINVAL);
		}
		if ((error = ufalloc(i)) < 0)
			return (error);
		return (dupit(i, fp, *pop &~ UF_EXCLOSE));

	case F_GETFD:
		u.u_r.r_val1 = *pop & 1;
		return (0);

	case F_SETFD:
		*pop = (*pop &~ 1) | (SCARG(uap, arg) & 1);
		return (0);

	case F_GETFL:
		u.u_r.r_val1 = OFLAGS(fp->f_flag);
		return (0);

	case F_SETFL:
		fp->f_flag &= ~FCNTLFLAGS;
		fp->f_flag |= (FFLAGS(SCARG(uap, arg))) & FCNTLFLAGS;
		u.u_error = fset(fp, FNONBLOCK, fp->f_flag & FNONBLOCK);
		if (u.u_error)
			return (u.u_error);
		u.u_error = fset(fp, FASYNC, fp->f_flag & FASYNC);
		if (u.u_error)
			(void) fset(fp, FNONBLOCK, 0);
		return (u.u_error);

	case F_GETOWN:
		u.u_error = fgetown(fp, &u.u_r.r_val1);
		return (u.u_error);

	case F_SETOWN:
		u.u_error = fsetown(fp, SCARG(uap, arg));
		return (u.u_error);

	case F_SETLKW:
		flg |= F_WAIT;
		/* Fall into F_SETLK */

	case F_SETLK:
		u.u_error = fsetlk(fp, SCARG(uap, arg), flg);
		return (u.u_error);

	case F_GETLK:
		u.u_error = fgetlk(fp, SCARG(uap, arg));
		return (u.u_error);

	default:
		u.u_error = EINVAL;
		return (u.u_error);
	}
	/* NOTREACHED */
}

int
fset(fp, bit, value)
	register struct file *fp;
	int bit, value;
{
	if (value)
		fp->f_flag |= bit;
	else
		fp->f_flag &= ~bit;
	return (fioctl(fp, (u_int)(bit == FNONBLOCK ? FIONBIO : FIOASYNC), (caddr_t)&value, u.u_procp));
}

int
fgetown(fp, valuep)
	register struct file *fp;
	register int *valuep;
{
	register int error;

	if (fp->f_type == DTYPE_SOCKET) {
//		*valuep = mfsd(&fp->f_socket->so_pgrp);
		*valuep = ((struct socket *)fp->f_data)->so_pgid;
		return (0);
	}
	error = fioctl(fp, (u_int)TIOCGPGRP, (caddr_t)valuep, u.u_procp);
	*valuep = -*valuep;
	return (error);
}

int
fsetown(fp, value)
	register struct file *fp;
	int value;
{
	if (fp->f_type == DTYPE_SOCKET) {
//		mtsd(&fp->f_socket->so_pgrp, value);
		return (0);
	}
	if (value > 0) {
		register struct proc *p = pfind(value);
		if (p == 0)
			return (ESRCH);
		value = p->p_pgrp->pg_id;
	} else {
		value = -value;
	}
	return (fioctl(fp, (u_int)TIOCSPGRP, (caddr_t)&value, u.u_procp));
}

int
fgetlk(fp, value)
	register struct file *fp;
	int value;
{
	struct proc *p;
	struct vnode *vp;
	struct flock fl;
	int error;

	if (fp->f_type != DTYPE_VNODE) {
		return (EBADF);
	}
	vp = (struct vnode*) fp->f_data;
	/* Copy in the lock structure */
	error = copyin((caddr_t) value, (caddr_t) & fl, sizeof(fl));
	if (error) {
		return (error);
	}
	p = pfind(value);
	if (fl.l_whence == SEEK_CUR)
		fl.l_start += fp->f_offset;
	if (error == VOP_ADVLOCK(vp, (caddr_t) p, F_GETLK, &fl, F_POSIX))
		return (error);
	return (copyout((caddr_t) &fl, (caddr_t) value, sizeof(fl)));
}

int
fsetlk(fp, value, flags)
	register struct file *fp;
	int value, flags;
{
	struct proc *p;
	struct vnode *vp;
	struct flock fl;
	int error;

	if (fp->f_type != DTYPE_VNODE) {
		return (EBADF);
	}
	vp = (struct vnode *)fp->f_data;
	/* Copy in the lock structure */
	error = copyin((caddr_t)value, (caddr_t) &fl, sizeof(fl));
	if (error) {
		return (error);
	}
	p = pfind(value);
	if (fl.l_whence == SEEK_CUR)
		fl.l_start += fp->f_offset;
	switch (fl.l_type) {
	case F_RDLCK:
		if ((fp->f_flag & FREAD) == 0)
			return (EBADF);
		p->p_flag |= P_ADVLOCK;
		return (VOP_ADVLOCK(vp, (caddr_t) p, F_SETLK, &fl, flags));

	case F_WRLCK:
		if ((fp->f_flag & FWRITE) == 0)
			return (EBADF);
		p->p_flag |= P_ADVLOCK;
		return (VOP_ADVLOCK(vp, (caddr_t) p, F_SETLK, &fl, flags));

	case F_UNLCK:
		return (VOP_ADVLOCK(vp, (caddr_t) p, F_UNLCK, &fl, F_POSIX));

	default:
		return (EINVAL);
	}
}

int
fioctl(fp, cmd, value, p)
	register struct file *fp;
	int cmd;
	caddr_t value;
	struct proc *p;
{
	return ((*fp->f_ops->fo_ioctl)(fp, cmd, value, p));
}

int
close()
{
	register struct close_args {
		syscallarg(int)	fd;
	} *uap = (struct close_args *)u.u_ap;
	register struct file *fp;

	GETF(fp, SCARG(uap, fd));
	u.u_ofile[SCARG(uap, fd)] = NULL;
	while (u.u_lastfile >= 0 && u.u_ofile[u.u_lastfile] == NULL)
		u.u_lastfile--;
	/* WHAT IF u.u_error ? */
	return (closef(fp));
}

int
fstat()
{
	register struct fstat_args {
		syscallarg(int)	fdes;
		syscallarg(struct stat *) sb;
	} *uap = (struct fstat_args *)u.u_ap;
	register struct file *fp;
	struct stat ub;

	if ((fp = getf(SCARG(uap, fdes))) == NULL)
		return (EBADF);
	switch (fp->f_type) {

	case DTYPE_VNODE:
		u.u_error = vn_stat((struct vnode *)fp->f_data, &ub, u.u_procp);
		break;

	case DTYPE_SOCKET:
		u.u_error = soo_stat((struct socket *)fp->f_socket, &ub);
		break;
	default:
		panic("fstat");
		/*NOTREACHED*/
		break;
	}
	if (u.u_error == 0)
		u.u_error = copyout((caddr_t)&ub, (caddr_t)SCARG(uap, sb), sizeof (ub));
	return (u.u_error);
}

/* copied, for supervisory networking, to sys_net.c */
/*
 * Allocate a user file descriptor.
 */
static int
ufalloc(i)
	register int i;
{
	for (; i < NOFILE; i++) {
		if (u.u_ofile[i] == NULL) {
			u.u_r.r_val1 = i;
			u.u_pofile[i] = 0;
			if (i > u.u_lastfile)
				u.u_lastfile = i;
			return (i);
		}
	}
	u.u_error = EMFILE;
	return (-1);
}

struct	file *lastf;
/*
 * Allocate a user file descriptor
 * and a file structure.
 * Initialize the descriptor
 * to point at the file structure.
 */
struct file *
falloc()
{
	register struct file *fp;
	register int i;

	i = ufalloc(0);
	if (i < 0)
		return (NULL);
	if (lastf == 0)
		lastf = fp;
	for (fp = lastf; fp < fileNFILE; fp++)
		if (fp->f_count == 0)
			goto slot;
	for (fp = file; fp < lastf; fp++)
		if (fp->f_count == 0)
			goto slot;
	tablefull("file");
	u.u_error = ENFILE;
	return (NULL);
slot:
	u.u_ofile[i] = fp;
	fp->f_count = 1;
	fp->f_data = 0;
	fp->f_offset = 0;
	lastf = fp + 1;
	return (fp);
}

/*
 * Convert a user supplied file descriptor into a pointer
 * to a file structure.  Only task is to check range of the descriptor.
 * Critical paths should use the GETF macro unless code size is a 
 * consideration.
 */

struct file *
getf(f)
	register int f;
{
	register struct file *fp;

	if ((unsigned)f >= NOFILE || (fp = u.u_ofile[f]) == NULL) {
		u.u_error = EBADF;
		return (NULL);
	}
	return (fp);
}

/*
 * Internal form of close.
 * Decrement reference count on file structure.
 */
int
closef(fp)
	register struct file *fp;
{
	int	error;

	if (fp == NULL)
		return(0);
	if (fp->f_count > 1) {
		fp->f_count--;
		return (0);
	}

	if (fp->f_count < 1) {
		panic("closef: count < 1");
	}
	error = (*fp->f_ops->fo_close)(fp, u.u_procp);
	fp->f_count = 0;
	return (error);
}

/*
 * Apply an advisory lock on a file descriptor.
 */
int
flock()
{
	register struct flock_args {
		syscallarg(int)	fd;
		syscallarg(int)	how;
	} *uap = (struct flock_args *)u.u_ap;
	register struct filedesc *fdp = u.u_fd;
	register struct file *fp;
	struct vnode *vp;
	struct flock lf;
	int error;

	if ((fp = getf(SCARG(uap, fd))) == NULL)
		return (EBADF);
	if (fp->f_type != DTYPE_VNODE) {
		return (EOPNOTSUPP);
	}
	vp = (struct vnode *)fp->f_data;
	lf.l_whence = SEEK_SET;
	lf.l_start = 0;
	lf.l_len = 0;
	if (SCARG(uap, how) & LOCK_UN) {
		lf.l_type = F_UNLCK;
		fp->f_flag &= ~FHASLOCK;
		return (VOP_ADVLOCK(vp, (caddr_t)fp, F_UNLCK, &lf, F_FLOCK));
	}
	if (SCARG(uap, how) & LOCK_EX) {
		lf.l_type = F_WRLCK;
	} else if (SCARG(uap, how) & LOCK_SH) {
		lf.l_type = F_RDLCK;
	} else {
		return (EBADF);
	}
	fp->f_flag |= FHASLOCK;
	if(SCARG(uap, how) & LOCK_NB) {
		return (VOP_ADVLOCK(vp, (caddr_t)fp, F_SETLK, &lf, F_FLOCK));
	}
	return (VOP_ADVLOCK(vp, (caddr_t)fp, F_SETLK, &lf, F_FLOCK|F_WAIT));
}

/*
 * File Descriptor pseudo-device driver (/dev/fd/).
 *
 * Opening minor device N dup()s the file (if any) connected to file
 * descriptor N belonging to the calling process.  Note that this driver
 * consists of only the ``open()'' routine, because all subsequent
 * references to this file will be direct to the other driver.
 */
/* ARGSUSED */
int
fdopen(dev, mode, type)
	dev_t dev;
	int mode, type;
{

	/*
	 * XXX Kludge: set u.u_dupfd to contain the value of the
	 * the file descriptor being sought for duplication. The error 
	 * return ensures that the vnode for this device will be released
	 * by vn_open. Open will detect this special error and take the
	 * actions in dupfdopen below. Other callers of vn_open will
	 * simply report the error.
	 */
	u.u_dupfd = minor(dev);
	return (ENODEV);
}

/*
 * Duplicate the specified descriptor to a free descriptor.
 */
int
dupfdopen(fdp, indx, dfd, mode, error)
	register struct filedesc *fdp;
	register int indx, dfd;
	int mode;
	int error;
{
	register struct file *wfp;
	struct file *fp;
	
	/*
	 * If the to-be-dup'd fd number is greater than the allowed number
	 * of file descriptors, or the fd to be dup'd has already been
	 * closed, reject.  Note, check for new == old is necessary as
	 * falloc could allocate an already closed to-be-dup'd descriptor
	 * as the new descriptor.
	 */
	fp = u.u_ofile[indx];
	if	(dfd >= NOFILE || (wfp = u.u_ofile[dfd]) == NULL || fp == wfp)
		return(EBADF);

	/*
	 * There are two cases of interest here.
	 *
	 * For ENODEV simply dup (dfd) to file descriptor
	 * (indx) and return.
	 *
	 * For ENXIO steal away the file structure from (dfd) and
	 * store it in (indx).  (dfd) is effectively closed by
	 * this operation.
	 *
	 * NOTE: ENXIO only comes out of the 'portal fs' code of 4.4 - since
	 * 2.11BSD does not implement the portal fs the code is ifdef'd out
	 * and a short message output.
	 *
	 * Any other error code is just returned.
	 */
	switch (error) {
	case ENODEV:
		/*
		 * Check that the mode the file is being opened for is a
		 * subset of the mode of the existing descriptor.
		 */
		if (((mode & (FREAD | FWRITE)) | wfp->f_flag) != wfp->f_flag)
			return (EACCES);
		u.u_ofile[indx] = wfp;
		u.u_pofile[indx] = u.u_pofile[dfd];
		wfp->f_count++;
		if (indx > u.u_lastfile)
			u.u_lastfile = indx;
		return (0);
	case ENXIO:
		/*
		 * Steal away the file pointer from dfd, and stuff it into indx.
		 */
		fdp->fd_ofiles[indx] = fdp->fd_ofiles[dfd];
		fdp->fd_ofiles[dfd] = NULL;
		fdp->fd_ofileflags[indx] = fdp->fd_ofileflags[dfd];
		fdp->fd_ofileflags[dfd] = 0;
		/*
		 * Complete the clean up of the filedesc structure by
		 * recomputing the various hints.
		 */
		if (indx > fdp->fd_lastfile)
			fdp->fd_lastfile = indx;
		else
			while (fdp->fd_lastfile > 0
					&& fdp->fd_ofiles[fdp->fd_lastfile] == NULL)
				fdp->fd_lastfile--;
		if (dfd < fdp->fd_freefile)
			fdp->fd_freefile = dfd;
		return (0);
	default:
		return (error);
	}
	/* NOTREACHED */
}

/*
 * Copy a filedesc structure.
 */
struct filedesc *
fdcopy(p)
	struct proc *p;
{
	register struct filedesc *newfdp, *fdp;
	register struct file **fpp;
	register int i;

	fdp = p->p_fd;
	MALLOC(newfdp, struct filedesc *, sizeof(struct filedesc0), M_FILEDESC, M_WAITOK);
	bcopy(fdp, newfdp, sizeof(struct filedesc));
	VREF(newfdp->fd_cdir);
	if (newfdp->fd_rdir)
		VREF(newfdp->fd_rdir);
	newfdp->fd_refcnt = 1;

	/*
	 * If the number of open files fits in the internal arrays
	 * of the open file structure, use them, otherwise allocate
	 * additional memory for the number of descriptors currently
	 * in use.
	 */
	if (newfdp->fd_lastfile < NDFILE) {
		newfdp->fd_ofiles = ((struct filedesc0*) newfdp)->fd_dfiles;
		newfdp->fd_ofileflags = ((struct filedesc0*) newfdp)->fd_dfileflags;
		i = NDFILE;
	} else {
		/*
		 * Compute the smallest multiple of NDEXTENT needed
		 * for the file descriptors currently in use,
		 * allowing the table to shrink.
		 */
		i = newfdp->fd_nfiles;
		while (i > 2 * NDEXTENT && i > newfdp->fd_lastfile * 2)
			i /= 2;
		MALLOC(newfdp->fd_ofiles, struct file **, i * OFILESIZE, M_FILEDESC, M_WAITOK);
		newfdp->fd_ofileflags = (char*) &newfdp->fd_ofiles[i];
	}
	newfdp->fd_nfiles = i;
	bcopy(fdp->fd_ofiles, newfdp->fd_ofiles, i * sizeof(struct file**));
	bcopy(fdp->fd_ofileflags, newfdp->fd_ofileflags, i * sizeof(char));
	fpp = newfdp->fd_ofiles;
	for (i = newfdp->fd_lastfile; i-- >= 0; fpp++)
		if (*fpp != NULL)
			(*fpp)->f_count++;
	return (newfdp);
}

/* Release a filedesc structure. */
void
fdfree(p)
	struct proc *p;
{
	register struct filedesc *fdp;
	struct file **fpp;
	register int i;

	fdp = p->p_fd;
	if (--fdp->fd_refcnt > 0)
		return;
	fpp = fdp->fd_ofiles;
	for (i = fdp->fd_lastfile; i-- >= 0; fpp++)
		if (*fpp)
			(void) closef(*fpp);
	if (fdp->fd_nfiles > NDFILE)
		FREE(fdp->fd_ofiles, M_FILEDESC);
	vrele(fdp->fd_cdir);
	if (fdp->fd_rdir)
		vrele(fdp->fd_rdir);
	FREE(fdp, M_FILEDESC);
}

void
fdremove(fdp, fd)
	register struct filedesc *fdp;
	int fd;
{
	simple_lock(&fdp->fd_slock);
	fdp->fd_ofiles[fd] = NULL;
	fd_unused(fdp, fd);
	simple_unlock(&fdp->fd_slock);
}

int
fdrelease(p, fd)
	struct proc *p;
	int fd;
{
	struct filedesc	*fdp;
	struct file	**fpp, *fp;

	fdp = p->p_fd;
	simple_lock(&fdp->fd_slock);
	if (fd < 0 || fd > fdp->fd_lastfile)
		goto badf;
	fpp = &fdp->fd_ofiles[fd];
	fp = *fpp;
	if (fp == NULL)
		goto badf;

	simple_lock(&fp->f_slock);
	if (!FILE_IS_USABLE(fp)) {
		simple_unlock(&fp->f_slock);
		goto badf;
	}

	FILE_USE(fp);

	*fpp = NULL;
	fdp->fd_ofileflags[fd] = 0;
	fd_unused(fdp, fd);
	simple_unlock(&fdp->fd_slock);
	return (closef(fp));

badf:
	simple_unlock(&fdp->fd_slock);
	return (EBADF);
}

/*
 * Make this process not share its filedesc structure, maintaining
 * all file descriptor state.
 */
void
fdunshare(p)
	struct proc *p;
{
	struct filedesc *newfd;

	if(u.u_fd->fd_refcnt == 1)
		return;

	newfd = fdcopy(p);
	fdfree(p);
	p->p_fd = newfd;
}

/*
 * Close any files on exec?
 */
void
fdcloseexec(p)
	struct proc *p;
{
	struct filedesc *fdp;
	int	fd;

	fdunshare(p);

	fdp = p->p_fd;
	for (fd = 0; fd <= fdp->fd_lastfile; fd++)
		if (fdp->fd_ofileflags[fd] & UF_EXCLOSE)
			(void) fdrelease(p, fd);
}
