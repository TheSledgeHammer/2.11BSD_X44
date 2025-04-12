/*	$NetBSD: pthread_sig.c,v 1.34 2004/03/24 20:01:37 lha Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_sig.c,v 1.34 2004/03/24 20:01:37 lha Exp $");

#define	__PTHREAD_SIGNAL_PRIVATE

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>		/* for memcpy() */
#include <ucontext.h>
#include <unistd.h>

#include <sched.h>
#include "pthread.h"
#include "pthread_int.h"
#include "pthread_syscalls.h"

#ifdef PTHREAD_SIG_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

extern int pthread__started;

extern pthread_spin_t pthread__runqueue_lock;
extern struct pthread_queue_t pthread__runqueue;

extern pthread_spin_t pthread__allqueue_lock;
extern struct pthread_queue_t pthread__allqueue;

extern pthread_spin_t pthread__suspqueue_lock;
extern struct pthread_queue_t pthread__suspqueue;

static pthread_spin_t	pt_sigacts_lock;
static struct sigaction pt_sigacts[_NSIG];

static pthread_spin_t	pt_process_siglock;
static sigset_t	pt_process_sigmask;
static sigset_t	pt_process_siglist;

/* Queue of threads that are waiting in sigsuspend(). */
static struct pthread_queue_t pt_sigsuspended;
static pthread_spin_t pt_sigsuspended_lock;

/*
 * Nothing actually signals or waits on this lock, but the sleepobj
 * needs to point to something.
 */
static pthread_cond_t pt_sigsuspended_cond = PTHREAD_COND_INITIALIZER;

/* Queue of threads that are waiting in sigtimedwait(). */
static struct pthread_queue_t pt_sigwaiting;
static pthread_spin_t pt_sigwaiting_lock;
static pthread_t pt_sigwmaster;
static pthread_cond_t pt_sigwaiting_cond = PTHREAD_COND_INITIALIZER;

static int __sigsetequal(const sigset_t *, const sigset_t *);
static int __sigplusset(const sigset_t *, sigset_t *);
static int __sigminusset(const sigset_t *, sigset_t *);
static int __sigandset(const sigset_t *, sigset_t *);

static void pthread__make_siginfo(siginfo_t *, int);
static void pthread__kill(pthread_t, pthread_t, siginfo_t *);
static void pthread__kill_self(pthread_t, siginfo_t *);

static int firstsig(const sigset_t *);
static int pthread__sigmask(int, const sigset_t *, sigset_t *);

/* 
 * Some alias's may need to change from strong to weak.
 * Noteably the ones blanked out. 
 * Cause multiple definition errors during compliation.
 */
__weak_alias(pthread_sys_execve, pthread_execve)
__weak_alias(pthread_sys_sigaction, pthread_sigaction)
__weak_alias(pthread_sys_sigprocmask, pthread_sigmask)
__weak_alias(pthread_sys_sigsuspend, pthread_sigsuspend)
__weak_alias(pthread_sys_sigtimedwait, pthread_timedwait)

static int
__sigsetequal(const sigset_t *s1, const sigset_t *s2)
{
    	int equal;

    	equal = (*s1 == *s2) == 0;
    	return (equal);
}

static int
__sigplusset(const sigset_t *s, sigset_t *t)
{
	int plus;

	plus = (*t |= *s) == 0;
	return (plus);
}

static int
__sigminusset(const sigset_t *s, sigset_t *t)
{
	int minus;

    	minus = (*t &= ~*s) == 0;
    	return (minus);
}

static int
__sigandset(const sigset_t *s, sigset_t *t)
{
	int and;

	and = (*t &= *s) == 0;
    	return (and);
}

void
pthread__signal_init(void)
{
	SDPRINTF(("(signal_init) setting process sigmask\n"));
	pthread_sys_sigprocmask(0, NULL, &pt_process_sigmask);

	PTQ_INIT(&pt_sigsuspended);
	PTQ_INIT(&pt_sigwaiting);
}

