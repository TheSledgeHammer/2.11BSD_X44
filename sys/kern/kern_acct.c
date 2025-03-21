/*	$NetBSD: kern_acct.c,v 1.47 1998/08/04 04:03:11 perry Exp $	*/

/*-
 * Copyright (c) 1994 Christopher G. Demetriou
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)kern_acct.c	8.8 (Berkeley) 5/14/95
 */
/*-
 * Copyright (c) 1982, 1986, 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 *
 *	@(#)kern_acct.c	7.25 (Berkeley) 7/10/92
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/namei.h>
#include <sys/errno.h>
#include <sys/acct.h>
#include <sys/resourcevar.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/user.h>

/*
 * Internal accounting functions.
 * The former's operation is described in Leffler, et al., and the latter
 * was provided by UCB with the 4.4BSD-Lite release
 */
comp_t	encode_comp_t(u_long, u_long);
void	acctwatch(void *);

/* Accounting vnode pointer, and saved vnode pointer. */
struct vnode *acctp;
struct vnode *savacctp;

/*
 * Values associated with enabling and disabling accounting
 */
int	acctsuspend = 2;	/* stop accounting when < 2% free space left */
int	acctresume = 4;		/* resume when free space risen to > 4% */
int	acctchkfreq = 15;	/* frequency (in seconds) to check space */

/*
 * Accounting system call.  Written based on the specification and
 * previous implementation done by Mark Tinguely.
 */

