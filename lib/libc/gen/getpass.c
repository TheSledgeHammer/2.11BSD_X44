/*
 * Copyright (c) 1988, 1993
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getpass.c	5.2 (Berkeley) 3/9/86";
static char sccsid[] = "@(#)getpass.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>
#include <sys/signal.h>

#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(RUN_SGTTY) && (RUN_SGTTY == 0)
#include <sgtty.h>
#else
#include <sys/termios.h>
#endif

#ifdef __weak_alias
__weak_alias(getpass,_getpass)
#endif

static void getpass_common(FILE *, FILE *, char *, const char *);

#if defined(RUN_SGTTY) && (RUN_SGTTY == 0)
static void getpass_sgtty(struct sgttyb *, FILE *, FILE *, long, char *, const char *);
#else
static void getpass_termios(struct termios *, FILE *, FILE *, long, char *, const char *);
#endif

char *
getpass(prompt)
	const char *prompt;
{
#if defined(RUN_SGTTY) && (RUN_SGTTY == 0)
    struct sgttyb ttyb;
#else
	struct termios term;
#endif
    long omask;
	FILE *fi, *fo;
	static char pbuf[_PASSWORD_LEN + 1];

    
    if ((fo = fi = fdopen(open(_PATH_TTY, O_RDWR), "r")) == NULL) {
        fo = stderr;
        fi = stdin;
    } else {
        setbuf(fo, (char *)NULL);
        setbuf(fi, (char *)NULL);
    }
    omask = sigblock(sigmask(SIGINT)|sigmask(SIGTSTP));
#if defined(RUN_SGTTY) && (RUN_SGTTY == 0)
    getpass_sgtty(&ttyb, fi, fo, omask, pbuf, prompt);
#else
    getpass_termios(&term, fi, fo, omask, pbuf, prompt);
#endif
    (void)sigsetmask(omask);
	if (fi != stdin) {
		fclose(fi);
    }
	return (pbuf);
}

static void
getpass_common(fi, fo, pbuf, prompt)
    FILE *fi, *fo;
    char *pbuf;
    const char *prompt;
{
    register char *p;
    register int c;

    (void)fputs(prompt, fo);
    rewind(fo);			/* implied flush */
	for (p = pbuf; (c = getc(fi)) != '\n' && c != EOF;) {
		if (p < &pbuf[_PASSWORD_LEN]) {
			*p++ = c;
        }
	}
    *p = '\0';
    (void)write(fileno(fo), "\n", 1);
}

#if defined(RUN_SGTTY) && (RUN_SGTTY == 0)

static void
getpass_sgtty(ttyb, fi, fo, omask, pbuf, prompt)
    struct sgttyb *ttyb;
    FILE *fi, *fo;
    long omask;
    char *pbuf;
    const char *prompt;
{
    int flags;

    ioctl(fileno(fi), TIOCGETP, ttyb);
    if ((flags = (ttyb->sg_flags & ECHO))) {
        ttyb->sg_flags &= ~ECHO;
        ioctl(fileno(fi), TIOCSETP, ttyb);
    }
    getpass_common(fi, fo, pbuf, prompt);
    if (flags) {
        ttyb->sg_flags |= ECHO;
        ioctl(fileno(fi), TIOCSETP, ttyb);
    }
}

#else

static void
getpass_termios(term, fi, fo, omask, pbuf, prompt)
    struct termios *term;
    FILE *fi, *fo;
    long omask;
    char *pbuf;
    const char *prompt;
{
    int flags;

    (void)tcgetattr(fileno(fi), term);
    if ((flags = (term->c_lflag & ECHO))) {
        term->c_lflag &= ~ECHO;
		(void)tcsetattr(fileno(fi), TCSAFLUSH|TCSASOFT, term);
    }
    getpass_common(fi, fo, pbuf, prompt);
    if (flags) {
        term->c_lflag |= ECHO;
		(void)tcsetattr(fileno(fi), TCSAFLUSH|TCSASOFT, term);
    }
}

#endif