static void
pthread__make_siginfo(siginfo_t *si, int sig)
{
	(void)memset(si, 0, sizeof(*si));
	si->si_signo = sig;
	si->si_code = SI_USER;
	/*
	 * XXX: these could be cached, but beware of setuid().
	 */
	si->si_uid = getuid();
	si->si_pid = getpid();
}

int
pthread_kill(pthread_t thread, int sig)
{
	pthread_t self;
	void (*handler)(int);

	self = pthread__self();

	SDPRINTF(("(pthread_kill %p) kill %p sig %d\n", self, thread, sig));

	if ((sig < 0) || (sig >= _NSIG))
		return EINVAL;
	if (pthread__find(self, thread) != 0)
		return ESRCH;

	/*
	 * We only let the thread handle this signal if the action for
	 * the signal is an explicit handler. Otherwise we feed it to
	 * the kernel so that it can affect the whole process.
	 */
	pthread_spinlock(self, &pt_sigacts_lock);
	handler = pt_sigacts[sig].sa_handler;
	pthread_spinunlock(self, &pt_sigacts_lock);

	if (handler == SIG_IGN) {
		SDPRINTF(("(pthread_kill %p) do nothing\n", self, thread, sig));
		/* Do nothing */
	} else if ((sig == SIGKILL) || (sig == SIGSTOP) ||
	    (handler == SIG_DFL)) {
		/* Let the kernel do the work */
		SDPRINTF(("(pthread_kill %p) kernel kill\n", self, thread, sig));
		kill(getpid(), sig);
	} else {
		siginfo_t si;
		pthread__make_siginfo(&si, sig);
		pthread_spinlock(self, &thread->pt_siglock);
		pthread__kill(self, thread, &si);
		pthread_spinunlock(self, &thread->pt_siglock);
	}

	return (0);
}

int
pthread_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	struct sigaction realact;
	sigset_t oldmask;
	pthread_t self;
	int retval;

	if ((sig <= 0) || (sig >= _NSIG))
		return EINVAL;

	self = pthread__self();
	pthread_spinlock(self, &pt_sigacts_lock);
	oldmask = pt_sigacts[sig].sa_mask;
	if (act != NULL) {
		/* Save the information for our internal dispatch. */
		pt_sigacts[sig] = *act;
		pthread_spinunlock(self, &pt_sigacts_lock);
		/*
		 * We want to handle all signals ourself, and not have
		 * the kernel mask them. Therefore, we clear the
		 * sa_mask field before passing this to the kernel. We
		 * do not set SA_NODEFER, which seems like it might be
		 * appropriate, because that would permit a continuous
		 * stream of signals to exhaust the supply of upcalls.
		 */
		if (pthread__started) {
			realact = *act;
			sigemptyset(&realact.sa_mask);
			act = &realact;
		}
	} else {
		pthread_spinunlock(self, &pt_sigacts_lock);
	}

	retval = pthread_sys_sigaction(sig, act, oact);
	if (oact && (retval == 0))
		oact->sa_mask = oldmask;

	return (retval);
}

int
pthread_sigsuspend(const sigset_t *sigmask)
{
	pthread_t self;
	sigset_t oldmask;

	/* if threading not started yet, just do the syscall */
	if (__predict_false(pthread__started == 0))
		return (pthread_sys_sigsuspend(sigmask));

	self = pthread__self();

	pthread_spinlock(self, &pt_sigsuspended_lock);
	pthread_spinlock(self, &self->pt_statelock);
	if (self->pt_cancel) {
		pthread_spinunlock(self, &self->pt_statelock);
		pthread_spinunlock(self, &pt_sigsuspended_lock);
		pthread_exit(PTHREAD_CANCELED);
	}
	pthread_sigmask(SIG_SETMASK, sigmask, &oldmask);

	self->pt_state = PT_STATE_BLOCKED_QUEUE;
	self->pt_sleepobj = &pt_sigsuspended_cond;
	self->pt_sleepq = &pt_sigsuspended;
	self->pt_sleeplock = &pt_sigsuspended_lock;
	pthread_spinunlock(self, &self->pt_statelock);

	PTQ_INSERT_TAIL(&pt_sigsuspended, self, pt_sleep);
	pthread__block(self, &pt_sigsuspended_lock);

	pthread__testcancel(self);

	pthread_sigmask(SIG_SETMASK, &oldmask, NULL);

	errno = EINTR;
	return -1;
}

