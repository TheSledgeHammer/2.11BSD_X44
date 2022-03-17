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
#include <sys/null.h>
//#include <sys/queue.h>

struct filelists filehead;	/* head of list of open files */
int 			 nfiles;		/* actual number of open files */

void
finit()
{
	LIST_INIT(&filehead);
}


/*
 * Return pathconf information about a file descriptor.
 */
/* ARGSUSED */
int
fpathconf()
{
	register struct fpathconf_args {
		syscallarg(int)	fd;
		syscallarg(int)	name;
	} *uap = (struct fpathconf_args *)u.u_ap;
	register_t *retval;

	int fd = SCARG(uap, fd);
	struct filedesc *fdp;
	struct file *fp;
	struct vnode *vp;

	fdp = u.u_fd;
	if ((u_int)fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL)
		return (EBADF);
	switch (fp->f_type) {

	case DTYPE_SOCKET:
		if (SCARG(uap, name) != _PC_PIPE_BUF)
			return (EINVAL);
		*retval = PIPE_BUF;
		return (0);

	case DTYPE_VNODE:
		vp = (struct vnode *)fp->f_data;
		return (VOP_PATHCONF(vp, SCARG(uap, name), retval));

	default:
		panic("fpathconf");
	}
	/*NOTREACHED*/
	return (0);
}

static int
ufalloc(i, want)
	register int i, want;
{
	if ((i = want) < u.u_freefile) {
		i = u.u_freefile;
	}
	for (; i < NOFILE; i++) {
		if (u.u_ofile[i] == NULL) {
			u.u_r.r_val1 = i;
			u.u_pofile[i] = 0;
			if (i > u.u_lastfile) {
				u.u_lastfile = i;
			}
			if (want <= u.u_freefile) {
				u.u_freefile = i;
			}
			return (i);
		}
	}

	u.u_error = EMFILE;
	return (-1);
}

int fdexpand;

static int
ufdalloc(want, result)
	int want;
	int *result;
{
	register int i;
	int lim, last, nfiles;
	struct file **newofile;
	char *newofileflags;

	lim = min((int) u.u_rlimit[RLIMIT_NOFILE].rlim_cur, maxfiles);
	for (;;) {
		last = min(u.u_nfiles, lim);
		if (ufalloc(i, want)) {
			*result = i;
			return (0);
		}
		/*
		 * No space in current array.  Expand?
		 */
	    if (u.u_nfiles >= lim) {
	        return (EMFILE);
	    }
	    if (u.u_nfiles >= NDEXTENT) {
	        nfiles = NDEXTENT;
	    } else {
	        nfiles = 2 * u.u_nfiles;
	    }
		MALLOC(newofile, struct file **, nfiles * OFILESIZE, M_FILEDESC, M_WAITOK);
		newofileflags = (char *) &newofile[nfiles];
		/*
		 * Copy the existing ofile and ofileflags arrays
		 * and zero the new portion of each array.
		 */
	    bcopy(u.u_ofile, newofile, (i = sizeof(struct file *) * u.u_nfiles));
	    bzero((char *)newofile + i, nfiles * sizeof(struct file *) - i);
	    bcopy(u.u_pofile, newofileflags,(i = sizeof(char) * u.u_nfiles));
	    bzero(newofileflags + i, nfiles * sizeof(char) - i);
	    if (u.u_nfiles > NDFILE) {
	        FREE(u.u_ofile, M_FILEDESC);
	    }
	    u.u_ofile = newofile;
	    u.u_pofile = newofileflags;
	    u.u_nfiles = nfiles;
	    fdexpand++;
	}
	return (0);
}

/*
 * Allocate a file descriptor for the process.
 */
struct filedesc *
fdalloc(result)
	int *result;
{
	register struct filedesc *fdp;
	int error, i;

	error = ufdalloc(0, result);
	if (error) {
		fdp = u.u_fd;
		return (fdp);
	}
	return (NULL);
}

/*
 * Allocate a user file descriptor
 * and a file structure.
 * Initialize the descriptor
 * to point at the file structure.
 */
struct file *
falloc()
{
	register struct file *fp, *fq;
	register int i;

	i = ufalloc(0);

	nfiles++;
	MALLOC(fp, struct file *, sizeof(struct file), M_FILE, M_WAITOK);
	bzero(fp, sizeof(struct file));
	if (fq == u.u_ofile[0]) {
		LIST_INSERT_AFTER(fq, fp, f_list);
	} else {
		LIST_INSERT_HEAD(&filehead, fp, f_list);
	}

	u.u_ofile[i] = fp;
	fp->f_count = 1;
	fp->f_data = 0;
	fp->f_offset = 0;
	fp->f_cred = u->u_ucred;
	crhold(fp->f_cred);
}

/*
 * Check to see whether n user file descriptors
 * are available to the process p.
 */
int
fdavail(n)
	register int n;
{
	register struct filedesc *fdp;
	register struct file **fpp;
	register int i, lim;

	fdp = u.u_fd;
	lim = min((int)u.u_rlimit[RLIMIT_NOFILE].rlim_cur, maxfiles);
	if ((i = lim - fdp->fd_nfiles) > 0 && (n -= i) <= 0)
		return (1);
	fpp = &fdp->fd_ofiles[fdp->fd_freefile];
	for (i = fdp->fd_nfiles - fdp->fd_freefile; --i >= 0; fpp++)
		if (*fpp == NULL && --n <= 0)
			return (1);
	return (0);
}

/*
 * Free a file descriptor.
 */
void
ffree(fp)
	register struct file *fp;
{
	register struct file *fq;

	LIST_REMOVE(fp, f_list);
	crfree(fp->f_cred);
#ifdef DIAGNOSTIC
	fp->f_count = 0;
#endif
	nfiles--;
	FREE(fp, M_FILE);
}
