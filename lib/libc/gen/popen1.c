/*	$NetBSD: popen.c,v 1.38 2022/04/19 20:32:15 rillig Exp $	*/

/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software written by Ken Arnold and
 * published in UNIX Review, Vol. 6, No. 8.
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
static char sccsid[] = "@(#)popen.c	5.15.1 (2.11BSD) 1999/10/24";
static char sccsid[] = "@(#)popen.c	8.3 (Berkeley) 5/3/95";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/param.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>

#include "reentrant.h"

#ifdef __weak_alias
__weak_alias(popen,_popen)
__weak_alias(pclose,_pclose)
#endif

static struct pid {
	struct pid *next;
	FILE *fp;
#ifdef _REENTRANT
	int fd;
#endif
	pid_t pid;
} *pidlist;

#ifdef _REENTRANT
static mutex_t  pidlist_mutex = MUTEX_INITIALIZER;
# define MUTEX_LOCK() \
    do { \
	    if (__isthreaded) \
		    mutex_lock(&pidlist_mutex); \
    } while (0)
# define MUTEX_UNLOCK() \
    do { \
	    if (__isthreaded) \
		    mutex_unlock(&pidlist_mutex); \
    } while (0)
#else
# define MUTEX_LOCK() __nothing
# define MUTEX_UNLOCK() __nothing
#endif

FILE *
popen(program, type)
	const char *program;
	const char *type;
{
	register FILE *iop;
	struct pid *cur, *old;
	int pdes[2], twoway;
	pid_t pid;

	if (strchr(type, '+')) {
		twoway = 1;
		type = "r+";
		if (socketpair(AF_UNIX, SOCK_STREAM, 0, pdes) < 0) {
			return (NULL);
		}
	} else {
		twoway = 0;
		if ((*type != 'r' && *type != 'w')
			|| (type[1] && (type[1] != 'e' || type[2]))) {
			errno = EINVAL;
			return (NULL);
		}
	}

	if ((cur = malloc(sizeof(struct pid))) == NULL) {
		errno = ENOMEM;
		return (NULL);
	}

	if (pipe2(pdes, O_CLOEXEC) < 0) {
		free(cur);
		return (NULL);
	}

	MUTEX_LOCK();
	switch (pid = vfork()) {
	case -1:			/* error */
		(void)close(pdes[0]);
		(void)close(pdes[1]);
		MUTEX_UNLOCK();
		free(cur);
		return (NULL);
		/* NOTREACHED */
	case 0:				/* child */
		for (old = pidlist; old; old = old->next) {
#ifdef _REENTRANT
			close(old->fd); /* don't allow a flush */
#else
			close(fileno(old->fp)); /* don't allow a flush */
#endif
		}
		if (*type == 'r') {
			if (pdes[1] != STDOUT_FILENO) {
				(void)dup2(pdes[1], STDOUT_FILENO);
				(void)close(pdes[1]);
			}
			(void)close(pdes[0]);
			if (twoway && (pdes[1] != STDIN_FILENO)) {
				(void)dup2(pdes[1], STDIN_FILENO);
			}
		} else {
			if (pdes[0] != STDIN_FILENO) {
				(void)dup2(pdes[0], STDIN_FILENO);
				(void)close(pdes[0]);
			}
			(void)close(pdes[1]);
		}
		execl(_PATH_BSHELL, "sh", "-c", program, (char *)NULL);
		_exit(127);
		/* NOTREACHED */
	}

	/* parent; assume fdopen can't fail...  */
	if (*type == 'r') {
		iop = fdopen(pdes[0], type);
#ifdef _REENTRANT
		cur->fd = pdes[0];
#endif
		(void)close(pdes[1]);
	} else {
		iop = fdopen(pdes[1], type);
#ifdef _REENTRANT
		cur->fd = pdes[1];
#endif
		(void)close(pdes[0]);
	}

	/* Link into list of file descriptors. */
	cur->fp = iop;
	cur->pid =  pid;
	cur->next = pidlist;
	pidlist = cur;
	MUTEX_UNLOCK();
	return (iop);
}

int
pclose(iop)
	FILE *iop;
{
	register struct pid *cur, *last;
	//sigset_t omask, nmask;
	int pstat;
	pid_t pid;

	/*
	 * pclose returns -1 if stream is not associated with a
	 * `popened' command, if already `pclosed', or waitpid
	 * returns an error.
	 */

	MUTEX_LOCK();

	/* Find the appropriate file pointer. */
	for (last = NULL, cur = pidlist; cur; last = cur, cur = cur->next) {
		if (cur->fp == iop) {
			break;
		}
	}
	if (cur == NULL) {
		MUTEX_UNLOCK();
		return (-1);
	}

	(void)fclose(iop);

	/* Remove the entry from the linked list. */
	if (last == NULL) {
		pidlist = cur->next;
	} else {
		last->next = cur->next;
	}

	MUTEX_UNLOCK();

	do {
		pid = waitpid(cur->pid, &pstat, 0);
	} while (pid == -1 && errno == EINTR);
	free(cur);
	return (pid == -1 ? -1 : pstat);
}
