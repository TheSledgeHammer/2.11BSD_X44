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
 *	@(#)signal.h	8.4 (Berkeley) 5/4/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)signal.h	1.2 (2.11BSD) 1997/8/28
 */

#ifndef	_SYS_SIGNAL_H_
#define _SYS_SIGNAL_H_

#include <machine/signal.h>
#define _NSIG		32

#ifndef	NSIG
#define NSIG		_NSIG

#define	SIGHUP		1		/* hangup */
#define	SIGINT		2		/* interrupt */
#define	SIGQUIT		3		/* quit */
#define	SIGILL		4		/* illegal instruction (not reset when caught) */
#define	ILL_RESAD_FAULT		0x0	/* reserved addressing fault */
//#define	ILL_PRIVIN_FAULT	0x1	/* privileged instruction fault */
//#define	ILL_RESOP_FAULT		0x2	/* reserved operand fault */
/* CHME, CHMS, CHMU are not yet given back to users reasonably */
#define	SIGTRAP		5		/* trace trap (not reset when caught) */
#define	SIGIOT		6		/* IOT instruction */
#define	SIGABRT		SIGIOT	/* compatibility */
#define	SIGEMT		7		/* EMT instruction */
#define	SIGFPE		8		/* floating point exception */
#define	SIGKILL		9		/* kill (cannot be caught or ignored) */
#define	SIGBUS		10		/* bus error */
#define	SIGSEGV		11		/* segmentation violation */
#define	SIGSYS		12		/* bad argument to system call */
#define	SIGPIPE		13		/* write on a pipe with no one to read it */
#define	SIGALRM		14		/* alarm clock */
#define	SIGTERM		15		/* software termination signal from kill */
#define	SIGURG		16		/* urgent condition on IO channel */
#define	SIGSTOP		17		/* sendable stop signal not from tty */
#define	SIGTSTP		18		/* stop signal from tty */
#define	SIGCONT		19		/* continue a stopped process */
#define	SIGCHLD		20		/* to parent on child stop or exit */
#define	SIGCLD		SIGCHLD	/* compatibility */
#define	SIGTTIN		21		/* to readers pgrp upon background tty read */
#define	SIGTTOU		22		/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	SIGIO		23		/* input/output possible signal */
#define	SIGXCPU		24		/* exceeded CPU time limit */
#define	SIGXFSZ		25		/* exceeded file size limit */
#define	SIGVTALRM 	26		/* virtual time alarm */
#define	SIGPROF		27		/* profiling time alarm */
#define SIGWINCH 	28		/* window size changes */
#define	SIGINFO		29		/* information request */
#define SIGUSR1 	30		/* user defined signal 1 */
#define SIGUSR2 	31		/* user defined signal 2 */
#define SIGTHD		32		/* to parent on thread stop or exit */

#define	SIG_DFL		((void (*) (int))  0)
#define	SIG_ERR		((void (*) (int)) -1)
#define	SIG_IGN		((void (*) (int))  1)
#define	SIG_CATCH	((void (*) (int))  2)
#define	SIG_HOLD	((void (*) (int))  3)

typedef unsigned long 	sigset_t;

/*
 * Signal vector "template" used in sigaction call.
 */
struct sigaction {
	void			(*sa_handler)(int);	/* signal handler */
	sigset_t 		sa_mask;			/* signal mask to apply */
	int				sa_flags;			/* see signal options below */
};

#define SA_ONSTACK		0x0001	/* take signal on signal stack */
#define SA_RESTART		0x0002	/* restart system on signal return */
#define	SA_DISABLE		0x0004	/* disable taking signals on alternate stack */
#define SA_NOCLDSTOP	0x0008	/* do not generate SIGCHLD on child stop */
#define SA_NODEFER		0x0010	/* don't mask the signal we're delivering */
#define SA_NOCLDWAIT    0x0020	/* do not generate zombies on unwaited child */
#define SA_SIGINFO		0x0040	/* take sa_sigaction handler */

/*
 * Flags for sigprocmask:
 */
#define	SIG_BLOCK	1		            /* block specified signal set */
#define	SIG_UNBLOCK	2		            /* unblock specified signal set */
#define	SIG_SETMASK	3		            /* set specified signal set */

typedef	void (*sig_t)(int);		        /* type of signal function */

/*
 * Structure used in sigaltstack call.
 */
typedef struct sigaltstack stack_t;
struct	sigaltstack {
	void			*ss_base;		    /* signal stack base */
#define ss_sp		ss_base
	size_t			ss_size;		    /* signal stack length */
	int				ss_flags;		    /* SA_DISABLE and/or SA_ONSTACK */
};
#define	MINSIGSTKSZ	8192					/* minimum allowable stack */
#define	SIGSTKSZ	(MINSIGSTKSZ + 32768)	/* recommended stack size */

/*
 * 4.3 compatibility:
 * Signal vector "template" used in sigvec call.
 */
struct sigvec {
	int				(*sv_handler)(int);	/* signal handler */
	long 			sv_mask;			/* signal mask to apply */
	int				sv_flags;			/* see signal options below */
};
#define SV_ONSTACK		SA_ONSTACK	/* take signal on signal stack */
#define SV_INTERRUPT	SA_RESTART	/* same bit, opposite sense */
#define sv_onstack 		sv_flags	/* isn't compatibility wonderful! */

/*
 * 4.3 compatibility:
 * Structure used in sigstack call.
 */
struct	sigstack {
	char			*ss_sp;			/* signal stack pointer */
	int				ss_onstack;		/* current status */
};
#define SS_ONSTACK	0x0001	        /* take signals on alternate stack */
#define SS_DISABLE	0x0004	        /* disable taking signals on alternate stack */

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to properly restore state if
 * a non-standard exit is performed.
 */
 
struct osigcontext {
	int			sc_onstack;		/* sigstack state to restore */
	long 		sc_mask;		/* signal mask to restore */
	int			sc_sp;			/* sp to restore */
	int			sc_fp;			/* fp to restore */
	int			sc_r1;			/* r1 to restore */
	int			sc_r0;			/* r0 to restore */
	int			sc_pc;			/* pc to restore */
	int			sc_ps;			/* psl to restore */
	int			sc_ovno;		/* overlay to restore */
};

/*
 * Macro for converting signal number to a mask suitable for
 * sigblock().
 */
#define sigmask(m)				(1L << ((m)-1))

#ifdef _KERNEL
#define sigaddset(set, signo)	(*(set) |= 1L << ((signo) - 1), 0)
#define sigdelset(set, signo)	(*(set) &= ~(1L << ((signo) - 1)), 0)
#define sigemptyset(set)	(*(set) = (sigset_t)0, (int)0)
#define sigfillset(set)         (*(set) = ~(sigset_t)0, (int)0)
#define sigismember(set, signo) ((*(set) & (1L << ((signo) - 1))) != 0)

extern long	sigblock(int);
extern long	sigsetmask(int);
#else
#include <sys/cdefs.h>

struct sigcontext;

__BEGIN_DECLS
void	(*signal(int, void (*)(int)))(int);
#define	BADSIG		SIG_ERR
__END_DECLS
#endif /* !_KERNEL */
#endif /* NSIG */
#endif /* _SYS_SIGNAL_H_ */
