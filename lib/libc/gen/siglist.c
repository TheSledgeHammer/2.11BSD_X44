 /*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)siglist.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <signal.h>

#ifdef __weak_alias
__weak_alias(sys_signame, _sys_signame)
__weak_alias(sys_siglist, _sys_siglist)
__weak_alias(sys_nsigname, _sys_nsigname)
__weak_alias(sys_nsiglist, _sys_nsiglist)
#endif

const char *sys_signame[NSIG] = {
		"Signal 0",
		"hup",						/* SIGHUP */
		"int",						/* SIGINT */
		"quit",						/* SIGQUIT */
		"ill",						/* SIGILL */
		"trap",						/* SIGTRAP */
		"abrt",						/* SIGABRT */
		"emt",						/* SIGEMT */
		"fpe",						/* SIGFPE */
		"kill",						/* SIGKILL */
		"bus",						/* SIGBUS */
		"segv",						/* SIGSEGV */
		"sys",						/* SIGSYS */
		"pipe",						/* SIGPIPE */
		"alrm",						/* SIGALRM */
		"term",						/* SIGTERM */
		"urg",						/* SIGURG */
		"stop",						/* SIGSTOP */
		"tstp",						/* SIGTSTP */
		"cont",						/* SIGCONT */
		"chld",						/* SIGCHLD */
		"ttin",						/* SIGTTIN */
		"ttou",						/* SIGTTOU */
		"io",						/* SIGIO */
		"xcpu",						/* SIGXCPU */
		"xfsz",						/* SIGXFSZ */
		"vtalrm",					/* SIGVTALRM */
		"prof",						/* SIGPROF */
		"winch",					/* SIGWINCH */
		"info",						/* SIGINFO */
		"usr1",						/* SIGUSR1 */
		"usr2",						/* SIGUSR2 */
};

const char *sys_siglist[NSIG] = {
		"Signal 0",
		"Hangup",					/* SIGHUP */
		"Interrupt",				/* SIGINT */
		"Quit",						/* SIGQUIT */
		"Illegal instruction",		/* SIGILL */
		"Trace/BPT trap",			/* SIGTRAP */
		"IOT trap",					/* SIGIOT */
		"EMT trap",					/* SIGEMT */
		"Floating point exception",	/* SIGFPE */
		"Killed",					/* SIGKILL */
		"Bus error",				/* SIGBUS */
		"Segmentation fault",		/* SIGSEGV */
		"Bad system call",			/* SIGSYS */
		"Broken pipe",				/* SIGPIPE */
		"Alarm clock",				/* SIGALRM */
		"Terminated",				/* SIGTERM */
		"Urgent I/O condition",		/* SIGURG */
		"Stopped (signal)",			/* SIGSTOP */
		"Stopped",					/* SIGTSTP */
		"Continued",				/* SIGCONT */
		"Child exited",				/* SIGCHLD */
		"Stopped (tty input)",		/* SIGTTIN */
		"Stopped (tty output)",		/* SIGTTOU */
		"I/O possible",				/* SIGIO */
		"Cputime limit exceeded",	/* SIGXCPU */
		"Filesize limit exceeded",	/* SIGXFSZ */
		"Virtual timer expired",	/* SIGVTALRM */
		"Profiling timer expired",	/* SIGPROF */
		"Window size changes",		/* SIGWINCH */
		"Information request",		/* SIGINFO */
		"User defined signal 1",	/* SIGUSR1 */
		"User defined signal 2"		/* SIGUSR2 */
};

const int sys_nsigname = (sizeof(sys_signame) / sizeof(sys_signame[0]));
const int sys_nsiglist = (sizeof(sys_siglist) / sizeof(sys_siglist[0]));
