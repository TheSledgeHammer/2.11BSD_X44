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

#include <sys/types.h>

#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>

int	pthread__cancel_stub_binder;

#include "pthread.h"
#include "pthread_int.h"
#include "pthread_syscalls.h"

/* 
 * Some alias's may need to change from strong to weak.
 * Noteably the ones blanked out. 
 * Cause multiple definition errors during compliation.
 */

__weak_alias(pthread_sys_fsync_range, _fsync_range)

__strong_alias(_accept, pthread_sys_accept)
__strong_alias(_clock_gettime, pthread_sys_clock_gettime)
__strong_alias(_clock_settime, pthread_sys_clock_settime)
__strong_alias(_close, pthread_sys_close)
__strong_alias(_connect, pthread_sys_connect)
__strong_alias(__exeve, pthread_sys_execve)
__strong_alias(_fcntl, pthread_sys_fcntl)
__strong_alias(_fsync, pthread_sys_fsync)
__strong_alias(_getitimer, pthread_sys_getitimer)

__strong_alias(_ksem_close, pthread_sys_ksem_close)
__strong_alias(_ksem_destroy, pthread_sys_ksem_destroy)
__strong_alias(_ksem_getvalue, pthread_sys_ksem_getvalue)
__strong_alias(_ksem_init, pthread_sys_ksem_init)
__strong_alias(_ksem_open, pthread_sys_ksem_open)
__strong_alias(_ksem_post, pthread_sys_ksem_post)
__strong_alias(_ksem_trywait, pthread_sys_ksem_trywait)
__strong_alias(_ksem_unlink, pthread_sys_ksem_unlink)
__strong_alias(_ksem_wait, pthread_sys_ksem_wait)

__strong_alias(_msgrcv, pthread_sys_msgrcv)
__strong_alias(_msgsnd, pthread_sys_msgsnd)
__strong_alias(_msync, pthread_sys_msync)
__strong_alias(_nanosleep, pthread_sys_nanosleep)
__strong_alias(_open, pthread_sys_open)
__strong_alias(_poll, pthread_sys_poll)
__strong_alias(_pread, pthread_sys_pread)
__strong_alias(_pwrite, pthread_sys_pwrite)
__strong_alias(_read, pthread_sys_read)
__strong_alias(_readv, pthread_sys_readv)
__strong_alias(_select, pthread_sys_select)
__strong_alias(_setitimer, pthread_sys_setitimer)
__strong_alias(_sigaction, pthread_sys_sigaction)
__strong_alias(_sigsuspend, pthread_sys_sigsuspend)
__strong_alias(_sigprocmask, pthread_sys_sigprocmask)
__strong_alias(_sigtimedwait, pthread_sys_sigtimedwait)
__strong_alias(_wait4, pthread_sys_wait4)
__strong_alias(_write, pthread_sys_write)
__strong_alias(_writev, pthread_sys_writev)

static int pthread_sys_timer_delete(int);

#define TESTCANCEL(id) 	do {			\
	if (__predict_false((id)->pt_cancel)) {	\
		pthread_exit(PTHREAD_CANCELED);	\
	} 					\
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

	return (retval);
}

int
pthread_sys_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_clock_gettime, clock_id, tp);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_clock_settime(clockid_t clock_id, const struct timespec *tp)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_clock_settime, clock_id, tp);
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
	retval = __syscall(SYS_close, d);
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
	retval = __syscall(SYS_connect, s, addr, namelen);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_execve(const char *path, char *const argv[], char *const envp[])
{
	return (__syscall(SYS_execve, path, argv, envp));
}

int
pthread_sys_fcntl(int fd, int cmd, va_list ap)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_fcntl,fd, cmd, va_arg(ap, void *));
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
	retval = __syscall(SYS_fsync, d);
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
	retval = ENOSYS;
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_ksem_close(semid_t id)
{
	return (ENOSYS);
}

int
pthread_sys_ksem_destroy(semid_t id)
{
	return (ENOSYS);
}

int
pthread_sys_ksem_getvalue(semid_t id, int *value)
{
	return (ENOSYS);
}

int
pthread_sys_ksem_init(unsigned int value, semid_t *idp)
{
	return (ENOSYS);
}

int
pthread_sys_ksem_open(const char *name, int oflag, mode_t mode, unsigned int value, semid_t *idp)
{
	return (ENOSYS);
}

int
pthread_sys_ksem_post(semid_t id)
{
	return (ENOSYS);
}

int
pthread_sys_ksem_trywait(semid_t id)
{
	return (ENOSYS);
}

int
pthread_sys_ksem_unlink(const char *name)
{
	return (ENOSYS);
}

int
pthread_sys_ksem_wait(semid_t id)
{
	return (ENOSYS);
}

ssize_t
pthread_sys_msgrcv(int msgid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = ENOSYS;
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
	retval = ENOSYS;
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
	retval = __syscall(SYS_msync, addr, len, flags);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_nanosleep, rqtp, rmtp);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_open(const char *path, int flags, va_list ap)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_open, path, flags, va_arg(ap, mode_t));
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_poll(struct pollfd *fds, unsigned long nfds, int timeout)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_poll, fds, nfds, timeout);
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
	retval = __syscall(SYS_pread, d, buf, nbytes, offset);
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
	retval = __syscall(SYS_pwrite, d, buf, nbytes, offset);
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
	retval = __syscall(SYS_read, d, buf, nbytes);
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
	retval = __syscall(SYS_readv, d, iov, iovcnt);
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
	retval = __syscall(SYS_select, nfds, readfds, writefds, exceptfds, timeout);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	if (act == NULL) {
		return  (__syscall(SYS_sigaction, 0, sig, act, oact));
	}

	if ((act->sa_flags & SA_SIGINFO) == 0) {
		int sav = errno;
		int rv = __syscall(SYS_sigaction, 0, sig, act, oact);
		if (rv >= 0 || errno != EINVAL) {
			return (rv);
		}
		errno = sav;
	}

	return (__syscall(SYS_sigaction, 0, sig, act, oact));
}

int
pthread_sys_sigprocmask(int how, sigset_t *set, sigset_t *oset)
{
	return (__syscall(SYS_sigprocmask, how, set, oset));
}

int
pthread_sys_sigsuspend(const sigset_t *sigmask)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_sigsuspend, sigmask);
	TESTCANCEL(self);

	return (retval);
}

int
pthread_sys_sigtimedwait(const sigset_t * set, int *signo, struct timespec * timeout)
{
	int retval;
	pthread_t self;

	self = pthread__self();
	TESTCANCEL(self);
	retval = __syscall(SYS_sigtimedwait, set, signo, timeout);
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
	int retval;

	retval = pthread_sys_timer_delete(timerid);
	if (retval != 0) {
		return (retval);
	}

	retval = __syscall(SYS_getitimer, timerid, &aitv);
	if (retval != 0) {
		return (retval);
	}
	TIMEVAL_TO_TIMESPEC(&aitv.it_interval, &its.it_interval);
	TIMEVAL_TO_TIMESPEC(&aitv.it_value, &its.it_value);
	return (0);
}

int
pthread_sys_timer_settime(int timerid, int flags, const struct itimerspec *value, struct itimerspec *ovalue)
{
	struct itimerval val, oval;
	int retval;

	retval = __syscall(SYS_setitimer, timerid, &val, &oval);
	if (retval != 0) {
		return (retval);
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
	retval = __syscall(SYS_wait4, wpid, status, options, rusage);
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
	retval = __syscall(SYS_write, d, buf, nbytes);
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
	retval = __syscall(SYS_writev, d, iov, iovcnt);
	TESTCANCEL(self);

	return (retval);
}
