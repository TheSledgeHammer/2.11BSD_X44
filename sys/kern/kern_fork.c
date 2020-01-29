/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_fork.c	1.6 (2.11BSD) 1999/8/11
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <vm/include/vm.h>


/*
 * fork --
 *	fork system call
 */
void
fork()
{
	fork1(0);
}

/*
 * vfork --
 *	vfork system call, fast version of fork
 */
void
vfork()
{
	fork1(1);
}

static void
fork1(isvfork)
{
	register int a;
	register struct proc *p1, *p2;
	int count;

	a = 0;
	if (u->u_uid != 0) {
		for (p1 = allproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u->u_uid)
				a++;
		for (p1 = zombproc; p1; p1 = p1->p_nxt)
			if (p1->p_uid == u->u_uid)
				a++;
	}
	/*
	 * Disallow if
	 *  No processes at all;
	 *  not su and too many procs owned; or
	 *  not su and would take last slot.
	 */
	p2 = freeproc;
	if (p2==NULL)
		tablefull("proc");

	count = chgproccnt(u->u_uid, 1);
	if (u->u_uid != 0 && count > p1->p_rlimit[RLIMIT_NPROC].rlim_cur) {
		(void)chgproccnt(uid, -1);
		u->u_error = EAGAIN;
	}
	if (p2==NULL || (u->u_uid!=0 && (p2->p_nxt == NULL || a>MAXUPRC))) {
		u->u_error = EAGAIN;
		goto out;
	}
	p1 = u->u_procp;
	if (newproc(isvfork)) {
		u->u_r.r_val1 = p1->p_pid;
		u->u_start = p1->p_rtime->tv_sec; //time.tv_sec;
		/* set forked but preserve suid/gid state */
		u->u_acflag = AFORK | (u->u_acflag & ASUGID);
		bzero(&u->u_ru, sizeof(u->u_ru));
		bzero(&u->u_cru, sizeof(u->u_cru));
		return;
	}
	u->u_r.r_val1 = p2->p_pid;

out:
	u->u_r.r_val2 = 0;
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
	register struct proc *rpp, *rip;
	register int n;
	static int pidchecked = 0;
	struct file *fp;
	int a1, s;
	size_t a[3];

	/*
	 * First, just locate a slot for a process
	 * and copy the useful info from this process into it.
	 * The panic "cannot happen" because fork has already
	 * checked for the existence of a slot.
	 */
	mpid++;
retry:
	if (mpid >= 30000) {
		mpid = 100;
		pidchecked = 0;
	}
	if (mpid >= pidchecked) {
		int doingzomb = 0;

		pidchecked = 30000;
		/*
		 * Scan the proc table to check whether this pid
		 * is in use.  Remember the lowest pid that's greater
		 * than mpid, so we can avoid checking for a while.
		 */
		rpp = allproc;
again:
		for (; rpp != NULL; rpp = rpp->p_nxt) {
			if (rpp->p_pid == mpid || rpp->p_pgrp == mpid) {
				mpid++;
				if (mpid >= pidchecked)
					goto retry;
			}
			if (rpp->p_pid > mpid && pidchecked > rpp->p_pid)
				pidchecked = rpp->p_pid;
			if (rpp->p_pgrp > mpid && pidchecked > rpp->p_pgrp)
				pidchecked = rpp->p_pgrp;
		}
		if (!doingzomb) {
			doingzomb = 1;
			rpp = zombproc;
			goto again;
		}
	}
	if ((rpp = freeproc) == NULL)
		panic("no procs");

	freeproc = rpp->p_nxt;			/* off freeproc */

	/*
	 * Make a proc table entry for the new process.
	 */
	rip = u->u_procp;
	rpp->p_stat = SIDL;
	rpp->p_realtimer.it_value = 0;
	rpp->p_flag = P_SLOAD;
	rpp->p_uid = rip->p_uid;
	rpp->p_pgrp = rip->p_pgrp;
	rpp->p_nice = rip->p_nice;
	rpp->p_pid = mpid;
	rpp->p_ppid = rip->p_pid;
	rpp->p_pptr = rip;
	rpp->p_rtime = 0;
	rpp->p_cpu = 0;
	rpp->p_sigmask = rip->p_sigmask;
	rpp->p_sigcatch = rip->p_sigcatch;
	rpp->p_sigignore = rip->p_sigignore;
	/* take along any pending signals like stops? */
	rpp->p_wchan = 0;
	rpp->p_slptime = 0;
	{
	struct proc **hash = &pidhash[PIDHASH(rpp->p_pid)];

	rpp->p_hash = *hash;
	*hash = rpp;
	}
	/*
	 * some shuffling here -- in most UNIX kernels, the allproc assign
	 * is done after grabbing the struct off of the freeproc list.  We
	 * wait so that if the clock interrupts us and vmtotal walks allproc
	 * the text pointer isn't garbage.
	 */
	rpp->p_nxt = allproc;				/* onto allproc */
	rpp->p_nxt->p_prev = &rpp->p_nxt;	/*   (allproc is never NULL) */
	rpp->p_prev = &allproc;
	allproc = rpp;

	/*
	 * Increase reference counts on shared objects.
	 */
	for (n = 0; n <= u->u_lastfile; n++) {
		fp = u->u_ofile[n];
		if (fp == NULL)
			continue;
		fp->f_count++;
	}
	if((rip->p_textvp != NULL) && !isvfork) {
		VREF(rip->p_textvp);
	}
	VREF(u->u_cdir);
	if (u->u_rdir) {
		VREF(u->u_rdir);
	}

	/*
	 * When the longjmp is executed for the new process,
	 * here's where it will resume.
	 */
	if (setjmp(&u->u_ssave)) {
		sureg();
		return(1);
	}

	/* bump references to the text vnode (for procfs) */
	rpp->p_textvp = rip->p_textvp;
	if(rpp->p_textvp)
		VREF(rpp->p_textvp);
	rpp->p_fd = fdcopy(rip);

	rpp->p_dsize = rip->p_dsize;
	rpp->p_ssize = rip->p_ssize;
	rpp->p_daddr = rip->p_daddr;
	rpp->p_saddr = rip->p_saddr;
	a1 = rip->p_addr;
	if (isvfork)
		a[2] = rmalloc(coremap, USIZE);
	else
		a[2] = rmalloc3(coremap, rip->p_dsize, rip->p_ssize, USIZE, a);

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
	}
	else {
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
	}
	rpp->p_flag |= P_SSWAP;
	if (isvfork) {
		/*
		 *  Set the parent's sizes to 0, since the child now
		 *  has the data and stack.
		 *  (If we had to swap, just free parent resources.)
		 *  Then wait for the child to finish with it.
		 */
		if (a[2] == NULL) {
			rmfree(coremap, rip->p_dsize, rip->p_daddr);
			rmfree(coremap, rip->p_ssize, rip->p_saddr);
		}
		rip->p_dsize = 0;
		rip->p_ssize = 0;
		rpp->p_flag |= P_SVFORK;
		rip->p_flag |= P_SVFPRNT;
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
	return(0);
}
