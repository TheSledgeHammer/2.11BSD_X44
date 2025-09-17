/*
 * Copyright (c) 1983, 1988, 1993
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
static char sccsid[] = "@(#)syslog.c	8.4.3 (2.11BSD) 1995/07/15";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <netdb.h>

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <stdarg.h>

#ifdef __weak_alias
__weak_alias(syslog,_syslog)
__weak_alias(vsyslog,_vsyslog)
__weak_alias(closelog,_closelog)
__weak_alias(openlog,_openlog)
__weak_alias(setlogmask,_setlogmask)
#endif

#if defined(pdp11)
static char ToFile = 0;		/* set if logfile is used */
static char logfile[] = "/usr/adm/messages";
#endif

static int	LogFile = -1;		/* fd for log */
static char connected;		/* have done connect */
static int	LogStat = 0;		/* status bits, set by openlog() */
static const char *LogTag = NULL;		/* string to tag the entry with */
static int	LogFacility = LOG_USER;	/* default facility code */
static int	LogMask = 0xff;		/* mask of priorities to be logged */

extern	char	*__progname;		/* Program name, from crt0. */

/*
 * syslog, vsyslog --
 *	print message on log file; output is intended for syslogd(8).
 */
void
syslog(int pri, const char *fmt, ...)
{
    va_list ap;

	va_start(ap, fmt);
	vsyslog(pri, fmt, ap);
	va_end(ap);
}

void
vsyslog(int pri, const char *fmt, va_list ap)
{
	register int cnt;
	register char ch, *p, *t;
	time_t now;
	int fd, saved_errno;
	char tbuf[2048], fmt_cpy[1024];
    char *stdp = NULL;
	pid_t	pid = 0;

#define	INTERNALLOG	LOG_ERR|LOG_CONS|LOG_PERROR|LOG_PID
	/* Check for invalid bits. */
	if (pri & ~(LOG_PRIMASK|LOG_FACMASK)) {
		syslog(INTERNALLOG,
		    "syslog: bad fac/pri: %x", pri);
		pri &= LOG_PRIMASK|LOG_FACMASK;
	}

	/* Check priority against setlogmask values. */
	if (!(LOG_MASK(LOG_PRI(pri)) & LogMask)) {
		return;
    }

	saved_errno = errno;

	/* Set default facility if none specified. */
	if ((pri & LOG_FACMASK) == 0)
		pri |= LogFacility;

	/* Build the message. */
	(void)time(&now);
	p = tbuf + sprintf(tbuf, "<%d>", pri);
	p += strftime(p, sizeof (tbuf) - (p - tbuf), "%h %e %T ",
	    localtime(&now));
	if (LogStat & LOG_PERROR)
		stdp = p;
	if (LogTag == NULL)
		LogTag = __progname;
	if (LogTag != NULL)
		p += sprintf(p, "%s", LogTag);
	if (LogStat & LOG_PID)
		p += sprintf(p, "[%d]", getpid());
	if (LogTag != NULL) {
		*p++ = ':';
		*p++ = ' ';
	}

	/* Substitute error message for %m. */
	for (t = fmt_cpy; (ch = *fmt); ++fmt)
		if (ch == '%' && fmt[1] == 'm') {
			++fmt;
			t += sprintf(t, "%s", strerror(saved_errno));
		} else
			*t++ = ch;
	*t = '\0';

	p += vsprintf(p, fmt_cpy, ap);
	cnt = p - tbuf;

	/* Output to stderr if requested. */
	if (LogStat & LOG_PERROR) {
		struct iovec iov[2];
		register struct iovec *v = iov;

		v->iov_base = stdp;
		v->iov_len = cnt - (stdp - tbuf);
		++v;
		v->iov_base = __UNCONST("\n");
		v->iov_len = 1;
		(void)writev(STDERR_FILENO, iov, 2);
	}

	/* Get connected, output the message to the local logger. */
	if (!connected)
		openlog(LogTag, LogStat | LOG_NDELAY, 0);
#if defined(pdp11)
	if (ToFile) {
		if (write(LogFile, tbuf, cnt) == cnt)
			return;
	} else
#endif
	if (send(LogFile, tbuf, cnt, 0) >= 0)
		return;

	/*
	 * Output the message to the console; don't worry about blocking,
	 * if console blocks everything will.  Make sure the error reported
	 * is the one from the syslogd failure.
	 *
	 * 2.11BSD has to do a more complicated dance because we do not
	 * want to acquire a controlling terminal (bad news for 'init'!).
	 * Until either the tty driver is ported from 4.4 or O_NOCTTY
	 * is implemented we have to fork and let the child do the open of
	 * the console.
	 */
	if (LogStat & LOG_CONS) {
		pid = vfork();
		if (pid == -1)
			return;
		if (pid == 0) {
	   		fd = open(_PATH_CONSOLE, O_WRONLY, 0);
			(void)strcat(tbuf, "\r\n");
			cnt += 2;
			p = index(tbuf, '>') + 1;
			(void)write(fd, p, cnt - (p - tbuf));
			(void)close(fd);
			_exit(0);
		}
		while (waitpid(pid, NULL, 0) == -1 && (errno == EINTR));
	}
}

static struct sockaddr SyslogAddr;	/* AF_UNIX address of local logger */

void
openlog(ident, logstat, logfac)
	const char *ident;
	int logstat;
	register int logfac;
{
	if (ident != NULL)
		LogTag = ident;
	LogStat = logstat;
	if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0)
		LogFacility = logfac;

	if (LogFile == -1) {
		SyslogAddr.sa_family = AF_UNIX;
		(void)strncpy(SyslogAddr.sa_data, _PATH_LOG,
		    sizeof(SyslogAddr.sa_data));
		if (LogStat & LOG_NDELAY) {
			LogFile = socket(AF_UNIX, SOCK_DGRAM, 0);
#if defined(pdp11)
			if (LogFile == -1) {
				LogFile = open(logfile, O_WRONLY|O_APPEND);
				ToFile = 1;
				connected = 1;
			} else
				ToFile = 0;
#endif
			if (LogFile == -1)
				return;
			(void)fcntl(LogFile, F_SETFD, 1);
		}
	}
	if ((LogFile != -1) && !connected) {
		if (connect(LogFile, &SyslogAddr, sizeof(SyslogAddr)) == -1) {
			(void)close(LogFile);
			LogFile = -1;
		} else {
			connected = 1;
        }
    }
}

void
closelog(void)
{
	(void)close(LogFile);
	LogFile = -1;
	connected = 0;
}

/* setlogmask -- set the log mask level */
int
setlogmask(pmask)
	register int pmask;
{
	register int omask;

	omask = LogMask;
	if (pmask != 0)
		LogMask = pmask;
	return (omask);
}
