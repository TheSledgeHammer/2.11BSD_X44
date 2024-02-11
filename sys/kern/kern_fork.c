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
 *	@(#)kern_fork.c	8.8 (Berkeley) 2/14/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_fork.c	1.6 (2.11BSD) 1999/8/11
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>
#include <sys/acct.h>
#include <sys/ktrace.h>
#include <sys/vmsystm.h>

#include <machine/setjmp.h>

struct forkstat forkstat;

static int fork1(int);

/*
 * fork --
 *	fork system call
 */
int
fork()
{
	return (fork1(0));
}

/*
 * vfork --
 *	vfork system call, fast version of fork
 */
int
vfork()
{
	return (fork1(1));
}

int	nproc = 1;		/* process 0 */

static int
fork1(isvfork)
	int isvfork;
{
	register int a;
	register struct proc *p1, *p2;
	register uid_t uid;
	int count;

	a = 0;
	if (u.u_uid != 0) {
		for (p1 = LIST_FIRST(&allproc); p1; p1 = LIST_NEXT(p1, p_list)) {
			if (p1->p_uid == u.u_uid) {
				a++;
			}
		}
		for (p1 = LIST_FIRST(&zombproc); p1; p1 = LIST_NEXT(p1, p_list)) {
			if (p1->p_uid == u.u_uid) {
				a++;
			}
		}
	}
	/*
	 * Disallow if
	 *  No processes at all;
	 *  not su and too many procs owned; or
	 *  not su and would take last slot.
	 */
	p2 = LIST_FIRST(&freeproc);
	if (p2 == NULL) {
		tablefull("proc");
		/* Allocate new proc. */
		MALLOC(p2, struct proc *, sizeof(struct proc), M_PROC, M_WAITOK);
	}
	uid = p1->p_cred->p_ruid;
	if (p2 == NULL || (nproc >= maxproc - 1 && uid != 0) || nproc >= maxproc) {
		u.u_error = EAGAIN;
		goto out;
	}
	count = chgproccnt(u.u_uid, 1);
	if (u.u_uid != 0 && count > p1->p_rlimit[RLIMIT_NPROC].rlim_cur) {
		(void) chgproccnt(u.u_uid, -1);
		u.u_error = EAGAIN;
		goto out;
	}

	if (p2 == NULL || (u.u_uid != 0 && (LIST_NEXT(p2, p_list) == NULL || a > MAXUPRC))) {
		u.u_error = EAGAIN;
		goto out;
	}
	p1 = u.u_procp;
	if (newproc(isvfork)) {
		u.u_r.r_val1 = p1->p_pid;
		u.u_r.r_val2 = 1;  				/* child */
		u.u_start.tv_sec = time.tv_sec;
		u.u_start.tv_usec = time.tv_usec;

		/* set forked but preserve suid/gid state */
		u.u_acflag = AFORK | (u.u_acflag & ASUGID);
		bzero(&u.u_ru, sizeof(u.u_ru));
		bzero(&u.u_cru, sizeof(u.u_cru));
		return (0);
	}

	u.u_r.r_val1 = p2->p_pid;
	return (0);

out:
	u.u_r.r_val2 = 0;
	return (u.u_error);
}

/*
 * newproc --
 *	Create a new process -- the internal version of system call fork.
 *	It returns 1 in the new process, 0 in the old.
 */
int
newproc(isvfork)
	int isvfork;
{
	register struct proc *rip, *rpp;
	static int mpid, pidchecked = 0;

	mpid++;
retry:
	if(mpid >= PID_MAX) {
		mpid = 100;
		pidchecked = 0;
	}
	if (mpid >= pidchecked) {
		int doingzomb = 0;

		pidchecked = PID_MAX;

		rpp = LIST_FIRST(&allproc);
again:
		for (; rpp != NULL; rpp = LIST_NEXT(rpp, p_list)) {
			while (rpp->p_pid == mpid || rpp->p_pgrp->pg_id == mpid) {
				mpid++;
				if (mpid >= pidchecked)
					goto retry;
			}
			if (rpp->p_pid > mpid && pidchecked > rpp->p_pid)
				pidchecked = rpp->p_pid;
			if (rpp->p_pgrp->pg_id > mpid && pidchecked > rpp->p_pgrp->pg_id)
				pidchecked = rpp->p_pgrp->pg_id;
		}
		if (!doingzomb) {
			doingzomb = 1;
			rpp = LIST_FIRST(&zombproc);
			goto again;
		}
	}
	rpp = LIST_FIRST(&freeproc);
	if (rpp == NULL) {
		panic("no procs");
	}
	LIST_REMOVE(rpp, p_list);				/* off freeproc */

	/*
	 * Make a proc table entry for the new process.
	 */
	nproc++;
	rip = u.u_procp;
	rpp->p_stat = SIDL;
	rpp->p_pid = mpid;
	rpp->p_realtimer.it_value = 0;
	rpp->p_flag = P_SLOAD;
	rpp->p_uid = rip->p_uid;
	rpp->p_pgrp = rip->p_pgrp;
	rpp->p_nice = rip->p_nice;
	rpp->p_ppid = rip->p_pid;
	rpp->p_pptr = rip;
	rpp->p_time = 0;
	rpp->p_cpu = 0;
	rpp->p_sigmask = rip->p_sigmask;
	rpp->p_sigcatch = rip->p_sigcatch;
	rpp->p_sigignore = rip->p_sigignore;
	/* take along any pending signals like stops? */
	if (isvfork) {
		forkstat.cntvfork++;
		forkstat.sizvfork += rip->p_dsize + rip->p_ssize;
	} else {
		forkstat.cntfork++;
		forkstat.sizfork += rip->p_dsize + rip->p_ssize;
	}
	rpp->p_wchan = 0;
	rpp->p_slptime = 0;

	LIST_INSERT_HEAD(PIDHASH(rpp->p_pid), rpp, p_hash);

	/*
	 * some shuffling here -- in most UNIX kernels, the allproc assign
	 * is done after grabbing the struct off of the freeproc list.  We
	 * wait so that if the clock interrupts us and vmtotal walks allproc
	 * the text pointer isn't garbage.
	 */
	LIST_INSERT_HEAD(&allproc, rpp, p_list); 		 	/* onto allproc: (allproc is never NULL) */

	/*
	 * Make a proc table entry for the new process.
	 * Start by zeroing the section of proc that is zero-initialized,
	 * then copy the section that is copied directly from the parent.
	 */
	bzero(&rpp->p_startzero, (unsigned) ((caddr_t)&rpp->p_endzero - (caddr_t)&rpp->p_startzero));
	bzero(&rip->p_startzero, (unsigned) ((caddr_t)&rpp->p_endzero - (caddr_t)&rpp->p_startzero));

	rpp->p_flag = P_INMEM;
	if (rip->p_flag & P_PROFIL) {
		startprofclock(rip);
	}
	MALLOC(rpp->p_cred, struct pcred *, sizeof(struct pcred), M_SUBPROC, M_WAITOK);
	bcopy(rip->p_cred, rpp->p_cred, sizeof(*rpp->p_cred));
	rpp->p_cred->p_refcnt = 1;
	crhold(rip->p_ucred);

	/* bump references to the text vnode (for procfs) */
	rpp->p_textvp = rip->p_textvp;
	if(rpp->p_textvp)
		VREF(rpp->p_textvp);

	rpp->p_fd = fdcopy(rip->p_fd);

	/*
	 * If p_limit is still copy-on-write, bump refcnt,
	 * otherwise get a copy that won't be modified.
	 * (If PL_SHAREMOD is clear, the structure is shared
	 * copy-on-write.)
	 */
	if (rip->p_limit->p_lflags & PL_SHAREMOD)
		rpp->p_limit = limcopy(rip->p_limit);
	else {
		rpp->p_limit = rip->p_limit;
		rpp->p_limit->p_refcnt++;
	}

	if (rip->p_session->s_ttyvp != NULL && (rip->p_flag & P_CONTROLT))
		rpp->p_flag |= P_CONTROLT;

	if (isvfork)
		rpp->p_flag |= P_PPWAIT;

	LIST_INSERT_AFTER(rip, rpp, p_pglist);
	rpp->p_pptr = rip;
	LIST_INSERT_HEAD(&rip->p_children, rpp, p_sibling);
	LIST_INIT(&rpp->p_children);

	if (setjmp(&u.u_ssave)) {
		sureg();
		return (1);
	}

	rpp->p_dsize = rip->p_dsize;
	rpp->p_ssize = rip->p_ssize;
	rpp->p_daddr = rip->p_daddr;
	rpp->p_saddr = rip->p_saddr;

    /*
     * Partially simulate the environment of the new process so that
     * when it is actually created (by copying) it will look right.
     */
	u.u_procp = rpp;

	/*
	 * set priority of child to be that of parent
	 */
	rpp->p_estcpu = rip->p_estcpu;

#ifdef KTRACE
	if (rip->p_traceflag & KTRFAC_INHERIT) {
		rpp->p_traceflag = rip->p_traceflag;
		if ((rpp->p_tracep = rip->p_tracep) != NULL) {
			ktradref(rpp);
		}
	}
#endif

	/*
	 * This begins the section where we must prevent the parent
	 * from being swapped.
	 */
	rip->p_flag |= P_NOSWAP;

	/*
	 * Set return values for child before vm_fork,
	 * so they can be copied to child stack.
	 * We return parent pid, and mark as child in retval[1].
	 * NOTE: the kernel stack may be at a different location in the child
	 * process, and thus addresses of automatic variables (including retval)
	 * may be invalid after vm_fork returns in the child process.
	 */

	u.u_r.r_val1 = rip->p_pid;
	u.u_r.r_val2 = 1;
	if(vm_fork(rip, rpp, isvfork)) {
		/*
		 * Child process.  Set start time and get to work.
		 */
		(void) splclock();
		rpp->p_stats->p_start = time;
		(void) spl0();
		rpp->p_acflag = AFORK;
		return (0);
	}

	/*
	 * Make child runnable and add to run queue.
	 */
	(void) splhigh();
	rpp->p_stat = SRUN;
	setrq(rpp);
	(void) spl0();

	/*
	 * Now can be swapped.
	 */
	rip->p_flag |= P_NOSWAP;

	/*
	 * Preserve synchronization semantics of vfork.  If waiting for
	 * child to exec or exit, set P_PPWAIT on child, and sleep on our
	 * proc (in case of exit).
	 */
	if (isvfork) {
		while (rpp->p_flag & P_PPWAIT) {
			tsleep(rip, PWAIT, "ppwait", 0);
		}
	}

	/*
	 * Return child pid to parent process,
	 * marking us as parent via retval[1].
	 */
	u.u_r.r_val1 = rpp->p_pid;
	u.u_r.r_val2 = 0;

	return (0);
}