int
acct()
{
	register struct acct_args {
		syscallarg(char	*) path;
		syscallarg(pid_t)	pid;
	} *uap = (struct acct_args *)u.u_ap;

	struct proc *p;
	struct nameidata nd;
	int error;

	if(SCARG(uap, pid) == 0) {
		SCARG(uap, pid) = u.u_procp->p_pid;
	}
	p = pfind(SCARG(uap, pid));
	if(p == 0) {
		return (u.u_error = ESRCH);
	}

	/* Make sure that the caller is root. */
	error = suser();
	if (error) {
		return (error);
	}

	/*
	 * If accounting is to be started to a file, open that file for
	 * writing and make sure it's a 'normal'.
	 */
	if (SCARG(uap, path) != NULL) {
		NDINIT(&nd, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
		error = vn_open(&nd, FWRITE, 0);
		if (error) {
			return (error);
		}
		VOP_UNLOCK(nd.ni_vp, 0, p);
		if (nd.ni_vp->v_type != VREG) {
			vn_close(nd.ni_vp, FWRITE, p->p_ucred, p);
			return (EACCES);
		}
	}

	/*
	 * If accounting was previously enabled, kill the old space-watcher,
	 * close the file, and (if no new file was specified, leave).
	 */
	if (acctp != NULLVP || savacctp != NULLVP) {
		untimeout(acctwatch, NULL);
		error = vn_close((acctp != NULLVP ? acctp : savacctp), FWRITE, p->p_ucred, p);
		acctp = savacctp = NULLVP;
	}
	if (SCARG(uap, path) == NULL) {
		return (error);
	}

	/*
	 * Save the new accounting file vnode, and schedule the new
	 * free space watcher.
	 */
	acctp = nd.ni_vp;
	acctwatch(NULL);

	return (error);
}

/*
 * Write out process accounting information, on process exit.
 * Data to be written out is specified in Leffler, et al.
 * and are enumerated below.  (They're also noted in the system
 * "acct.h" header file.)
 */

int
acct_process(p)
	struct proc *p;
{
	struct acct acct;
	struct rusage *r;
	struct timeval ut, st, tmp;
	int s, t;
	struct vnode *vp;

	/* If accounting isn't enabled, don't bother */
	vp = acctp;
	if (vp == NULLVP)
		return (0);

	/*
	 * Get process accounting information.
	 */

	/* (1) The name of the command that ran */
	bcopy(p->p_comm, acct.ac_comm, sizeof acct.ac_comm);

	/* (2) The amount of user and system time that was used */
	calcru(p, &ut, &st, NULL);
	acct.ac_utime = encode_comp_t(ut.tv_sec, ut.tv_usec);
	acct.ac_stime = encode_comp_t(st.tv_sec, st.tv_usec);

	/* (3) The elapsed time the commmand ran (and its starting time) */
	acct.ac_btime = u.u_start.tv_sec;

	s = splclock();
	tmp = time;
	splx(s);
	timevalsub(&tmp, &u.u_start);
	acct.ac_etime = encode_comp_t(tmp.tv_sec, tmp.tv_usec);

	/* (4) The average amount of memory used */
	r = &p->p_stats->p_ru;
	tmp = ut;
	timevaladd(&tmp, &st);
	t = tmp.tv_sec * hz + tmp.tv_usec / tick;
	if (t)
		acct.ac_mem = (r->ru_ixrss + r->ru_idrss + r->ru_isrss) / t;
	else
		acct.ac_mem = 0;

	/* (5) The number of disk I/O operations done */
	acct.ac_io = encode_comp_t(r->ru_inblock + r->ru_oublock, (long) 0);

	/* (6) The UID and GID of the process */
	acct.ac_uid = p->p_cred->p_ruid;
	acct.ac_gid = p->p_cred->p_rgid;

	/* (7) The terminal from which the process was started */
	if ((p->p_flag & P_CONTROLT) && p->p_pgrp->pg_session->s_ttyp)
		acct.ac_tty = p->p_pgrp->pg_session->s_ttyp->t_dev;
	else
		acct.ac_tty = NODEV;

	/* (8) The boolean flags that tell how the process terminated, etc. */
	acct.ac_flag = u.u_acflag;

	/*
	 * Now, just write the accounting information to the file.
	 */
	VOP_LEASE(vp, p, p->p_ucred, LEASE_WRITE);
	return (vn_rdwr(UIO_WRITE, vp, (caddr_t) & acct, sizeof(acct), (off_t) 0,
			UIO_SYSSPACE, IO_APPEND | IO_UNIT, p->p_ucred, (int*) 0, p));
}

#define	MANTSIZE	13						/* 13 bit mantissa. */
#define	EXPSIZE		3						/* Base 8 (3 bit) exponent. */
#define	MAXFRACT	((1 << MANTSIZE) - 1)	/* Maximum fractional value. */

comp_t
encode_comp_t(s, us)
	u_long s, us;
{
	int exp, rnd;

	exp = 0;
	rnd = 0;
	s *= AHZ;
	s += us / (1000000 / AHZ);		/* Maximize precision. */

	while (s > MAXFRACT) {
	rnd = s & (1 << (EXPSIZE - 1));	/* Round up? */
		s >>= EXPSIZE;				/* Base 8 exponent == 3 bit shift. */
		exp++;
	}

	/* If we need to round up, do it (and handle overflow correctly). */
	if (rnd && (++s > MAXFRACT)) {
		s >>= EXPSIZE;
		exp++;
	}

	/* Clean it up and polish it off. */
	exp <<= MANTSIZE;				/* Shift the exponent into place */
	exp += s;						/* and add on the mantissa. */
	return (exp);
}

/*
 * Periodically check the file system to see if accounting
 * should be turned on or off.  Beware the case where the vnode
 * has been vgone()'d out from underneath us, e.g. when the file
 * system containing the accounting file has been forcibly unmounted.
 */
/* ARGSUSED */
void
acctwatch(a)
	void *a;
{
	struct statfs sb;
	if (savacctp != NULLVP) {
		if (savacctp->v_type == VBAD) {
			(void) vn_close(savacctp, FWRITE, NOCRED, NULL);
			savacctp = NULLVP;
			return;
		}
		(void) VFS_STATFS(savacctp->v_mount, &sb, (struct proc*) 0);
		if (sb.f_bavail > acctresume * sb.f_blocks / 100) {
			acctp = savacctp;
			savacctp = NULLVP;
			log(LOG_NOTICE, "Accounting resumed\n");
		}
	} else if (acctp != NULLVP) {
		if (acctp->v_type == VBAD) {
			(void) vn_close(acctp, FWRITE, NOCRED, NULL);
			acctp = NULLVP;
			return;
		}
		(void) VFS_STATFS(acctp->v_mount, &sb, (struct proc*) 0);
		if (sb.f_bavail <= acctsuspend * sb.f_blocks / 100) {
			savacctp = acctp;
			acctp = NULLVP;
			log(LOG_NOTICE, "Accounting suspended\n");
		}
	} else
		return;
	timeout(acctwatch, NULL, acctchkfreq * hz);
}
