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
#ifndef	NSIG
#define NSIG				32

#define	SIGHUP				1	/* hangup */
#define	SIGINT				2	/* interrupt */
#define	SIGQUIT				3	/* quit */
#define	SIGILL				4	/* illegal instruction (not reset when caught) */
#define	ILL_RESAD_FAULT		0x0	/* reserved addressing fault */
#define	ILL_PRIVIN_FAULT	0x1	/* privileged instruction fault */
#define	ILL_RESOP_FAULT		0x2	/* reserved operand fault */
/* CHME, CHMS, CHMU are not yet given back to users reasonably */
#define	SIGTRAP				5	/* trace trap (not reset when caught) */
#define	SIGIOT				6	/* IOT instruction */
#define	SIGABRT				SIGIOT	/* compatibility */
#define	SIGEMT				7	/* EMT instruction */
#define	SIGFPE				8	/* floating point exception */
#define	FPE_INTOVF_TRAP		0x1	/* integer overflow */
#define	FPE_INTDIV_TRAP		0x2	/* integer divide by zero */
#define	FPE_FLTOVF_TRAP		0x3	/* floating overflow */
#define	FPE_FLTDIV_TRAP		0x4	/* floating/decimal divide by zero */
#define	FPE_FLTUND_TRAP		0x5	/* floating underflow */
#define	FPE_DECOVF_TRAP		0x6	/* decimal overflow */
#define	FPE_SUBRNG_TRAP		0x7	/* subscript out of range */
#define	FPE_FLTOVF_FAULT	0x8	/* floating overflow fault */
#define	FPE_FLTDIV_FAULT	0x9	/* divide by zero floating fault */
#define	FPE_FLTUND_FAULT	0xa	/* floating underflow fault */
#ifdef pdp11
#define FPE_CRAZY			0xb	/* illegal return code - FPU crazy */
#define FPE_OPCODE_TRAP		0xc	/* bad floating point op code */
#define FPE_OPERAND_TRAP	0xd	/* bad floating point operand */
#define FPE_MAINT_TRAP		0xe	/* maintenance trap */
#endif
#define	SIGKILL				9	/* kill (cannot be caught or ignored) */
#define	SIGBUS				10	/* bus error */
#define	SIGSEGV				11	/* segmentation violation */
#define	SIGSYS				12	/* bad argument to system call */
#define	SIGPIPE				13	/* write on a pipe with no one to read it */
#define	SIGALRM				14	/* alarm clock */
#define	SIGTERM				15	/* software termination signal from kill */
#define	SIGURG				16	/* urgent condition on IO channel */
#define	SIGSTOP				17	/* sendable stop signal not from tty */
#define	SIGTSTP				18	/* stop signal from tty */
#define	SIGCONT				19	/* continue a stopped process */
#define	SIGCHLD				20	/* to parent on child stop or exit */
#define	SIGCLD				SIGCHLD	/* compatibility */
#define	SIGTTIN				21	/* to readers pgrp upon background tty read */
#define	SIGTTOU				22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	SIGIO				23	/* input/output possible signal */
#define	SIGXCPU				24	/* exceeded CPU time limit */
#define	SIGXFSZ				25	/* exceeded file size limit */
#define	SIGVTALRM 			26	/* virtual time alarm */
#define	SIGPROF				27	/* profiling time alarm */
#define SIGWINCH 			28	/* window size changes */
#define SIGUSR1 			30	/* user defined signal 1 */
#define SIGUSR2 			31	/* user defined signal 2 */


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
int	raise __P((int));
#ifndef	_ANSI_SOURCE
int	kill __P((pid_t, int));
int	sigaction __P((int, const struct sigaction *, struct sigaction *));
int	sigaddset __P((sigset_t *, int));
int	sigdelset __P((sigset_t *, int));
int	sigemptyset __P((sigset_t *));
int	sigfillset __P((sigset_t *));
int	sigismember __P((const sigset_t *, int));
int	sigpending __P((sigset_t *));
int	sigprocmask __P((int, const sigset_t *, sigset_t *));
int	sigsuspend __P((const sigset_t *));
#ifndef _POSIX_SOURCE
int	killpg __P((pid_t, int));
int	sigblock __P((int));
int	siginterrupt __P((int, int));
int	sigpause __P((int));
int	sigreturn __P((struct sigcontext *));
int	sigsetmask __P((int));
int	sigstack __P((const struct sigstack *, struct sigstack *));
int	sigvec __P((int, struct sigvec *, struct sigvec *));
void	psignal __P((unsigned int, const char *));
#endif	/* !_POSIX_SOURCE */
#endif	/* !_ANSI_SOURCE */
__END_DECLS

#endif	/* !_SIGNAL_H_ */
