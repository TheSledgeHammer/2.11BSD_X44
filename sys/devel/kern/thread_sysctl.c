/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
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
 *	@(#)kern_sysctl.c	8.4.11 (2.11BSD) 1999/8/11
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/boot.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/map.h>
#include <sys/sysctl.h>

#define KERN_PROC_THREAD	7 /* traverse proc threads */

#define KERN_THREAD_ALL		0
#define KERN_THREAD_TID		1
#define KERN_THREAD_PGRP	2
#define	KERN_THREAD_SESSION	3	/* by session of pid - NOT IN 2.11 */
#define	KERN_THREAD_TTY		4	/* by controlling tty */
#define KERN_THREAD_UID		5
#define KERN_THREAD_RUID	6

/*
 * KERN_THREAD subtype ops return arrays of augmented thread structures:
 */
struct kinfo_thread {
	struct thread 		kt_thread;				/* thread structure */
	struct ethread {
		struct	thread	*e_taddr;				/* address of thread */
		struct	session *e_sess;				/* session pointer */

		struct	pcred 	e_pcred;				/* thread credentials */
		struct	ucred 	e_ucred;				/* current credentials */

		pid_t			e_ptid;					/* parent process id */
		pid_t			e_pgid;					/* process group id */
		short			e_jobc;					/* job control counter */
		dev_t			e_tdev;					/* controlling tty dev */
		pid_t			e_tpgid;				/* tty process group id */

		struct	session *e_tsess;				/* tty session pointer */

#define	WMESGLEN		7
		char			e_wmesg[WMESGLEN+1];	/* wchan message */
		segsz_t 		e_xsize;				/* text size */
		short			e_xrssize;				/* text rss */
		short			e_xccount;				/* text references */
		short			e_xswrss;
		long			e_flag;
	} kt_ethread;
};

#define KERN_THREADSLOP	(5 * sizeof (struct kinfo_thread))

int
sysctl_dothread(name, namelen, where, sizep)
	int *name;
	u_int namelen;
	char *where;
	size_t *sizep;
{
	register struct proc *p;
	register struct thread *td;
	register struct kinfo_thread *dt = (struct kinfo_thread *)where;
	int needed = 0;
	int buflen = where != NULL ? *sizep : 0;
	struct ethread ethread;
	int error = 0;

	if (namelen != 2 && !(namelen == 1 && name[0] == KERN_THREAD_ALL))
		return (EINVAL);
	td = LIST_FIRST(&p->p_allthread);

	for (; td != NULL; td = LIST_NEXT(td, td_list)) {

		if (td->td_stat == SIDL) {
			continue;
		}

		switch (name[0]) {

		case KERN_THREAD_TID:
			/* could do this with just a lookup */
			if (td->td_tid != (pid_t)name[1])
				continue;
			break;

		case KERN_THREAD_PGRP:
			/* could do this by traversing pgrp */
			if (td->td_pgrp->pg_id != (pid_t)name[1])
				continue;
			break;

		case KERN_THREAD_TTY:
			if ((td->td_procp->p_flag & P_CONTROLT) == 0
					|| td->td_procp->p_session->s_ttyp == NULL
					|| td->td_procp->p_session->s_ttyp->t_dev
							!= (dev_t) name[1]) {
				continue;
			}
			break;

		case KERN_THREAD_UID:
			if (td->td_ucred->cr_uid != (uid_t)name[1])
				continue;
			break;

		case KERN_THREAD_RUID:
			if (td->td_cred->p_ruid != (uid_t)name[1])
				continue;
			break;

		case KERN_THREAD_ALL:
			break;
		default:
			return(EINVAL);
		}
		if (buflen >= sizeof(struct kinfo_thread)) {
			fill_eproc(p, &ethread);
			if (error == copyout((caddr_t)p, &dt->kt_thread, sizeof(struct thread)))
				return (error);
			if (error == copyout((caddr_t)&ethread, &dt->kt_ethread, sizeof(ethread)))
				return (error);
			dt++;
			buflen -= sizeof(struct kinfo_thread);
		}
		needed += sizeof(struct kinfo_thread);
	}
	if (where != NULL) {
		*sizep = (caddr_t)dt - where;
		if (needed > *sizep) {
			return (ENOMEM);
		}
	} else {
		needed += KERN_THREADSLOP;
		*sizep = needed;
	}
	return (0);
}

void
fill_ethread(p, td, etd)
	register struct thread *td;
	register struct ethread *etd;
{
	register struct proc *p = td->td_procp;
	struct	tty	*ttyp;

	etd->e_taddr = td;
	etd->e_sess = td->td_pgrp->pg_session;
	etd->e_pcred = *td->td_cred;
	etd->e_ucred = *td->td_ucred;
	if (td->td_stat == SIDL || td->td_stat == SZOMB) {

	} else {

	}
	if (td->td_procp) {
		etd->e_ptid = td->td_procp->p_pid;
	} else {
		etd->e_ptid = 0;
	}
	etd->e_pgid = td->td_pgrp->pg_id;
	etd->e_jobc = p->p_pgrp->pg_jobc;
	if ((td->td_procp->p_flag & P_CONTROLT) && (ttyp = etd->e_sess->s_ttyp)) {
		etd->e_tdev = ttyp->t_dev;
		etd->e_tpgid = ttyp->t_pgrp ? ttyp->t_pgrp->pg_id : NO_PID;
		etd->e_tsess = ttyp->t_session;
	} else {
		etd->e_tdev = NODEV;
	}
	etd->e_flag = etd->e_sess->s_ttyvp ? EPROC_CTTY : 0;
	if (SESS_LEADER(td->td_procp)) {
		etd->e_flag |= EPROC_SLEADER;
	}
	if (p->p_wmesg) {
		strncpy(etd->e_wmesg, td->td_wmesg, WMESGLEN);
	}
	etd->e_xsize = etd->e_xrssize = 0;
	etd->e_xccount = etd->e_xswrss = 0;
}
