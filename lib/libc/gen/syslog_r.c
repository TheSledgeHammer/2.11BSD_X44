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

#include "reentrant.h"

#ifdef __weak_alias
__weak_alias(syslog,_syslog)
__weak_alias(vsyslog,_vsyslog)
__weak_alias(closelog,_closelog)
__weak_alias(openlog,_openlog)
__weak_alias(setlogmask,_setlogmask)
#endif

#if defined(pdp11)
static char logfile[] = "/usr/adm/messages";
#endif

void syslog_r(int, struct syslog_data *, const char *, ...);
void vsyslog_r(int, struct syslog_data *, const char *, va_list);
void openlog_r(const char *, int, int, struct syslog_data *);
void closelog_r(struct syslog_data *);
int setlogmask_r(int, struct syslog_data *);

extern	char	*__progname;	/* Program name, from crt0. */

struct syslog_data {
#if defined(pdp11)
	char ToFile;			/* set if logfile is used */
#endif
	int LogFile;			/* fd for log */
	char connected;			/* have done connect */
	int	LogStat;			/* status bits, set by openlog() */
	const char *LogTag;		/* string to tag the entry with */
	int LogFacility;		/* default facility code */
	int LogMask;			/* mask of priorities to be logged */
};

#if defined(pdp11)

#define SYSLOG_DATA_INIT 			\
{ 									\
	.ToFile = 0,					\
	.LogFile = -1, 					\
	.connected = 0, 				\
	.LogStat = 0, 					\
	.LogTag = (const char *)0, 		\
	.LogFacility = LOG_USER, 		\
	.LogMask = 0xff 				\
}

#else

#define SYSLOG_DATA_INIT 			\
{ 									\
	.LogFile = -1, 					\
	.connected = 0, 				\
	.LogStat = 0, 					\
	.LogTag = (const char *)0, 		\
	.LogFacility = LOG_USER, 		\
	.LogMask = 0xff 				\
}

#endif

struct syslog_data _syslog_data = SYSLOG_DATA_INIT;

#ifdef _REENTRANT
static mutex_t	syslog_mutex = MUTEX_INITIALIZER;
#endif

/*
 * syslog, vsyslog --
 *	print message on log file; output is intended for syslogd(8).
 */
void
syslog(int pri, const char *fmt, ...)
{
    va_list ap;

	va_start(ap, fmt);
	vsyslog_r(pri, &_syslog_data, fmt, ap);
	va_end(ap);
}

void
vsyslog(int pri, const char *fmt, va_list ap)
{
	vsyslog_r(pri, &_syslog_data, fmt, ap);
}

void
openlog(const char *ident, int logstat, int logfac)
{
	openlog_r(ident, logstat, logfac, &_syslog_data);
}

void
closelog(void)
{
	closelog_r(&_syslog_data);
}

/* setlogmask -- set the log mask level */
int
setlogmask(int pmask)
{
	return (setlogmask_r(pmask, &_syslog_data));
}

void
syslog_r(int pri, struct syslog_data *data, const char *fmt, ...)
{
    va_list ap;

	va_start(ap, fmt);
	vsyslog_r(pri, data, fmt, ap);
	va_end(ap);
}

void
vsyslog_r(int pri, struct syslog_data *data, const char *fmt, va_list ap)
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
	if (!(LOG_MASK(LOG_PRI(pri)) & data->LogMask)) {
		return;
    }

	saved_errno = errno;

	/* Set default facility if none specified. */
	if ((pri & LOG_FACMASK) == 0)
		pri |= data->LogFacility;

	/* Build the message. */
	(void)time(&now);
	p = tbuf + sprintf(tbuf, "<%d>", pri);
	p += strftime(p, sizeof (tbuf) - (p - tbuf), "%h %e %T ",
	    localtime(&now));
	if (data->LogStat & LOG_PERROR)
		stdp = p;
	if (data->LogTag == NULL)
		data->LogTag = __progname;
	if (data->LogTag != NULL)
		p += sprintf(p, "%s", data->LogTag);
	if (data->LogStat & LOG_PID)
		p += sprintf(p, "[%d]", getpid());
	if (data->LogTag != NULL) {
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
	if (data->LogStat & LOG_PERROR) {
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
	mutex_lock(&syslog_mutex);
	if (!data->connected)
		openlog_r(data->LogTag, data->LogStat | LOG_NDELAY, 0, data);
#if defined(pdp11)
	if (data->ToFile) {
		if (write(data->LogFile, tbuf, cnt) == cnt)
			mutex_unlock(&syslog_mutex);
			return;
	} else
#endif
	if (send(data->LogFile, tbuf, cnt, 0) >= 0)
		mutex_unlock(&syslog_mutex);
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
	if (data->LogStat & LOG_CONS) {
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
openlog_r(const char *ident, int logstat, int logfac, struct syslog_data *data)
{
	mutex_lock(&syslog_mutex);
	if (ident != NULL)
		data->LogTag = ident;
	data->LogStat = logstat;
	if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0)
		data->LogFacility = logfac;

	if (data->LogFile == -1) {
		SyslogAddr.sa_family = AF_UNIX;
		(void)strncpy(SyslogAddr.sa_data, _PATH_LOG,
		    sizeof(SyslogAddr.sa_data));
		if (data->LogStat & LOG_NDELAY) {
			data->LogFile = socket(AF_UNIX, SOCK_DGRAM, 0);
#if defined(pdp11)
			if (data->LogFile == -1) {
				data->LogFile = open(logfile, O_WRONLY|O_APPEND);
				data->ToFile = 1;
				data->connected = 1;
			} else
				data->ToFile = 0;
#endif
			if (data->LogFile == -1)
				mutex_unlock(&syslog_mutex);
				return;
			(void)fcntl(data->LogFile, F_SETFD, 1);
		}
	}
	if ((data->LogFile != -1) && !data->connected) {
		if (connect(data->LogFile, &SyslogAddr, sizeof(SyslogAddr)) == -1) {
			(void)close(data->LogFile);
			data->LogFile = -1;
		} else {
			data->connected = 1;
        }
    }
	mutex_unlock(&syslog_mutex);
}

void
closelog_r(struct syslog_data *data)
{
	mutex_lock(&syslog_mutex);
	(void)close(data->LogFile);
	data->LogFile = -1;
	data->connected = 0;
	mutex_unlock(&syslog_mutex);
}

int
setlogmask_r(int pmask, struct syslog_data *data)
{
	register int omask;

	omask = data->LogMask;
	if (pmask != 0)
		data->LogMask = pmask;
	return (omask);
}
