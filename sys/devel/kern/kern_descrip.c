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
 * - management of filedesc cdir, rdir & cmask
 * - fdcopy
 * - fdfree
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

/*
void			fdesc_init(struct proc *, struct filedesc0 *);
int				ufalloc(int, int *);
struct file		*falloc();
void			fdexpand(int);
int  			ufdalloc(int *);
struct filedesc *fdalloc(int *);
int 			fdavail(int);
void			ffree(struct file *);
void 			ufdsync(struct filedesc *);
void 			fdsync(struct filedesc *);
*/
struct filelists filehead;	/* head of list of open files */
int 			 nfiles;		/* actual number of open files */

#define file_lock(fp) 		(simple_lock(&fp->f_slock))
#define file_unlock(fp) 	(simple_unlock(&fp->f_slock))
#define fdesc_lock(fdp) 	(simple_lock(&fdp->fd_slock))
#define fdesc_unlock(fdp) 	(simple_unlock(&fdp->fd_slock))

void
fdesc_init(p, fdp)
	struct proc 	 *p;
	struct filedesc0 *fdp;
{
	u.u_fd = &fdp->fd_fd;
	p->p_fd = u.u_fd;
	fdp->fd_fd.fd_refcnt = 1;
	fdp->fd_fd.fd_cmask = cmask;
	fdp->fd_fd.fd_ofiles = fdp->fd_dfiles;
	fdp->fd_fd.fd_ofileflags = fdp->fd_dfileflags;
	fdp->fd_fd.fd_nfiles = NDFILE;

	LIST_INIT(&filehead);
	simple_lock_init(&fdp->fd_fd.fd_slock, "filedesc_slock");
}

static int
ufalloc(want, result)
	register int want, *result;
{
	register int i;
	int lim, last, nfiles;
	lim = min((int) u.u_rlimit[RLIMIT_NOFILE].rlim_cur, maxfiles);
	for (;;) {
		last = min(u.u_nfiles, lim);
		if ((i = want) < u.u_freefile) {
			i = u.u_freefile;
		}
		for (; i < last; i++) {
			if (u.u_ofile[i] == NULL) {
				u.u_r.r_val1 = i;
				u.u_pofile[i] = 0;
				if (i > u.u_lastfile) {
					u.u_lastfile = i;
				}
				if (want <= u.u_freefile) {
					u.u_freefile = i;
				}
				*result = i;
				return (0);
			}
		}
	}

	u.u_error = EMFILE;
	return (-1);
}

struct file *
falloc()
{
	register struct file *fp, *fq;
	register int i, error;

	error = ufalloc(0, &i);
	if (error != 0) {
		return (NULL);
	}
	if (nfiles >= maxfiles) {
		tablefull("file");
		u.u_error = ENFILE;
		return (NULL);
	}
	nfiles++;
	MALLOC(fp, struct file *, sizeof(struct file), M_FILE, M_WAITOK);
	bzero(fp, sizeof(struct file));
	if ((fq == u.u_ofile[0]) != NULL) {
		LIST_INSERT_AFTER(fq, fp, f_list);
	} else {
		LIST_INSERT_HEAD(&filehead, fp, f_list);
	}
	u.u_ofile[i] = fp;
	fp->f_count = 1;
	fp->f_data = 0;
	fp->f_offset = 0;
	fp->f_cred = u.u_ucred;
	crhold(fp->f_cred);

	return (fp);
}

