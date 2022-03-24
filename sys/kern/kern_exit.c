/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_exit.c	2.6 (2.11BSD) 2000/2/20
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/kernel.h>

#include <vm/include/vm.h>

/*
 * exit system call: pass back caller's arg
 */
int
rexit()
{
	register struct exit_args {
		syscallarg(int)	rval;
	} *uap = (struct exit_args *)u.u_ap;

	exit(W_EXITCODE(SCARG(uap, rval), 0));
	/* NOTREACHED */
}

/*
 * Exit: deallocate address space and other resources,
 * change proc state to zombie, and unlink proc from allproc
 * list.  Save exit status and rusage for wait4().
 * Check for child processes and orphan them.
 */
void
exit(rv)
	int rv;
{
	register int i;
	register struct proc *p;
	struct proc 		**pp;
	register struct vmspace *vm;

	/*
	 * If parent is waiting for us to exit or exec,
	 * P_PPWAIT is set; we will wakeup the parent below.
	 */
	p = u.u_procp;
	p->p_flag &= ~(P_TRACED | P_PPWAIT | P_SULOCK);
	p->p_sigignore = ~0;
	p->p_sigacts = 0;
	untimeout(realitexpire, (caddr_t)p);

	if (p->p_pid == 1) {
		panic("init died (signal %d, exit %d)", WTERMSIG(rv), WEXITSTATUS(rv));
	}

	if (p->p_flag & P_PROFIL) {
		stopprofclock(p);
	}

	fdfree(u.u_fd);
	/*
	 * 2.11 doesn't need to do this and it gets overwritten anyway.
	 * p->p_realtimer.it_value = 0;
	 */
	for (i = 0; i <= u.u_lastfile; i++) {
		register struct file *f;

		f = u.u_ofile[i];
		u.u_ofile[i] = NULL;
		u.u_pofile[i] = 0;
		(void)closef(f);
	}
	vm = p->p_vmspace;
	if (vm->vm_refcnt == 1) {
		(void) vm_map_remove(&vm->vm_map, VM_MIN_ADDRESS, VM_MAXUSER_ADDRESS);
	}

	if(SESS_LEADER(p)) {
		register struct session *sp = p->p_session;
		if (sp->s_ttyvp) {
			if (sp->s_ttyp->t_session == sp) {
				if (sp->s_ttyp->t_pgrp)
					pgsignal(sp->s_ttyp->t_pgrp, SIGHUP, 1);
				(void) ttywait(sp->s_ttyp);

				if (sp->s_ttyvp)
					vgoneall(sp->s_ttyvp);
			}
			if (sp->s_ttyvp)
				vrele(sp->s_ttyvp);
			sp->s_ttyvp = NULL;
		}
		sp->s_leader = NULL;
	}

	fixjobc(p, p->p_pgrp, 0);
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	(void)acct_process(p);
#ifdef KTRACE
	/*
	 * release trace file
	 */
	p->p_traceflag = 0;	/* don't trace the vrele() */
	if (p->p_tracep)
		vrele(p->p_tracep);
#endif

	/*
	 * Freeing the user structure and kernel stack
	 * for the current process: have to run a bit longer
	 * using the slots which are about to be freed...
	 */
	if (p->p_flag & P_SVFORK)
		endvfork();
	else {
		rmfree(coremap, p->p_dsize, p->p_daddr);
		rmfree(coremap, p->p_ssize, p->p_saddr);
	}
	rmfree(coremap, USIZE, p->p_addr);

	if (p->p_pid == 1) {
		panic("init died");
	}
	LIST_REMOVE(p, p_list); 				/* off allproc queue */
	LIST_INSERT_HEAD(&zombproc, p, p_list);	/* onto zombproc */
	p->p_stat = SZOMB;

	noproc = 1;
	for (pp = PIDHASH(p->p_pid); *pp; pp = LIST_NEXT(*pp, p_hash)) {
		if (*pp == p) {
			*pp = p->p_hash;
			goto done;
		}
	}
	panic("exit");

done:
	/*
 	 * Overwrite p_alive substructure of proc - better not be anything
 	 * important left!
 	 */
	LIST_REMOVE(p, p_hash);
	p->p_xstat = rv;
	p->p_ru = u.u_ru;
	calcru(p, &p->p_ru->ru_utime, &p->p_ru->ru_stime, NULL);
	ruadd(&p->p_ru, &u.u_cru);
	{
		register struct proc *q, *nq;
		int doingzomb = 0;

		q = LIST_FIRST(p->p_children);
again:
		if (q) {								/* only need this if any child is S_ZOMB */
			wakeup((caddr_t) initproc);
		}
		for ( ; q != NULL; q = nq) {
			nq = LIST_NEXT(q, p_sibling);
			if (q->p_pptr == p) {
				LIST_REMOVE(q, p_sibling);
				LIST_INSERT_HEAD(&initproc->p_children, q, p_sibling);
				q->p_pptr = initproc;
				q->p_ppid = 1;
				wakeup((caddr_t)initproc);
				if (q->p_flag & P_TRACED) {
					q->p_flag &= ~P_TRACED;
					psignal(q, SIGKILL);
				} else if (q->p_stat == SSTOP) {
					psignal(q, SIGHUP);
					psignal(q, SIGCONT);
				}
			}
		}
		if (!doingzomb) {
			doingzomb = 1;
			q = LIST_FIRST(&zombproc);
			goto again;
		}
	}

	psignal(p->p_pptr, SIGCHLD);
	wakeup((caddr_t)p->p_pptr);

	curproc = NULL;
	if (--p->p_limit->p_refcnt == 0) {
		FREE(p->p_limit, M_SUBPROC);
	}

	//swtch();
	cpu_exit(p);
	/* NOTREACHED */
}

