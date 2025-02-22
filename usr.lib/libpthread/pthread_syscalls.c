/*	$NetBSD: pthread_cancelstub.c,v 1.8 2003/11/24 23:23:17 cl Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_cancelstub.c,v 1.8 2003/11/24 23:23:17 cl Exp $");

#define	pthread_sys_fsync_range	_fsync_range

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <fcntl.h>

#include <stdarg.h>
#include <unistd.h>

int	pthread__cancel_stub_binder;

#include <signal.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "pthread.h"
#include "pthread_int.h"
#include "pthread_syscalls.h"

static int pthread_sys_timer_delete(int);

#define TESTCANCEL(id) 	do {			\
	if (__predict_false((id)->pt_cancel)) {	\
		pthread_exit(PTHREAD_CANCELED);	\
	} 					\
} while (/*CONSTCOND*/0)

int
pthread_sys_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
	int retval;

	retval = thr_atfork(prepare, parent, child);

	return (retval);
}

int
pthread_sys_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_accept(s, addr, addrlen);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_close(int d)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_close(d);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_connect(int s, const struct sockaddr *addr, socklen_t namelen)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_connect(s, addr, namelen);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_fcntl(int fd, int cmd, ...)
{
	int retval;
	pthread_t self;
	va_list ap;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_fcntl(fd, cmd, va_arg(ap, void *));
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_fsync(int d)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_fsync(d);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_fsync_range(int d, int f, off_t s, off_t e)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_fsync_range(d, f, s, e);
	TESTCANCEL(self);

	return (retval);
}

ssize_t
pthread_sys_msgrcv(int msgid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_msgrcv(msgid, msgp, msgsz, msgtyp, msgflg);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_msgsnd(int msgid, const void *msgp, size_t msgsz, int msgflg)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_msgsnd(msgid, msgp, msgsz, msgflg);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_msync(void *addr, size_t len, int flags)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_msync(addr, len, flags);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_open(const char *path, int flags, ...)
{
	int retval;
	pthread_t self;
	va_list ap;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_open(SYS_open, path, flags, va_arg(ap, mode_t));
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_poll(fds, nfds, timeout);
	TESTCANCEL(self);

	return (retval);
}

ssize_t
pthread_sys_pread(int d, void *buf, size_t nbytes, off_t offset)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_pread(d, buf, nbytes, offset);
	TESTCANCEL(self);

	return (retval);
}

ssize_t
pthread_sys_pwrite(int d, const void *buf, size_t nbytes, off_t offset)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_pwrite(d, buf, nbytes, offset);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_read(int d, void *buf, size_t nbytes)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_read(d, buf, nbytes);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_readv(int d, const struct iovec *iov, int iovcnt)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_readv(d, iov, iovcnt);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_select(nfds, readfds, writefds, exceptfds, timeout);
	TESTCANCEL(self);

	return (retval);
}

static int
pthread_sys_timer_delete(int timerid)
{
	if (timerid < 2 || timerid >= TIMER_MAX) {
		return (EINVAL);
	}
	return (0);
}

int
pthread_sys_timer_gettime(int timerid, struct itimerspec *value)
{
	struct itimerval aitv;
	struct itimerspec its;
	int error;

	error = pthread_sys_timer_delete(timerid);
	if (error != 0) {
		return (error);
	}

	error = thr_getitimer(timerid, &aitv);
	if (error != 0) {
		return (error);
	}
	TIMEVAL_TO_TIMESPEC(&aitv.it_interval, &its.it_interval);
	TIMEVAL_TO_TIMESPEC(&aitv.it_value, &its.it_value);
	return (0);
}

int
pthread_sys_timer_settime(int timerid, int flags, const struct itimerspec *value, struct itimerspec *ovalue)
{
	struct itimerval val, oval;
	int error;

	error = thr_setitimer(timerid, &val, &oval);
	if (error != 0) {
		return (error);
	}

	TIMESPEC_TO_TIMEVAL(&val.it_value, &value->it_value);
	TIMESPEC_TO_TIMEVAL(&val.it_interval, &value->it_interval);
	if (ovalue) {
		TIMEVAL_TO_TIMESPEC(&oval.it_value, &ovalue->it_value);
		TIMEVAL_TO_TIMESPEC(&oval.it_interval, &ovalue->it_interval);
	}
	return (0);
}

int
pthread_sys_wait4(pid_t wpid, int *status, int options, struct rusage *rusage)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_wait4(wpid, status, options, rusage);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_write(int d, const void *buf, size_t nbytes)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_write(d, buf, nbytes);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_writev(int d, const struct iovec *iov, int iovcnt)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = thr_writev(d, iov, iovcnt);
	TESTCANCEL(self);

	return (retval);
}

#ifdef __weak_alias
__weak_alias(pthread_sys_atfork, thr_atfork)
__weak_alias(pthread_sys_fsync_range, thr_fsync_range)
#endif /* __weak_alias */
__strong_alias(thr_accept, pthread_sys_accept)
__strong_alias(thr_close, pthread_sys_close)
__strong_alias(thr_fcntl, pthread_sys_fcntl)
__strong_alias(thr_fsync, pthread_sys_fsync)
__strong_alias(thr_msgrcv, pthread_sys_msgrcv)
__strong_alias(thr_msgsnd, pthread_sys_msgsnd)
__strong_alias(thr_msync, pthread_sys_msync)
__strong_alias(thr_open, pthread_sys_open)
__strong_alias(thr_poll, pthread_sys_poll)
__strong_alias(thr_pread, pthread_sys_pread)
__strong_alias(thr_pwrite, pthread_sys_pwrite)
__strong_alias(thr_read, pthread_sys_read)
__strong_alias(thr_readv, pthread_sys_readv)
__strong_alias(thr_select, pthread_sys_select)
__strong_alias(thr_getitimer, pthread_sys_timer_gettime)
__strong_alias(thr_setitimer, pthread_sys_timer_settime)
__strong_alias(thr_wait4, pthread_sys_wait4)
__strong_alias(thr_write, pthread_sys_write)
__strong_alias(thr_writev, pthread_sys_writev)