static void
pthread_sigtimedwait__callback(void *arg)
{
	pthread__sched(pthread__self(), (pthread_t) arg);
}

int
pthread_timedwait(const sigset_t * __restrict set, siginfo_t * __restrict info, const struct timespec * __restrict timeout)
{
	pthread_t self;
	int error = 0;
	pthread_t target;
	sigset_t wset;
	struct timespec timo;

	/* if threading not started yet, just do the syscall */
	if (__predict_false(pthread__started == 0)) {
		return (pthread_sys_sigtimedwait(set, &info->si_signo, __UNCONST(timeout)));
	}

	self = pthread__self();
	pthread__testcancel(self);

	/* also call syscall if timeout is zero (i.e. polling) */
	if (timeout && timeout->tv_sec == 0 && timeout->tv_nsec == 0) {
		error = pthread_sys_sigtimedwait(set, &info->si_signo, __UNCONST(timeout));
		pthread__testcancel(self);
		return (error);
	}
	if (timeout) {
		if ((u_int) timeout->tv_nsec >= 1000000000) {
			return (EINVAL);
		}

		timo = *timeout;
	}

	pthread_spinlock(self, &pt_sigwaiting_lock);

	/*
	 * If there is already master thread running, arrange things
	 * to accomodate for eventual extra signals to wait for,
	 * and join the sigwaiting list.
	 */
	if (pt_sigwmaster) {
		struct pt_alarm_t timoalarm;
		struct timespec etimo;

		/*
		 * Get current time. We need it if we would become master.
		 */
		if (timeout) {
			pthread_sys_clock_gettime(CLOCK_MONOTONIC, &etimo);
			timespecadd(&etimo, timeout, &etimo);
		}

		/*
		 * Check if this thread's wait set is different to master set.
		 */
		wset = *set;

		__sigminusset(pt_sigwmaster->pt_sigwait, &wset);
		if (firstsig(&wset)) {
			/*
			 * Some new signal in set, wakeup master. It will
			 * rebuild its wait set.
			 */
			//_lwp_wakeup(pt_sigwmaster->pt_blockedp);
		}

		/* Save our wait set and info pointer */
		wset = *set;
		self->pt_sigwait = &wset;
		self->pt_wsig = info;

		/* zero to recognize when we get passed the signal from master */
		info->si_signo = 0;

		if (timeout) {
			pthread__alarm_add(self, &timoalarm, &etimo, pthread_sigtimedwait__callback, self);
		}

		block: pthread_spinlock(self, &self->pt_statelock);
		self->pt_state = PT_STATE_BLOCKED_QUEUE;
		self->pt_sleepobj = &pt_sigwaiting_cond;
		self->pt_sleepq = &pt_sigwaiting;
		self->pt_sleeplock = &pt_sigwaiting_lock;
		pthread_spinunlock(self, &self->pt_statelock);

		PTQ_INSERT_TAIL(&pt_sigwaiting, self, pt_sleep);

		pthread__block(self, &pt_sigwaiting_lock);

		/* check if we got a signal we waited for */
		if (info->si_signo) {
			/* got the signal from master */
			pthread__testcancel(self);
			return (0);
		}

		/* need the lock from now on */
		pthread_spinlock(self, &pt_sigwaiting_lock);

		/*
		 * If alarm fired, remove us from queue, adjust master
		 * wait set and return with EAGAIN.
		 */
		if (timeout) {
			if (pthread__alarm_fired(&timoalarm)) {
				PTQ_REMOVE(&pt_sigwaiting, self, pt_sleep);

				/*
				 * Signal master. It will rebuild it's wait set.
				 */
				//_lwp_wakeup(pt_sigwmaster->pt_blockedp);

				pthread_spinunlock(self, &pt_sigwaiting_lock);
				errno = EAGAIN;
				return (-1);
			}
			pthread__alarm_del(self, &timoalarm);
		}

		/*
		 * May have been woken up to deliver signal - check if we are
		 * the master and reblock if appropriate.
		 */
		if (pt_sigwmaster != self)
			goto block;

		/* not signal nor alarm, must have been upgraded to master */
		pthread__assert(pt_sigwmaster == self);

		/* update timeout before upgrading to master */
		if (timeout) {
			struct timespec tnow;

			pthread_sys_clock_gettime(CLOCK_MONOTONIC, &tnow);
			/* compute difference to end time */
			timespecsub(&tnow, &etimo, &tnow);
			/* substract the difference from timeout */
			timespecsub(&timo, &tnow, &timo);
		}
	}

	/* MASTER */
	self->pt_sigwait = &wset;
	self->pt_wsig = NULL;

	/* Master thread loop */
	pt_sigwmaster = self;
	for (;;) {
		/* Build our wait set */
		wset = *set;
		if (!PTQ_EMPTY(&pt_sigwaiting)) {
			PTQ_FOREACH(target, &pt_sigwaiting, pt_sleep) {
				__sigplusset(target->pt_sigwait, &wset);
			}
		}

		pthread_spinunlock(self, &pt_sigwaiting_lock);

		/*
		 * We are either the only one, or wait set was setup already.
		 * Just do the syscall now.
		 */
		error = pthread_sys_sigtimedwait(&wset, &info->si_signo, (timeout) ? &timo : NULL);

		pthread_spinlock(self, &pt_sigwaiting_lock);
		if ((error && (errno != ECANCELED || self->pt_cancel))
				|| (!error && sigismember(set, info->si_signo))) {
			/*
			 * Normal function return. Clear pt_sigwmaster,
			 * and if wait queue is nonempty, make first waiter
			 * new master.
			 */
			pt_sigwmaster = NULL;
			if (!PTQ_EMPTY(&pt_sigwaiting)) {
				pt_sigwmaster = PTQ_FIRST(&pt_sigwaiting);
				PTQ_REMOVE(&pt_sigwaiting, pt_sigwmaster, pt_sleep);
				pthread__sched(self, pt_sigwmaster);
			}

			pthread_spinunlock(self, &pt_sigwaiting_lock);

			pthread__testcancel(self);
			return (error);
		}

		if (!error) {
			/*
			 * Got a signal, but not from _our_ wait set.
			 * Scan the queue of sigwaiters and wakeup
			 * the first thread waiting for this signal.
			 */
			PTQ_FOREACH(target, &pt_sigwaiting, pt_sleep) {
				if (sigismember(target->pt_sigwait, info->si_signo)) {
					pthread__assert(target->pt_state == PT_STATE_BLOCKED_QUEUE);

					/* copy to waiter siginfo */
					memcpy(target->pt_wsig, info, sizeof(*info));
					PTQ_REMOVE(&pt_sigwaiting, target, pt_sleep);
					pthread__sched(self, target);
					break;
				}
			}

			if (!target) {
				/*
				 * Didn't find anyone waiting on this signal.
				 * Deliver signal normally. This might
				 * happen if a thread times out, but
				 * 'their' signal arrives before the master
				 * thread would be scheduled after _lwp_wakeup().
				 */
				pthread__signal(self, NULL, info);
			} else {
				/*
				 * Signal waiter removed, adjust our wait set.
				 */
				wset = *set;
				PTQ_FOREACH(target, &pt_sigwaiting, pt_sleep) {
					__sigplusset(target->pt_sigwait, &wset);
				}
			}
		} else {
			/*
			 * ECANCELED - new sigwaiter arrived and added to master
			 * wait set, or some sigwaiter exited due to timeout
			 * and removed from master wait set. All the work
			 * was done already, so just update our timeout
			 * and go back to syscall.
			 */
		}

		/* Timeout was adjusted by the syscall, just call again. */
	}

	/* NOTREACHED */
	return (0);
}

