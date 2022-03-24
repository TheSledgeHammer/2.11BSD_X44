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

#include <vm/include/vm_systm.h>

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

int
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
	//p2 = freeproc;
	uid = p1->p_cred->p_ruid;
	if (p2 == NULL || (nproc >= maxproc - 1 && uid != 0) || nproc >= maxproc) {
		tablefull("proc");
		u.u_error = EAGAIN;
		goto out;
	}
	count = chgproccnt(u.u_uid, 1);
	if (u.u_uid != 0 && count > p1->p_rlimit[RLIMIT_NPROC].rlim_cur) {
		(void) chgproccnt(u.u_uid, -1);
		u.u_error = EAGAIN;
		goto out;
	}

	/* Allocate new proc. */
	MALLOC(p2, struct proc *, sizeof(struct proc), M_PROC, M_WAITOK);
	if (p2 == NULL || (u.u_uid != 0 && (LIST_NEXT(p2, p_list) == NULL || a > MAXUPRC))) {
		u.u_error = EAGAIN;
		goto out;
	}
	p1 = u.u_procp;
	if (newproc(isvfork)) {
		u.u_r.r_val1 = p1->p_pid;
		u.u_r.r_val2 = 1;  				/* child */
		u.u_start = p1->p_rtime->tv_sec;

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
	struct proc *newproc;
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
			rpp = zombproc;
			goto again;
		}
	}
	/*
	if ((rpp = freeproc) == NULL)
			panic("no procs");
	 */
	//freeproc = rpp->p_nxt;				/* off freeproc */

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
	rpp->p_rtime = 0;
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

	/* onto allproc */
	LIST_INSERT_HEAD(&allproc, rpp, p_list); 	/* (allproc is never NULL) */
	rpp->p_forw = rpp->p_back = NULL;			/* shouldn't be necessary */
	LIST_INSERT_HEAD(PIDHASH(rpp->p_pid), rpp, p_hash);

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
			VREF(rpp->p_tracep);
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
	if (isvfork)
		while (rpp->p_flag & P_PPWAIT)
			tsleep(rip, PWAIT, "ppwait", 0);

	/*
	 * Return child pid to parent process,
	 * marking us as parent via retval[1].
	 */
	u.u_r.r_val1 = rpp->p_pid;
	u.u_r.r_val2 = 0;

	return (0);
}


/* 2.11BSD Modified Original newproc */
int
copyproc(rip, rpp, isvfork)
	register struct proc *rpp, *rip;
	int isvfork;
{
	register int n;
	struct file *fp;
	int a1, s;
	size_t a[3];

	/*
	 * Increase reference counts on shared objects.
	 */
	for (n = 0; n <= u->u_lastfile; n++) {
		fp = u->u_ofile[n];
		if (fp == NULL)
			continue;
		fp->f_count++;
	}

	rpp->p_dsize = rip->p_dsize;
	rpp->p_ssize = rip->p_ssize;
	rpp->p_daddr = rip->p_daddr;
	rpp->p_saddr = rip->p_saddr;
	a1 = rip->p_addr;
	if (isvfork) {
		a[2] = rmalloc(coremap, USIZE);
	} else {
		a[2] = rmalloc3(coremap, rip->p_dsize, rip->p_ssize, USIZE, a);
	}

	/*
	 * Partially simulate the environment of the new process so that
	 * when it is actually created (by copying) it will look right.
	 */
	u->u_procp = rpp;

	/*
	 * If there is not enough core for the new process, swap out the
	 * current process to generate the copy.
	 */
	if (a[2] == NULL) {
		rip->p_stat = SIDL;
		rpp->p_addr = a1;
		rpp->p_stat = SRUN;
		swapout(rpp);
		rip->p_stat = SRUN;
		u->u_procp = rip;
	} else {
		/*
		 * There is core, so just copy.
		 */
		rpp->p_addr = a[2];
		copy(a1, rpp->p_addr, USIZE);
		u->u_procp = rip;
		if (isvfork == 0) {
			rpp->p_daddr = a[0];
			copy(rip->p_daddr, rpp->p_daddr, rpp->p_dsize);
			rpp->p_saddr = a[1];
			copy(rip->p_saddr, rpp->p_saddr, rpp->p_ssize);
		}
		s = splhigh();
		rpp->p_stat = SRUN;
		setrq(rpp);
		splx(s);
		splhigh();
	}
	rpp->p_flag |= P_SSWAP;

	if (isvfork) {
		/*
		 *  Set the parent's sizes to 0, since the child now
		 *  has the data and stack.
		 *  (If we had to swap, just free parent resources.)
		 *  Then wait for the child to finish with it.
		 */
		rip->p_dsize = 0;
		rip->p_ssize = 0;
		rip->p_textvp = NULL;
		rpp->p_flag |= P_SVFORK;
		rip->p_flag |= P_SVFPRNT;
		if (a[2] == NULL) {
			mfree(coremap, rip->p_dsize, rip->p_daddr);
			mfree(coremap, rip->p_ssize, rip->p_saddr);
		}
		while (rpp->p_flag & P_SVFORK)
			sleep((caddr_t)rpp, PSWP + 1);
		if ((rpp->p_flag & P_SLOAD) == 0)
			panic("newproc vfork");
		u->u_dsize = rip->p_dsize = rpp->p_dsize;
		rip->p_daddr = rpp->p_daddr;
		rpp->p_dsize = 0;
		u->u_ssize = rip->p_ssize = rpp->p_ssize;
		rip->p_saddr = rpp->p_saddr;
		rpp->p_ssize = 0;
		rpp->p_flag |= P_SVFDONE;
		wakeup((caddr_t)rip);

		rip->p_flag &= ~P_SVFPRNT;
	}
	return (0);
}
