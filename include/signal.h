/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)signal.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/signal.h>

#if defined(__BSD_VISIBLE)
extern const char *sys_signame[];
#ifndef __SYS_SIGLIST_DECLARED
#define __SYS_SIGLIST_DECLARED
/* also in unistd.h */
extern const char *sys_siglist[];
#endif /* __SYS_SIGLIST_DECLARED */
extern const int sys_nsigname;
extern const int sys_nsiglist;
#endif /* __BSD_VISIBLE */

#if defined(__BSD_VISIBLE)
/* 2.11BSD Compatability: No siginfo in kernel */
#ifndef __SIGINFO_DECLARED
#define __SIGINFO_DECLARED
#include <sys/siginfo.h>
#endif /* __SIGINFO_DECLARED */
#endif /* __BSD_VISIBLE */

__BEGIN_DECLS
int		raise(int);
#ifdef	__BSD_VISIBLE
int		kill(pid_t, int);
int		sigaction(int, const struct sigaction *, struct sigaction *);
int		sigaddset(sigset_t *, int);
int		sigdelset(sigset_t *, int);
int		sigemptyset(sigset_t *);
int		sigfillset(sigset_t *);
int		sigismember(const sigset_t *, int);
int		sigpending(sigset_t *);
int		sigprocmask(int, const sigset_t *, sigset_t *);
int		sigsuspend(const sigset_t *);
#endif	/* __BSD_VISIBLE */
#if (defined(_XOPEN_SOURCE) && defined(_XOPEN_SOURCE_EXTENDED)) || \
    (_XOPEN_SOURCE - 0) >= 500 || (_POSIX_C_SOURCE - 0) >= 200809L || \
    defined(__BSD_VISIBLE)
int		killpg(pid_t, int);
int		sigblock(int);
int		siginterrupt(int, int);
int		sigpause(int);
int		sigreturn(struct sigcontext *);
int		sigsetmask(int);
int		sigstack(const struct sigstack *, struct sigstack *);
int		sigvec(int, struct sigvec *, struct sigvec *);
void	psignal(unsigned int, const char *);
#endif /* _XOPEN_SOURCE_EXTENDED || _XOPEN_SOURCE >= 500
	   || _POSIX_C_SOURCE >= 200809L || __BSD_VISIBLE */
__END_DECLS

#endif	/* !_SIGNAL_H_ */