static int
firstsig(const sigset_t *ss)
{
	int sig;

	sig = ffs((int)ss);
	if (sig != 0) {
		return (sig);
	}

	return (0);
}

int
pthread__sigmask(int how, const sigset_t *set, sigset_t *oset)
{
	sigset_t tmp, takelist;
	pthread_t self;
	int i;
	siginfo_t si;

	self = pthread__self();

	if (oset != NULL) {
		*oset = self->pt_sigmask;
	}

	if (set == NULL) {
		return 0;
	}

	if (how == SIG_BLOCK) {
		__sigplusset(set, &self->pt_sigmask);
		/*
		 * Blocking of signals that are now
		 * blocked by all threads will be done
		 * lazily, at signal delivery time.
		 */
		if (__predict_true(pthread__started)) {
			pthread_spinunlock(self, &self->pt_siglock);
			return 0;
		}
	} else if (how == SIG_UNBLOCK) {
		__sigminusset(set, &self->pt_sigmask);
	} else if (how == SIG_SETMASK) {
		self->pt_sigmask = *set;
	} else {
		pthread_spinunlock(self, &self->pt_siglock);
		return EINVAL;
	}

	if (__predict_false(pthread__started == 0)) {
		pt_process_sigmask = self->pt_sigmask;
		pthread_sys_sigprocmask(SIG_SETMASK, &pt_process_sigmask, NULL);
		pthread_spinunlock(self, &self->pt_siglock);
		return 0;
	}

	/* See if there are any signals to take */
	sigemptyset(&takelist);
	tmp = self->pt_siglist;
	while ((i = firstsig(&tmp)) != 0) {
		if (!sigismember(&self->pt_sigmask, i)) {
			sigaddset(&takelist, i);
			sigdelset(&self->pt_siglist, i);
		}
		sigdelset(&tmp, i);
	}
	pthread_spinlock(self, &pt_process_siglock);
	tmp = pt_process_siglist;
	while ((i = firstsig(&tmp)) != 0) {
		if (!sigismember(&self->pt_sigmask, i)) {
			sigaddset(&takelist, i);
			sigdelset(&pt_process_siglist, i);
		}
		sigdelset(&tmp, i);
	}

	/* Unblock any signals that were blocked process-wide before this. */
	tmp = pt_process_sigmask;
	__sigandset(&self->pt_sigmask, &tmp);
	if (!__sigsetequal(&tmp, &pt_process_sigmask)) {
		pt_process_sigmask = tmp;
		pthread_sys_sigprocmask(SIG_SETMASK, &pt_process_sigmask, NULL);
	}
	pthread_spinunlock(self, &pt_process_siglock);

	pthread__make_siginfo(&si, 0);
	while ((i = firstsig(&takelist)) != 0) {
		/* Take the signal */
		SDPRINTF(("(pt_sigmask %p) taking unblocked signal %d\n", self, i));
		si.si_signo = i;
		pthread__kill_self(self, &si);
		sigdelset(&takelist, i);
	}

	pthread_spinunlock(self, &self->pt_siglock);

	return (0);
}

