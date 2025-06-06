/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)system.c	5.10.1 (2.11BSD) 1999/10/24";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <paths.h>

int
system(command)
	const char *command;
{
	union wait pstat;
	register int pid;
	sigset_t omask, nmask;
	sig_t intsave, quitsave;

	if (!command)		/* just checking... */
		return(1);

	sigemptyset(&nmask);
	sigaddset(&nmask, SIGCHLD);
	(void) sigprocmask(SIG_BLOCK, &nmask, &omask);
	switch (pid = vfork()) {
	case -1:			/* error */
		(void) sigprocmask(SIG_SETMASK, &omask, NULL);
		pstat.w_status = 0;
		pstat.w_retcode = 127;
		return (pstat.w_status);
	case 0:				/* child */
		(void) sigprocmask(SIG_SETMASK, &omask, NULL);
		execl("/bin/sh", "sh", "-c", command, (char*) NULL);
		_exit(127);
	}
	intsave = signal(SIGINT, SIG_IGN);
	quitsave = signal(SIGQUIT, SIG_IGN);
	pid = waitpid(pid, (int*) &pstat, 0);
	(void) sigprocmask(SIG_SETMASK, &omask, NULL);
	(void) signal(SIGINT, intsave);
	(void) signal(SIGQUIT, quitsave);
	return (pid == -1 ? -1 : pstat.w_status);
}