struct wait_args {
	syscallarg(int)	pid;
	syscallarg(int *) status;
	syscallarg(int) options;
	syscallarg(struct rusage *) rusage;
	syscallarg(int) compat;
};

int
wait4()
{
	register struct wait_args *uap = (struct wait_args *)u.u_ap;

	int retval[2];

	SCARG(uap, compat) = 0;
	u.u_error = wait1(u.u_procp, uap, retval);
	if (!u.u_error) {
		u.u_r.r_val1 = retval[0];
	}
	return (u.u_error);
}

/*
 * Wait: check child processes to see if any have exited,
 * stopped under trace or (optionally) stopped by a signal.
 * Pass back status and make available for reuse the exited
 * child's proc structure.
 */
static int
wait1(q, uap, retval)
	struct proc *q;
	register struct wait_args *uap;
	int retval[];
{
	int nfound, status;
	struct rusage ru;			/* used for local conversion */
	register struct proc *p, *t;
	register int error;

	if (SCARG(uap, pid) == WAIT_MYPGRP)		/* == 0 */
		SCARG(uap, pid) = -q->p_pgrp;
loop:
	nfound = 0;
	/*
	 * 4.X has child links in the proc structure, so they consolidate
	 * these two tests into one loop.  We only have the zombie chain
	 * and the allproc chain, so we check for ZOMBIES first, then for
	 * children that have changed state.  We check for ZOMBIES first
	 * because they are more common, and, as the list is typically small,
	 * a faster check.
	 */
	for (p = LIST_FIRST(&zombproc); p; p = LIST_NEXT(p, p_sibling)) {
		if (p->p_pptr != q) {
			continue;
		}
		if (SCARG(uap, pid) != WAIT_ANY && p->p_pid != SCARG(uap, pid)
				&& p->p_pgrp != -SCARG(uap, pid)) {
			continue;
		}
		retval[0] = p->p_pid;
		retval[1] = p->p_xstat;
		if (SCARG(uap, status)
				&& (error = copyout(&p->p_xstat, SCARG(uap, status),
						sizeof(SCARG(uap, status))))) {
			return (error);
		}
		if (SCARG(uap, rusage)) {
			rucvt(&ru, &p->p_ru);
			if (error == copyout(&ru, SCARG(uap, rusage), sizeof(ru))) {
				return (error);
			}
		}

		if (p->p_oppid && (t = pfind(p->p_oppid))) {
			p->p_oppid = 0;
			proc_reparent(p, t);
			psignal(t, SIGCHLD);
			wakeup((caddr_t)t);
			return (0);
		}

		ruadd(&u->u_cru, &p->p_ru);
		(void) chgproccnt(p->p_cred->p_ruid, -1);
		FREE(p->p_ru, M_ZOMBIE);

		p->p_xstat = 0;
		p->p_stat = NULL;
		p->p_pid = 0;
		p->p_ppid = 0;

		/* Free up credentials. */
		if (--p->p_cred->p_refcnt == 0) {
			crfree(p->p_cred->pc_ucred);
			FREE(p->p_cred, M_SUBPROC);
		}

		/* Release reference to text vnode */
		if (p->p_textvp)
			vrele(p->p_textvp);

		leavepgrp(p);
		LIST_REMOVE(p, p_list);					/* off zombproc */
		LIST_REMOVE(p, p_sibling);
		//LIST_INSERT_HEAD(&freeproc, p, p_list);/* onto freeproc */
		p->p_pptr = 0;
		p->p_sigacts = 0;
		p->p_sigcatch = 0;
		p->p_sigignore = 0;
		p->p_sigmask = 0;
		p->p_pgrp = 0;
		p->p_flag = 0;
		p->p_wchan = 0;

		cpu_wait(p);
		FREE(p, M_PROC);
		return (0);
	}

	for (p = LIST_FIRST(&allproc); p; p = LIST_NEXT(p, p_sibling)) {
		if (p->p_pptr != q) {
			continue;
		}
		if (SCARG(uap, pid) != WAIT_ANY && p->p_pid != SCARG(uap, pid)
				&& p->p_pgrp != -SCARG(uap, pid)) {
			continue;
		}
		++nfound;
		if ((p->p_stat == SSTOP || p->p_stat == SZOMB)
				&& (p->p_flag & P_WAITED) == 0
				&& ((p->p_flag & P_TRACED) || (SCARG(uap, options) & WUNTRACED))) {
			p->p_flag |= P_WAITED;
			if (curproc->p_pid != 1) {
				curproc->p_estcpu = min(curproc->p_estcpu + p->p_estcpu,
				UCHAR_MAX);
			}
			retval[0] = p->p_pid;
			error = 0;
			if (SCARG(uap, compat))
				retval[1] = W_STOPCODE(p->p_ptracesig);
			else if (SCARG(uap, status)) {
				status = W_STOPCODE(p->p_ptracesig);
				error = copyout(&status, SCARG(uap, status), sizeof(status));
			}
			return (error);
		}
	}
	if (nfound == 0)
		return (ECHILD);
	if (SCARG(uap, options) & WNOHANG) {
		retval[0] = 0;
		return (0);
	}
	error = tsleep(q, PWAIT | PCATCH, "wait", 0);
	if (error == 0)
		goto loop;
	return (error);
}

/*
 * Notify parent that vfork child is finished with parent's data.  Called
 * during exit/exec(getxfile); must be called before xfree().  The child
 * must be locked in core so it will be in core when the parent runs.
 */
void
endvfork()
{
	register struct proc *rip, *rpp;

	rpp = u.u_procp;
	rip = rpp->p_pptr;
	rpp->p_flag &= ~P_SVFORK;
	rpp->p_flag |= P_SLOCK;
	wakeup((caddr_t)rpp);
	while(!(rpp->p_flag&P_SVFDONE))
		sleep((caddr_t)rip, PZERO-1);
	/*
	 * The parent has taken back our data+stack, set our sizes to 0.
	 */
	u.u_dsize = rpp->p_dsize = 0;
	u.u_ssize = rpp->p_ssize = 0;
	rpp->p_flag &= ~(P_SVFDONE | P_SLOCK);
}

/* make process 'parent' the new parent of process 'child'. */
void
proc_reparent(child, parent)
	register struct proc *child;
	register struct proc *parent;
{
	if (child->p_pptr == parent) {
		return;
	}

	LIST_REMOVE(child, p_sibling);
	LIST_INSERT_HEAD(&parent->p_children, child, p_sibling);
	child->p_pptr = parent;
}