int
pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
	return (pthread__sigmask(how, set, oset));
}

/*
 * Dispatch a signal to thread t, if it is non-null, and to any
 * willing thread, if t is null.
 */
void
pthread__signal(pthread_t self, pthread_t t, siginfo_t *si)
{
	pthread_t target, good, okay;

	if (t) {
		target = t;
		pthread_spinlock(self, &target->pt_siglock);
	} else {
		/*
		 * Pick a thread that doesn't have the signal blocked
		 * and can be reasonably forced to run.
		 */
		okay = good = NULL;
		pthread_spinlock(self, &pthread__allqueue_lock);
		PTQ_FOREACH(target, &pthread__allqueue, pt_allq) {
			/*
			 * Changing to PT_STATE_ZOMBIE is protected by
			 * the pthread__allqueue lock, so we can just
			 * test for it here.
			 */
			if ((target->pt_state == PT_STATE_ZOMBIE) ||
			    (target->pt_type != PT_THREAD_NORMAL))
				continue;
			pthread_spinlock(self, &target->pt_siglock);
			SDPRINTF((
				"(pt_signal %p) target %p: state %d, mask %08x\n",
				self, target, target->pt_state, target->pt_sigmask.__bits[0]));
			if (!sigismember(&target->pt_sigmask, si->si_signo)) {
				if (target->pt_blockgen == target->pt_unblockgen) {
					good = target;
					/* Leave target locked */
					break;
				} else if (okay == NULL) {
					okay = target;
					/* Leave target locked */
					continue;
				}
			}
			pthread_spinunlock(self, &target->pt_siglock);
		}
		pthread_spinunlock(self, &pthread__allqueue_lock);
		if (good) {
			target = good;
			if (okay)
				pthread_spinunlock(self, &okay->pt_siglock);
		} else {
			target = okay;
		}

		if (target == NULL) {
			/*
			 * They all have it blocked. Note that in our
			 * process-wide signal mask, and stash the signal
			 * for later unblocking.
			 */
			pthread_spinlock(self, &pt_process_siglock);
			sigaddset(&pt_process_sigmask, si->si_signo);
			SDPRINTF(("(pt_signal %p) lazily setting proc sigmask to "
			    "%08x\n", self, pt_process_sigmask));
			pthread_sys_sigprocmask(SIG_SETMASK, &pt_process_sigmask, NULL);
			sigaddset(&pt_process_siglist, si->si_signo);
			pthread_spinunlock(self, &pt_process_siglock);
			return;
		}
	}

	/*
	 * We now have a signal and a victim.
	 * The victim's pt_siglock is locked.
	 */

	/*
	 * Reset the process signal mask so we can take another
	 * signal.  We will not exhaust our supply of upcalls; if
	 * another signal is delivered after this, the resolve_locks
	 * dance will permit us to finish and recycle before the next
	 * upcall reaches this point.
	 */
	pthread_spinlock(self, &pt_process_siglock);
	SDPRINTF(("(pt_signal %p) setting proc sigmask to "
	    "%08x\n", self, pt_process_sigmask));
	pthread_sys_sigprocmask(SIG_SETMASK, &pt_process_sigmask, NULL);
	pthread_spinunlock(self, &pt_process_siglock);

	pthread__kill(self, target, si);
	pthread_spinunlock(self, &target->pt_siglock);
}

