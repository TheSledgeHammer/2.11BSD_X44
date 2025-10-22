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
 *	@(#)kern_sig.c	8.14.2 (2.11BSD) 1999/9/9
 */

/*
 * This module is a hacked down version of kern_sig.c from 4.4BSD.  The
 * original signal handling code is still present in 2.11's kern_sig.c.  This
 * was done because large modules are very hard to fit into the kernel's
 * overlay structure.  A smaller kern_sig2.c fits more easily into an overlaid
 * kernel.
*/

#define	SIGPROP			/* include signal properties table */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/namei.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>		/* for coredump */
#include <sys/event.h>
#include <sys/eventvar.h>
#include <sys/sysdecl.h>

void    setsigvec(int, struct sigaction *);

int
sigaction()
{
	register struct sigaction_args {
		syscallarg(int *)sigtramp;
		syscallarg(int) signum;
		syscallarg(struct sigaction *)nsa;
		syscallarg(struct sigaction *)osa;
	} *uap = (struct sigaction_args *)u.u_ap;

	struct sigaction vec;
	register struct sigaction *sa;
	register int signum;
	u_long bit;
	int error = 0;

	//u.u_pcb.pcb_sigc = SCARG(uap, sigtramp);	/* save trampoline address */

	signum = SCARG(uap, signum);
	if (signum <= 0 || signum >= NSIG) {
		error = EINVAL;
		goto out;
	}
	if (SCARG(uap, nsa) && (signum == SIGKILL || signum == SIGSTOP)) {
		error = EINVAL;
		goto out;
	}
	sa = &vec;
	if (SCARG(uap, osa)) {
		sa->sa_handler = u.u_signal[signum];
		sa->sa_mask = u.u_sigmask[signum];
		bit = sigmask(signum);
		sa->sa_flags = 0;
		if ((u.u_sigonstack & bit) != 0)
			sa->sa_flags |= SA_ONSTACK;
		if ((u.u_sigintr & bit) == 0)
			sa->sa_flags |= SA_RESTART;
		if (u.u_procp->p_flag & P_NOCLDSTOP)
			sa->sa_flags |= SA_NOCLDSTOP;
		if ((error = copyout(sa, SCARG(uap, osa), sizeof(vec))) != 0)
			goto out;
	}
	if (SCARG(uap, nsa)) {
		if ((error = copyin(SCARG(uap, nsa), sa, sizeof(vec))) != 0)
			goto out;
		setsigvec(signum, sa);
	}
out:
	return (u.u_error = error);
}

void
setsigvec(signum, sa)
	int signum;
	register struct sigaction *sa;
{
	unsigned long bit;
	register struct proc *p;

	p = u.u_procp;
	bit = sigmask(signum);
	/*
	 * Change setting atomically.
	 */
	(void) splhigh();
	u.u_signal[signum] = sa->sa_handler;
	u.u_sigmask[signum] = sa->sa_mask &~ sigcantmask;
	if ((sa->sa_flags & SA_RESTART) == 0)
		u.u_sigintr |= bit;
	else
		u.u_sigintr &= ~bit;
	if (sa->sa_flags & SA_ONSTACK)
		u.u_sigonstack |= bit;
	else
		u.u_sigonstack &= ~bit;
	if (signum == SIGCHLD) {
		if (sa->sa_flags & SA_NOCLDSTOP)
			p->p_flag |= P_NOCLDSTOP;
		else
			p->p_flag &= ~P_NOCLDSTOP;
	}
	/*
	 * Set bit in p_sigignore for signals that are set to SIG_IGN,
	 * and for signals set to SIG_DFL where the default is to ignore.
	 * However, don't put SIGCONT in p_sigignore,
	 * as we have to restart the process.
	 */
	if (sa->sa_handler == SIG_IGN ||
	    ((sigprop[signum] & SA_IGNORE) && sa->sa_handler == SIG_DFL)) {
		p->p_sig &= ~bit;		/* never to be seen again */
		if (signum != SIGCONT)
			p->p_sigignore |= bit;	/* easier in psignal */
		p->p_sigcatch &= ~bit;
	} else {
		p->p_sigignore &= ~bit;
		if (sa->sa_handler == SIG_DFL)
			p->p_sigcatch &= ~bit;
		else
			p->p_sigcatch |= bit;
	}
	(void) spl0();
}

/*
 * Kill current process with the specified signal in an uncatchable manner;
 * used when process is too confused to continue, or we are unable to
 * reconstruct the process state safely.
 */
void
fatalsig(signum)
	int signum;
{
	unsigned long mask;
	register struct proc *p;

	p = u.u_procp;
	u.u_signal[signum] = SIG_DFL;
	mask = sigmask(signum);
	p->p_sigignore &= ~mask;
	p->p_sigcatch &= ~mask;
	p->p_sigmask &= ~mask;
	psignal(p, signum);
}

/*
 * Initialize signal state for process 0;
 * set to ignore signals that are ignored by default.
 */
void
siginit(p)
	register struct proc *p;
{
	register int i;

	for (i = 0; i < NSIG; i++)
		if ((sigprop[i] & SA_IGNORE) && i != SIGCONT)
			p->p_sigignore |= sigmask(i);
}

/*
 * Manipulate signal mask.
 * Unlike 4.4BSD we do not receive a pointer to the new and old mask areas and
 * do a copyin/copyout instead of storing indirectly thru a 'retval' parameter.
 * This is because we have to return both an error indication (which is 16 bits)
 * _AND_ the new mask (which is 32 bits).  Can't do both at the same time with
 * the 2BSD syscall return mechanism.
 */
int
sigprocmask()
{
	register struct sigprocmask_args {
		syscallarg(int) how;
		syscallarg(sigset_t *) set;
		syscallarg(sigset_t *) oset;
	} *uap = (struct sigprocmask_args *)u.u_ap;

	int error = 0;
	sigset_t oldmask, newmask;
	register struct proc *p;

	p = u.u_procp;
	oldmask = p->p_sigmask;
	if	(!SCARG(uap, set))	/* No new mask, go possibly return old mask */
		goto out;
	if	(error == copyin(SCARG(uap, set), &newmask, sizeof (newmask)))
		goto out;
	(void) splhigh();

	switch (SCARG(uap, how)) {
	case SIG_BLOCK:
		p->p_sigmask |= (newmask &~ sigcantmask);
		break;
	case SIG_UNBLOCK:
		p->p_sigmask &= ~newmask;
		break;
	case SIG_SETMASK:
		p->p_sigmask = newmask &~ sigcantmask;
		break;
	default:
		error = EINVAL;
		break;
	}
	(void) spl0();
out:
	if	(error == 0 && SCARG(uap, oset))
		error = copyout(&oldmask, SCARG(uap, oset), sizeof (oldmask));
	return (u.u_error = error);
}

/*
 * sigpending and sigsuspend use the standard calling sequence unlike 4.4 which
 * used a nonstandard (mask instead of pointer) calling convention.
*/

int
sigpending()
{
	register struct sigpending_args {
		syscallarg(struct sigset_t *) set;
	} *uap = (struct sigpending_args *) u.u_ap;

	register int error = 0;
	struct proc *p = u.u_procp;

	if (SCARG(uap, set))
		error = copyout((caddr_t) &p->p_sigacts, (caddr_t) SCARG(uap, set), sizeof(p->p_sigacts));
	else
		error = EINVAL;
	return (u.u_error = error);
}

/*
 * sigsuspend is supposed to always return EINTR so we ignore errors on the
 * copyin by assuming a mask of 0.
*/

int
sigsuspend()
{
	register struct sigsuspend_args {
		syscallarg(struct sigset_t *) set;
	} *uap = (struct sigsuspend_args *)u.u_ap;

	sigset_t nmask;
	struct proc *p = u.u_procp;
	int error;

	if (SCARG(uap, set) && (error = copyin(SCARG(uap, set), &nmask, sizeof(nmask))))
		nmask = 0;
	/*
	 * When returning from sigsuspend, we want the old mask to be restored
	 * after the signal handler has finished.  Thus, we save it here and set
	 * a flag to indicate this.
	 */
	u.u_oldmask = p->p_sigmask;
	u.u_psflags |= SAS_OLDMASK;
	p->p_sigmask = nmask & ~ sigcantmask;
	while (tsleep((caddr_t) &u, PPAUSE | PCATCH, "pause", 0) == 0)
		;
	/* always return EINTR rather than ERESTART */
	return (u.u_error = EINTR);
}

int
sigaltstack()
{
	register struct sigaltstack_args {
		syscallarg(struct sigaltstack *) nss;
		syscallarg(struct sigaltstack *) oss;
	} *uap = (struct sigaltstack_args *)u.u_ap;

	struct sigaltstack ss;
	int error = 0;

	if ((u.u_psflags & SAS_ALTSTACK) == 0)
		u.u_sigstk.ss_flags |= SA_DISABLE;
	if (SCARG(uap, oss) && (error = copyout((caddr_t) & u.u_sigstk, (caddr_t) SCARG(uap, oss), sizeof(struct sigaltstack))))
		goto out;
	if (SCARG(uap, nss) == 0)
		goto out;
	if ((error = copyin(SCARG(uap, nss), &ss, sizeof(ss))) != 0)
		goto out;
	if (ss.ss_flags & SA_DISABLE) {
		if (u.u_sigstk.ss_flags & SA_ONSTACK) {
			error = EINVAL;
			goto out;
		}
		u.u_psflags &= ~SAS_ALTSTACK;
		u.u_sigstk.ss_flags = ss.ss_flags;
		goto out;
	}
	if (ss.ss_size < MINSIGSTKSZ) {
		error = ENOMEM;
		goto out;
	}
	u.u_psflags |= SAS_ALTSTACK;
	u.u_sigstk = ss;
out:
	return(u.u_error = error);
}

int
sigwait()
{
	register struct sigwait_args {
		syscallarg(sigset_t *) set;
		syscallarg(int *) sig;
	} *uap = (struct sigwait_args *)u.u_ap;
	sigset_t wanted, sigsavail;
	register struct proc *p = u.u_procp;
	int signo, error;

	if (SCARG(uap, set) == 0 || SCARG(uap, sig) == 0) {
		error = EINVAL;
		goto out;
	}
	if ((error = copyin(SCARG(uap, set), &wanted, sizeof(sigset_t))))
		goto out;

	wanted |= sigcantmask;
	while ((sigsavail = (wanted & p->p_sig)) == 0)
		tsleep(&u.u_signal[0], PPAUSE | PCATCH, "sigwait", 0);

	if (sigsavail & sigcantmask) {
		error = EINTR;
		goto out;
	}

	signo = ffs(sigsavail);
	p->p_sig &= ~sigmask(signo);
	error = copyout(&signo, SCARG(uap, sig), sizeof(int));
out:
	return (u.u_error = error);
}

int
sigtimedwait()
{
	register struct sigtimedwait_args {
		syscallarg(sigset_t *) set;
		syscallarg(int *) sig;
		syscallarg(struct timespec *) timeout;
	} *uap = (struct sigtimedwait_args *)u.u_ap;
	sigset_t wanted, sigsavail;
	register struct proc *p = u.u_procp;
	struct timespec ts;
	struct timeval tv;
	int signo, error, s;
	int timo = 0;

	if (SCARG(uap, set) == 0 || SCARG(uap, sig) == 0) {
		error = EINVAL;
		goto out;
	}
	if ((error = copyin(SCARG(uap, set), &wanted, sizeof(sigset_t)))) {
		goto out;
	}

	if (SCARG(uap, timeout)) {
		long ms;

		if ((error = copyin(SCARG(uap, timeout), &ts, sizeof(ts)))) {
			goto out;
		}

		ms = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
		timo = mstohz(ms);
		if (timo == 0 && ts.tv_sec == 0 && ts.tv_nsec > 0) {
			timo = 1;
		}
		if (timo <= 0) {
			error = EAGAIN;
			goto out;
		}

		/*
		 * Remember current mono_time, it would be used in
		 * ECANCELED/ERESTART case.
		 */
		s = splclock();
		tv = mono_time;
		splx(s);
	}

	wanted |= sigcantmask;
	while ((sigsavail = (wanted & p->p_sig)) == 0) {
		error = tsleep(&u.u_signal[0], PPAUSE | PCATCH, "sigtimedwait", timo);
	}

	if (sigsavail & sigcantmask) {
		error = EINTR;
		goto out;
	}

	if (error) {
		if (timo && error) {
			struct timeval tvnow, tvtimo;
			int err;

			s = splclock();
			tvnow = mono_time;
			splx(s);

			TIMESPEC_TO_TIMEVAL(&tvtimo, &ts);

			/* compute how much time has passed since start */
			timersub(&tvnow, &tv, &tvnow);
			/* substract passed time from timeout */
			timersub(&tvtimo, &tvnow, &tvtimo);

			if (tvtimo.tv_sec < 0) {
				error = EAGAIN;
				goto out;
			}

			TIMEVAL_TO_TIMESPEC(&tvtimo, &ts);

			if ((err = copyout(&ts, SCARG(uap, timeout), sizeof(ts)))) {
				error = err;
				goto out;
			}
		}
		goto out;
	}

	signo = ffs(sigsavail);
	p->p_sig &= ~sigmask(signo);
	error = copyout(&signo, SCARG(uap, sig), sizeof(int));
out:
	return (u.u_error = error);
}

static int
filt_sigattach(kn)
	struct knote *kn;
{
	struct proc *p = curproc;

	kn->kn_obj = p;
	kn->kn_flags |= EV_CLEAR;	/* automatically set */

	SIMPLEQ_INSERT_HEAD(&p->p_klist, kn, kn_selnext);

	return 0;
}

static void
filt_sigdetach(kn)
	struct knote *kn;
{
	struct proc *p = kn->kn_obj;

	SIMPLEQ_REMOVE(&p->p_klist, kn, knote, kn_selnext);
}

/*
 * Signal knotes are shared with proc knotes, so we apply a mask to
 * the hint in order to differentiate them from process hints.  This
 * could be avoided by using a signal-specific knote list, but probably
 * isn't worth the trouble.
 */
static int
filt_signal(kn, hint)
	struct knote *kn;
	long hint;
{

	if (hint & NOTE_SIGNAL) {
		hint &= ~NOTE_SIGNAL;

		if (kn->kn_id == hint)
			kn->kn_data++;
	}
	return (kn->kn_data != 0);
}

const struct filterops sig_filtops = {
	.f_isfd = 0,
	.f_attach = filt_sigattach,
	.f_detach = filt_sigdetach,
	.f_event = filt_signal,
};

/*
 * Old system calls that were specific to 2.11BSD, which
 * have potential future use cases.
 */
#ifdef notyet
/*
 * fetch the word at iaddr from user I-space.  This system call is
 * required on machines with separate I/D space because the mfpi
 * instruction reads from D-space if the current and previous modes
 * in the program status word are both user.
 */
int
fetchi()
{
	register struct fetchi_args {
		syscallarg(caddr_t) iaddr;
	} *uap = (struct fetchi_args *)u.u_ap;

	u.u_error = EINVAL;
/*
#ifdef NONSEPARATE
	u.u_error = EINVAL;

#else !NONSEPARATE
	u.u_error = copyin(SCARG(uap, iaddr), u.u_r.r_val1, NBPW);
#endif NONSEPARATE
*/
	return (u.u_error);
}

/*
 * return the floating point error registers as they were following
 * the last floating point exception generated by the calling process.
 * Required because the error registers may have been changed so the
 * system saves them at the time of the exception.
 */
int
fperr()
{
	register struct fperr_args {
		syscallarg(int) 	fec;
		syscallarg(caddr_t) fea;
	}*uap = (struct fperr_args *)u.u_ap;
	register struct fperr *fpe;

	fpe = &u.u_fperr;
	if(fpe->f_fec) {
		bcopy(fpe->f_fec, SCARG(uap, fec), sizeof(fpe->f_fec));
		u.u_r.r_val1 = (int)SCARG(uap, fec);
	}
	if(fpe->f_fea) {
		bcopy(fpe->f_fea, SCARG(uap, fea), sizeof(fpe->f_fea));
		u.u_r.r_val2 = (int)SCARG(uap, fea);
	}

	return (0);
}

/*
 * ucall allows user level code to call various kernel functions.
 * Autoconfig uses it to call the probe and attach routines of the
 * various device drivers.
 * New revision makes use of Soft Interrupts.
 */
int
ucall()
{
	typedef int (*routine_t)(int, int);
	register struct ucall_args {
		syscallarg(routine_t) 	routine;
		syscallarg(int) 		priority;
		syscallarg(int) 		arg1;
		syscallarg(int) 		arg2;
	} *uap = (struct ucall_args *)u.u_ap;
	int s;
	routine_t routine;

	if (suser()) {
		switch (SCARG(uap, priority)) {
		case IPL_SOFTCLOCK:
			s = splsoftclock();
			break;
		case IPL_SOFTSERIAL:
			s = splsoftserial();
			break;
		case IPL_SOFTNET:
			s = splsoftnet();
			break;
		case IPL_SOFTBIO:
			s = splsoftbio();
		}
	}
	routine = (*SCARG(uap, routine))(SCARG(uap, arg1), SCARG(uap, arg2));
	u.u_error = copyout(routine, SCARG(uap, routine), sizeof(routine));
	splx(s);
	return (u.u_error);
}
#endif
