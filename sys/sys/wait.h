/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)wait.h	7.2.1 (2.11BSD GTE) 1995/06/23
 */

/*
 * This file holds definitions relevent to the wait system call.
 * Some of the options here are available only through the ``wait3''
 * entry point; the old entry point with one argument has more fixed
 * semantics, never returning status of unstopped children, hanging until
 * a process terminates if any are outstanding, and never returns
 * detailed information about process resource utilization (<vtimes.h>).
 */
#ifndef _SYS_WAIT_H_
#define _SYS_WAIT_H_

/*
 * Structure of the information in the first word returned by both
 * wait and wait3.  If w_stopval==WSTOPPED, then the second structure
 * describes the information returned, else the first.  See WUNTRACED below.
 */
union wait	{
	int	w_status;		/* used in syscall */
	/*
	 * Terminated process status.
	 */
	struct {
		unsigned short	w_Termsig:7;	/* termination signal */
		unsigned short	w_Coredump:1;	/* core dump indicator */
		unsigned short	w_Retcode:8;	/* exit code if w_termsig==0 */
	} w_T;
	/*
	 * Stopped process status.  Returned
	 * only for traced children unless requested
	 * with the WUNTRACED option bit.
	 */
	struct {
		unsigned short	w_Stopval:8;	/* == W_STOPPED if stopped */
		unsigned short	w_Stopsig:8;	/* signal that stopped us */
	} w_S;
};
#define	w_termsig	w_T.w_Termsig
#define w_coredump	w_T.w_Coredump
#define w_retcode	w_T.w_Retcode
#define w_stopval	w_S.w_Stopval
#define w_stopsig	w_S.w_Stopsig

#define	WSTOPPED	0177				/* value of s.stopval if process is stopped */
#define	WCOREFLAG	0200

/*
 * Option bits for the second argument of wait3.  WNOHANG causes the
 * wait to not hang if there are no stopped or terminated processes, rather
 * returning an error indication in this case (pid==0).  WUNTRACED
 * indicates that the caller should receive status about untraced children
 * which stop due to signals.  If children are stopped and a wait without
 * this option is done, it is as though they were still running... nothing
 * about them is returned.
 */
#define WNOHANG		1	/* dont hang in wait */
#define WUNTRACED	2	/* tell about stopped, untraced children */

#ifdef _KERNEL

#define WIFSTOPPED(x)		((x).w_stopval == WSTOPPED)
#define WIFSIGNALED(x)		((x).w_stopval != WSTOPPED && (x).w_termsig != 0)
#define WTERMSIG(x)		((x).w_stopval)
#define WIFEXITED(x)		((x).w_stopval != WSTOPPED && (x).w_termsig == 0)
#define	WEXITSTATUS(x)		((x).w_retcode)

#else /* !_KERNEL */

/* Userspace Wait Signals Conversion */
#define	_W_INT(w)	        (*(int *)&(w))	/* convert union wait to int */
#define	_WSTATUS(x)	        (_W_INT(x) & 0177)
#define	_WSTOPPED           	WSTOPPED
#define	_WCOREFLAG          	WCOREFLAG

#define WIFSTOPPED(x)	    	(_WSTATUS(x) == _WSTOPPED)
#define WSTOPSIG(x)	        (_W_INT(x) >> 8)
#define WIFSIGNALED(x)	    	(_WSTATUS(x) != _WSTOPPED && _WSTATUS(x) != 0)
#define WTERMSIG(x)	        (_WSTATUS(x))
#define WIFEXITED(x)	    	(_WSTATUS(x) == 0)
#define WEXITSTATUS(x)	    	(_W_INT(x) >> 8)

#define WCOREDUMP(x)	    	(_W_INT(x) & _WCOREFLAG)

#endif

#define	W_STOPCODE(sig)		((sig << 8) | WSTOPPED)
#define	W_EXITCODE(ret,sig)	((ret << 8) | (sig))

#define	WAIT_ANY		(-1)
#define	WAIT_MYPGRP		0

#ifndef _KERNEL
#include <sys/types.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

struct rusage;	/* forward declaration */

pid_t	wait(int *);
pid_t	waitpid(pid_t, int *, int);
#ifdef __BSD_VISIBLE
pid_t	wait3(int *, int, struct rusage *);
pid_t	wait4(pid_t, int *, int, struct rusage *);
#endif /* __BSD_VISIBLE */

__END_DECLS
#endif

#endif