/*
 * Call the signal handler in the context of this thread. Not quite as
 * suicidal as it sounds.
 * Must be called with target's siglock held.
 */
static void
pthread__kill_self(pthread_t self, siginfo_t *si)
{
	sigset_t oldmask;
	struct sigaction act;
	//ucontext_t uc;	/* XXX: we don't pass the right context here */

	pthread_spinlock(self, &pt_sigacts_lock);
	act = pt_sigacts[si->si_signo];
	pthread_spinunlock(self, &pt_sigacts_lock);

	SDPRINTF(("(pthread__kill_self %p) sig %d\n", self, si->si_signo));

	oldmask = self->pt_sigmask;
	__sigplusset(&self->pt_sigmask, &act.sa_mask);
	if ((act.sa_flags & SA_NODEFER) == 0) {
		sigaddset(&self->pt_sigmask, si->si_signo);
	}

	/*
	pthread_spinunlock(self, &self->pt_siglock);
	(*act.sa_sigaction)(si->si_signo, si, &uc);
	pthread_spinlock(self, &self->pt_siglock);
	*/

	self->pt_sigmask = oldmask;
}

/* Must be called with target's siglock held */
static void
pthread__kill(pthread_t self, pthread_t target, siginfo_t *si)
{
	SDPRINTF(("(pthread__kill %p) target %p sig %d code %d\n", self, target, si->si_signo, si->si_code));

	if (sigismember(&target->pt_sigmask, si->si_signo)) {
		/* Record the signal for later delivery. */
		sigaddset(&target->pt_siglist, si->si_signo);
		return;
	}

	if (self == target) {
		pthread__kill_self(self, si);
		return;
	}

	/*
	 * Ensure the victim is not running.
	 * In a MP world, it could be on another processor somewhere.
	 *
	 * XXX As long as this is uniprocessor, encountering a running
	 * target process is a bug.
	 */
	pthread__assert(target->pt_state != PT_STATE_RUNNING ||
		target->pt_blockgen != target->pt_unblockgen);

	/*
	 * Holding the state lock blocks out cancellation and any other
	 * attempts to set this thread up to take a signal.
	 */
	pthread_spinlock(self, &target->pt_statelock);
	if (target->pt_blockgen != target->pt_unblockgen) {
		/*
		 * The target is not on a queue at all, and
		 * won't run again for a while. Try to wake it
		 * from its torpor, then mark the signal for
		 * later processing.
		 */
		sigaddset(&target->pt_sigblocked, si->si_signo);
		sigaddset(&target->pt_sigmask, si->si_signo);
		pthread_spinlock(self, &target->pt_flaglock);
		target->pt_flags |= PT_FLAG_SIGDEFERRED;
		pthread_spinunlock(self, &target->pt_flaglock);
		pthread_spinunlock(self, &target->pt_statelock);
		//_lwp_wakeup(target->pt_blockedp);
		return;
	}
	switch (target->pt_state) {
	case PT_STATE_SUSPENDED:
		pthread_spinlock(self, &pthread__runqueue_lock);
		PTQ_REMOVE(&pthread__suspqueue, target, pt_runq);
		pthread_spinunlock(self, &pthread__runqueue_lock);
		break;
	case PT_STATE_RUNNABLE:
		pthread_spinlock(self, &pthread__runqueue_lock);
		PTQ_REMOVE(&pthread__runqueue, target, pt_runq);
		pthread_spinunlock(self, &pthread__runqueue_lock);
		break;
	case PT_STATE_BLOCKED_QUEUE:
		pthread_spinlock(self, target->pt_sleeplock);
		PTQ_REMOVE(target->pt_sleepq, target, pt_sleep);
		pthread_spinunlock(self, target->pt_sleeplock);
		break;
	default:
		;
	}

	pthread__sched(self, target);
	pthread_spinunlock(self, &target->pt_statelock);
}

