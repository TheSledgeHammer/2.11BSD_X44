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

struct filelists filehead;	/* head of list of open files */
int 			 nfiles;		/* actual number of open files */

struct file *
fd_getfile(fd)
	int fd;
{
	struct file *fp;

	if ((u_int) fd >= u.u_nfiles || (fp = u.u_ofile[fd]) == NULL) {
		return (NULL);
	}

	simple_lock(&fp->f_slock);
	if (FILE_IS_USABLE(fp) == 0) {
		simple_unlock(&fp->f_slock);
		return (NULL);
	}
	return (fp);
}

/*
 * Make this process not share its filedesc structure, maintaining
 * all file descriptor state.
 */
void
fdunshare()
{
	struct filedesc *newfd;

	if (u.u_fd->fd_refcnt == 1)
		return;

	newfd = fdcopy(u.u_fd);
	fdfree(u.u_fd);
	u.u_fd = newfd;
}

void
fdremove(fd)
	int fd;
{
	u.u_ofile[fd] = NULL;
	fd_unused(fd);
}

int
fdrelease(fd)
	int fd;
{
	struct filedesc	*fdp;
	struct file	**fpp, *fp;

	fdp = u.u_fd;
	fpp = &u.u_ofile[fd];
	fp = *fpp;
	if (fp == NULL) {
		return (EBADF);
	}

	simple_lock(&fp->f_slock);
	if (!FILE_IS_USABLE(fp)) {
		simple_unlock(&fp->f_slock);
		return (EBADF);
	}

	FILE_USE(fp);

	*fpp = NULL;
	fdp->fd_ofileflags[fd] = 0;
	if (fd < fdp->fd_knlistsize)
		knote_fdclose(p, fd);
	fd_unused(fd);
	return (closef(fp));
}

void
fdcloseexec(void)
{
	int	fd;

	fdunshare();
	for (fd = 0; fd <= u.u_lastfile; fd++) {
		if (u.u_ofile[fd] & UF_EXCLOSE) {
			(void) fdrelease(fd);
		}
	}
}