void
fdexpand(lim)
	int lim;
{
	struct file **newofile;
	char *newofileflags;
	size_t copylen;
	int nfiles;

	if (u.u_nfiles >= lim) {
		return (EMFILE);
	}
	if (u.u_nfiles >= NDEXTENT) {
		nfiles = NDEXTENT;
	} else {
		nfiles = 2 * u.u_nfiles;
	}
	newofile = (struct file **)calloc(nfiles, OFILESIZE, M_FILEDESC, M_WAITOK);
	newofileflags = (char *) &newofile[nfiles];

	/*
	 * Copy the existing ofile and ofileflags arrays
	 * and zero the new portion of each array.
	 */
	copylen = sizeof(struct file *) * u.u_nfiles;
	memcpy(newofile, u.u_ofile, copylen);
	memset((char*) newofile + copylen, 0, nfiles * sizeof(struct file*) - copylen);
	copylen = sizeof(char) * u.u_nfiles;
	memcpy(newofileflags, u.u_pofile, copylen);
	memset(newofileflags + copylen, 0, nfiles * sizeof(char) - copylen);
    if (u.u_nfiles > NDFILE) {
        FREE(u.u_ofile, M_FILEDESC);
    }
    u.u_ofile = newofile;
    u.u_pofile = newofileflags;
    u.u_nfiles = nfiles;
    fdsync(u.u_fd);
}

static int
ufdalloc(result)
	int *result;
{
	struct file **newofile;
	char *newofileflags;
	int error, lim;

	lim = min((int) u.u_rlimit[RLIMIT_NOFILE].rlim_cur, maxfiles);
	error = ufalloc(0, result);
	if (error != 0) {
		/*
		 * No space in current array.  Expand?
		 */
		fdexpand(lim);
	    return (0);
	}
	return (error);
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
 * Check to see whether n user file descriptors
 * are available to the process p.
 */
int
fdavail(n)
	register int n;
{
	register struct file **fpp;
	register int i, lim;

	lim = min((int)u.u_rlimit[RLIMIT_NOFILE].rlim_cur, maxfiles);
	if ((i = lim - u.u_nfiles) > 0 && (n -= i) <= 0)
		return (1);
	fpp = &u.u_ofile[u.u_freefile];
	for (i = u.u_nfiles - u.u_freefile; --i >= 0; fpp++)
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

/* sync to filedesc tables from user */
void
fdsync(fdp)
	struct filedesc *fdp;
{
	memcpy(fdp->fd_ofiles, u.u_ofile, sizeof(struct file**));
	memcpy(fdp->fd_ofileflags, u.u_pofile, sizeof(char*));
	memcpy(fdp->fd_nfiles, u.u_nfiles, sizeof(int));
	memcpy(fdp->fd_lastfile, u.u_lastfile, sizeof(int));
	memcpy(fdp->fd_freefile, u.u_freefile, sizeof(int));
}

/* sync to user from filedesc  */
void
ufdsync(fdp)
	struct filedesc *fdp;
{
	memcpy(u.u_ofile, fdp->fd_ofiles, sizeof(struct file **));
	memcpy(u.u_pofile, fdp->fd_ofileflags, sizeof(char *));
	memcpy(u.u_nfiles, fdp->fd_nfiles, sizeof(int));
	memcpy(u.u_lastfile, fdp->fd_lastfile, sizeof(int));
	memcpy(u.u_freefile, fdp->fd_freefile, sizeof(int));
}

/*
 * Copy a filedesc structure.
 */
struct filedesc *
fdcopy()
{
	register struct filedesc *newfdp, *fdp;
	register struct file **fpp;
	register int i;

	fdp = u.u_fd;
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
		newfdp->fd_ofiles = (struct file **)calloc(i, OFILESIZE, M_FILEDESC, M_WAITOK);
		newfdp->fd_ofileflags = (char*) &newfdp->fd_ofiles[i];
	}
	newfdp->fd_nfiles = i;
	bcopy(fdp->fd_ofiles, newfdp->fd_ofiles, i * sizeof(struct file **));
	bcopy(fdp->fd_ofileflags, newfdp->fd_ofileflags, i * sizeof(char));
	fpp = newfdp->fd_ofiles;
	for (i = newfdp->fd_lastfile; i-- >= 0; fpp++)
		if (*fpp != NULL)
			(*fpp)->f_count++;

	return (newfdp);
}

/*
 * Release a filedesc structure.
 */
void
fdfree(fdp)
	register struct filedesc *fdp;
{
	struct file **fpp;
	register int i;

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
