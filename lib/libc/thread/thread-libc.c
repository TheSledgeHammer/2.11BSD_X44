/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 */

#include <sys/cdefs.h>

#include "namespace.h"
#include "reentrant.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/syscall.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>  /* required for socklen_t */
#include <poll.h>
#include <select.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

//#include "thread_private.h"

int
__libc_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	return (__syscall(SYS_accept, s, addr, addrlen));
}

int
__libc_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	return (__syscall(SYS_clock_gettime, clock_id, tp));
}

int
__libc_clock_settime(clockid_t clock_id, const struct timespec *tp)
{
	return (__syscall(SYS_clock_settime, clock_id, tp));
}

int
__libc_close(int d)
{
	return (__syscall(SYS_close, d));
}

int
__libc_connect(int s, const struct sockaddr *addr, socklen_t namelen)
{
	return (__syscall(SYS_connect, s, addr, namelen));
}

int
__libc_execve(const char *path, char *const argv[], char *const envp[])
{
	return (__syscall(SYS_execve, path, argv, envp));
}

int
__libc_fcntl(int fd, int cmd, va_list ap)
{
	return (__syscall(SYS_fcntl, fd, cmd, ap));
}

int
__libc_fsync(int d)
{
	return (__syscall(SYS_fsync, d));
}

int
__libc_fsync_range(int d, int f, off_t s, off_t e)
{
	return (ENOSYS);
}

int
__libc_getitimer(unsigned int which, struct itimerval *itv)
{
	return (__syscall(SYS_setitimer, which, itv));
}

int
__libc_ksem_close(semid_t id)
{
	return (ENOSYS);
}

int
__libc_ksem_destroy(semid_t id)
{
	return (ENOSYS);
}

int
__libc_ksem_getvalue(semid_t id, unsigned int *value)
{
	return (ENOSYS);
}

int
__libc_ksem_init(unsigned int value, semid_t *idp)
{
	return (ENOSYS);
}

int
__libc_ksem_open(const char *name, int oflag, mode_t mode, unsigned int value, semid_t *idp)
{
	return (ENOSYS);
}

int
__libc_ksem_post(semid_t id)
{
	return (ENOSYS);
}

int
__libc_ksem_trywait(semid_t id)
{
	return (ENOSYS);
}

int
__libc_ksem_unlink(const char *name)
{
	return (ENOSYS);
}

int
__libc_ksem_wait(semid_t id)
{
	return (ENOSYS);
}

ssize_t
__libc_msgrcv(int msgid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
	return (ENOSYS);
}

int
__libc_msgsnd(int msgid, const void *msgp, size_t msgsz, int msgflg)
{
	return (ENOSYS);
}

int
__libc_msync(void *addr, size_t len, int flags)
{
	return (__syscall(SYS_msync, addr, len, flags));
}

int
__libc_nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	return (__syscall(SYS_nanosleep, rqtp, rmtp));
}

int
__libc_open(const char *path, int flags, va_list ap)
{
	return (__syscall(SYS_open, path, flags, ap));
}

int
__libc_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	return (__syscall(SYS_poll, fds, nfds, timeout));
}

ssize_t
__libc_pread(int d, void *buf, size_t nbytes, off_t offset)
{
	return (pread(d, buf, nbytes, offset));
}

ssize_t
__libc_pwrite(int d, const void *buf, size_t nbytes, off_t offset)
{
	return (pwrite(d, buf, nbytes, offset));
}

int
__libc_read(int d, void *buf, size_t nbytes)
{
	return (__syscall(SYS_read, d, buf, nbytes));
}

int
__libc_readv(int d, const struct iovec *iov, int iovcnt)
{
	return (__syscall(SYS_readv, d, iov, iovcnt));
}

int
__libc_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	return (__syscall(SYS_select, nfds, readfds, writefds, exceptfds, timeout));
}

int
__libc_setitimer(unsigned int which, struct itimerval *itv, struct itimerval *oitv)
{
	return (__syscall(SYS_setitimer, which, itv, oitv));
}

int
__libc_sigaction(int sig, struct sigaction *act, struct sigaction *oact)
{
	if (act == NULL) {
		return  __syscall(SYS_sigaction, 0, sig, act, oact);
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
__libc_sigsuspend(const sigset_t *sigmask)
{
	return (__syscall(SYS_sigsuspend, sigmask));
}

int
__libc_sigprocmask(int how, sigset_t *set, sigset_t *oset)
{
	return (__syscall(SYS_sigprocmask, how, set, oset));
}

int
__libc_sigtimedwait(const sigset_t * set, int *signo, struct timespec * timeout)
{
	return (__syscall(SYS_sigtimedwait, set, signo, timeout));
}

int
__libc_wait4(pid_t wpid, int *status, int options, struct rusage *rusage)
{
	return (__syscall(SYS_wait4, wpid, status, options, rusage));
}

int
__libc_write(int d, const void *buf, size_t nbytes)
{
	return (__syscall(SYS_write, d, buf, nbytes));
}

int
__libc_writev(int d, const struct iovec *iov, int iovcnt)
{
	return (__syscall(SYS_writev, d, iov, iovcnt));
}

__weak_alias(__libc_fsync_range, _fsync_range)

__strong_alias(_accept, __libc_accept)
__strong_alias(_clock_gettime, __libc_clock_gettime)
__strong_alias(_clock_settime, __libc_clock_settime)
__strong_alias(_close, __libc_close)
__strong_alias(_connect, __libc_connect)
__strong_alias(__exeve, __libc_execve)
__strong_alias(_fcntl, __libc_fcntl)
__strong_alias(_fsync, __libc_fsync)
__strong_alias(_getitimer, __libc_getitimer)
__strong_alias(_ksem_close, __libc_ksem_close)
__strong_alias(_ksem_destroy, __libc_ksem_destroy)
__strong_alias(_ksem_getvalue, __libc_ksem_getvalue)
__strong_alias(_ksem_init, __libc_ksem_init)
__strong_alias(_ksem_open, __libc_ksem_open)
__strong_alias(_ksem_post, __libc_ksem_post)
__strong_alias(_ksem_trywait, __libc_ksem_trywait)
__strong_alias(_ksem_unlink, __libc_ksem_unlink)
__strong_alias(_ksem_wait, __libc_ksem_wait)
__strong_alias(_msgrcv, __libc_msgrcv)
__strong_alias(_msgsnd, __libc_msgsnd)
__strong_alias(_msync, __libc_msync)
__strong_alias(_nanosleep, __libc_nanosleep)
__strong_alias(_open, __libc_open)
__strong_alias(_poll, __libc_poll)
__strong_alias(_pread, __libc_pread)
__strong_alias(_pwrite, __libc_pwrite)
__strong_alias(_read, __libc_read)
__strong_alias(_readv, __libc_readv)
__strong_alias(_select, __libc_select)
__strong_alias(_setitimer, __libc_setitimer)
__strong_alias(_sigaction, __libc_sigaction)
__strong_alias(_sigsuspend, __libc_sigsuspend)
__strong_alias(_sigprocmask, __libc_sigprocmask)
__strong_alias(_sigtimedwait, __libc_sigtimedwait)
__strong_alias(_wait4, __libc_wait4)
__strong_alias(_write, __libc_write)
__strong_alias(_writev, __libc_writev)
