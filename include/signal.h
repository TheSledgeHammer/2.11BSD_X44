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

#ifdef notyet
/*
 * Macro for converting signal number to a mask suitable for
 * sigblock().
 */
#define sigmask(m)	((long)1 << ((m)-1))

#ifndef KERNEL
extern long	sigblock(int), sigsetmask(int);
#endif
#endif

__BEGIN_DECLS
int		raise(int);
#ifndef	_ANSI_SOURCE
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
#ifndef _POSIX_SOURCE
int		killpg(pid_t, int);
int		sigblock(int);
int		siginterrupt(int, int);
int		sigpause(int);
int		sigreturn(struct sigcontext *);
int		sigsetmask(int);
int		sigstack(const struct sigstack *, struct sigstack *);
int		sigvec(int, struct sigvec *, struct sigvec *);
void	psignal(unsigned int, const char *);
#endif	/* !_POSIX_SOURCE */
#endif	/* !_ANSI_SOURCE */
__END_DECLS

#endif	/* !_SIGNAL_H_ */