/*
 * The execve() system call and the libc exec*() calls that use it are
 * specified to propagate the signal mask of the current thread to the
 * initial thread of the new process image. Since thread signal masks
 * are maintained in userlevel, this wrapper is necessary to give the
 * kernel the correct value.
 */
int
pthread_execve(const char *path, char *const argv[], char *const envp[])
{
	pthread_t self;
	int ret;

	self = pthread__self();

	/*
	 * Don't acquire pt_process_siglock, even though it seems like
	 * the right thing to do. The most common reason to be here is
	 * that we're on the child side of a fork() or vfork()
	 * call. In either case, another thread could have held
	 * pt_process_siglock at the moment of forking, and acquiring
	 * it here would cause us to deadlock. Additionally, in the
	 * case of vfork(), acquiring the lock here would cause it to
	 * be locked in the parent's address space and cause a
	 * deadlock there the next time a signal routine is called.
	 *
	 * The remaining case is where a live multithreaded program
	 * calls exec*() from one of several threads with no explicit
	 * synchronization. It may get the wrong process sigmask in
	 * the new process image if another thread executes a signal
	 * routine between the sigprocmask and the _sys_execve()
	 * call. I don't have much sympathy for such a program.
	 */
	pthread_sys_sigprocmask(SIG_SETMASK, &self->pt_sigmask, NULL);
	ret = pthread_sys_execve(path, argv, envp);

	/* Normally, we shouldn't get here; this is an error condition. */
	pthread_sys_sigprocmask(SIG_SETMASK, &pt_process_sigmask, NULL);

	return ret;
}
