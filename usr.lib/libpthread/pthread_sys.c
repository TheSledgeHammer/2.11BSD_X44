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

#define	fsync_range	_fsync_range

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <unistd.h>

int	pthread__cancel_stub_binder;

#include <signal.h>
#include <sys/mman.h>
#include <sys/socket.h>

#include "pthread.h"
#include "pthread_int.h"
#include "pthread_sys.h"

#define TESTCANCEL(id) 	do {					\
	if (__predict_false((id)->pt_cancel)) {		\
		pthread_exit(PTHREAD_CANCELED);			\
	} 											\
} while (/*CONSTCOND*/0)

int
pthread_sys_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_accept, s, addr, addrlen);
	TESTCANCEL(self);

	return retval;
}

int
pthread_sys_close(int d)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_close, d);
	TESTCANCEL(self);

	return retval;
}

int
pthread_sys_connect(int s, const struct sockaddr *addr, socklen_t namelen)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_connect, s, addr, namelen);
	TESTCANCEL(self);

	return retval;
}

int
pthread_sys_fcntl(int fd, int cmd, ...)
{
	int retval;
	pthread_t self;
	va_list ap;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_fcntl, fd, cmd, va_arg(ap, void *));
	TESTCANCEL(self);

	return retval;
}

int
pthread_sys_fsync(int d)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_fsync, d);
	TESTCANCEL(self);

	return retval;
}

int
pthread_sys_msync(void *addr, size_t len, int flags)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
    retval = __syscall(SYS_msync, addr, len, flags);
    TESTCANCEL(self);

    return retval;
}

int
pthread_sys_open(const char *path, int flags, ...)
{
	int retval;
	pthread_t self;
	va_list ap;

	self = pthread__self();
	TESTCANCEL(self);
    retval = __syscall(SYS_open, path, flags, va_arg(ap, mode_t));
    TESTCANCEL(self);

    return retval;
}

int
pthread_sys_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
    retval = ENOSYS;
    TESTCANCEL(self);

    return retval;
}

ssize_t
pthread_sys_pread(int d, void *buf, size_t nbytes, off_t offset)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
    retval = pread(d, buf, nbytes, offset);
    TESTCANCEL(self);

    return retval;
}

ssize_t
pthread_sys_pwrite(int d, const void *buf, size_t nbytes, off_t offset)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
    retval = pwrite(d, buf, nbytes, offset);
    TESTCANCEL(self);

    return retval;
}

int
pthread_sys_read(int d, void *buf, size_t nbytes)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
    retval = __syscall(SYS_read, d, buf, nbytes);
    TESTCANCEL(self);

    return retval;
}

int
pthread_sys_readv(int d, const struct iovec *iov, int iovcnt)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
    retval = __syscall(SYS_readv, d, iov, iovcnt);
    TESTCANCEL(self);

    return retval;
}

int
pthread_sys_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
    retval = __syscall(SYS_select, nfds, readfds, writefds, exceptfds, timeout);
    TESTCANCEL(self);

    return retval;
}

int
pthread_sys_wait4(pid_t wpid, int *status, int options, struct rusage *rusage)
{
	int retval;
	pthread_t self;

	self = pthread__self();
    retval = __syscall(SYS_wait4, wpid, status, options, rusage);
    TESTCANCEL(self);

	return retval;
}

int
pthread_sys_write(int d, const void *buf, size_t nbytes)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
    retval = __syscall(SYS_write, d, buf, nbytes);
    TESTCANCEL(self);

	return retval;
}

int
pthread_sys_writev(int d, const struct iovec *iov, int iovcnt)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
    retval = __syscall(SYS_writev, d, iov, iovcnt);
    TESTCANCEL(self);

	return retval;
}

int
pthread_sys_fsync_range(int d, int f, off_t s, off_t e)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = ENOSYS;
	TESTCANCEL(self);

	return ENOSYS;
}

ssize_t
pthread_sys_msgrcv(int msgid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
	ssize_t retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = ENOSYS;
	TESTCANCEL(self);

	return retval;
}

int
pthread_sys_msgsnd(int msgid, const void *msgp, size_t msgsz, int msgflg)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = ENOSYS;
	TESTCANCEL(self);

	return retval;
}

__strong_alias(_close, pthread_sys_close)
__strong_alias(_fcntl, pthread_sys_fcntl)
__strong_alias(_fsync, pthread_sys_fsync)
__weak_alias(pthread_sys_fsync_range, _fsync_range)
__strong_alias(_msgrcv, pthread_sys_msgrcv)
__strong_alias(_msgsnd, pthread_sys_msgsnd)
__strong_alias(_msync, pthread_sys_msync)
__strong_alias(_open, pthread_sys_open)
__strong_alias(_poll, pthread_sys_poll)
__strong_alias(_pread, pthread_sys_pread)
__strong_alias(_pwrite, pthread_sys_pwrite)
__strong_alias(_read, pthread_sys_read)
__strong_alias(_readv, pthread_sys_readv)
__strong_alias(_select, pthread_sys_select)
__strong_alias(_wait4, pthread_sys_wait4)
__strong_alias(_write, pthread_sys_write)
__strong_alias(_writev, pthread_sys_writev)
