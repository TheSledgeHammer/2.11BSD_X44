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

#ifndef	_LIBC_THREAD_PRIVATE_H_
#define	_LIBC_THREAD_PRIVATE_H_

#include <pthread.h>

#ifdef _REENTRANT
#include <netdb.h>  /* required for socklen_t */
#include <poll.h>
#include <select.h>
#include <signal.h>
#include <stdarg.h>

struct iovec;
struct rusage;
struct sockaddr;
__BEGIN_DECLS
int 		__libc_accept(int, struct sockaddr *, socklen_t *);
int 		__libc_atfork(void (*)(void), void (*)(void), void (*)(void));
int		    __libc_clock_gettime(clockid_t, struct timespec *);
int		    __libc_clock_settime(clockid_t, const struct timespec *);
int 		__libc_close(int);
int 		__libc_connect(int, const struct sockaddr *, socklen_t);
int 		__libc_execve(const char *, char *const *, char *const *);
int 		__libc_fcntl(int, int, va_list);
pid_t		__libc_fork(void);
int 		__libc_fsync(int);
int 		__libc_fsync_range(int, int, off_t, off_t);
int		    __libc_getitimer(unsigned int, struct itimerval *);
int 		__libc_ksem_close(semid_t);
int 		__libc_ksem_destroy(semid_t);
int 		__libc_ksem_getvalue(semid_t, unsigned int *);
int 		__libc_ksem_init(unsigned int, semid_t *);
int 		__libc_ksem_open(const char *, int, mode_t, unsigned int, semid_t *);
int 		__libc_ksem_post(semid_t);
int 		__libc_ksem_trywait(semid_t);
int 		__libc_ksem_unlink(const char *);
int 		__libc_ksem_wait(semid_t);
ssize_t 	__libc_msgrcv(int, void *, size_t, long, int);
int 		__libc_msgsnd(int, const void *, size_t, int);
int 		__libc_msync(void *, size_t, int);
int		    __libc_nanosleep(const struct timespec *, struct timespec *);
int 		__libc_open(const char *, int, va_list);
int 		__libc_poll(struct pollfd *, nfds_t, int);
ssize_t 	__libc_pread(int, void *, size_t, off_t);
ssize_t 	__libc_pwrite(int, const void *, size_t, off_t);
int 		__libc_read(int, void *, size_t);
int 		__libc_readv(int, const struct iovec *, int);
int 		__libc_select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int		    __libc_setitimer(unsigned int, struct itimerval *, struct itimerval *);
int 		__libc_sigaction(int, struct sigaction *, struct sigaction *);
int 		__libc_sigsuspend(const sigset_t *);
int 		__libc_sigprocmask(int, sigset_t *, sigset_t *);
int 		__libc_sigtimedwait(const sigset_t *, int *, struct timespec *);
int		    __libc_wait4(pid_t, int *, int, struct rusage *);
int 		__libc_write(int, const void *, size_t);
int 		__libc_writev(int, const struct iovec *, int);
__END_DECLS

#define	thr_accept(s, addr, addrlen)				    __libc_accept(s, addr, addrlen)
#define	thr_atfork(prepare, parent, child)			    __libc_atfork(prepare, parent, child)
#define	thr_clock_gettime(clock_id, tp)				    __libc_clock_gettime(clock_id, tp)
#define	thr_clock_settime(clock_id, tp)				    __libc_clock_settime(clock_id, tp)
#define	thr_close(d)						            __libc_close(d)
#define	thr_connect(s, addr, namelen)				    __libc_connect(s, addr, namelen)
#define	thr_execve(fname, argp, envp)				    __libc_execve(fname, argp, envp)
#define	thr_fcntl(fd, cmd, ap)					        __libc_fcntl(fd, cmd, ap)
#define thr_fork()						                __libc_fork()
#define	thr_fsync(d)						            __libc_fsync(d)
#define	thr_fsync_range(d, f, s, e)				        __libc_fsync_range(d, f, s, e)
#define thr_getitimer(which, itv)				        __libc_getitimer(which, itv)
#define thr_ksem_close(id)					            __libc_ksem_close(id)
#define thr_ksem_destroy(id)					        __libc_ksem_destroy(id)
#define thr_ksem_getvalue(id, value)				    __libc_ksem_getvalue(id, value)
#define thr_ksem_init(value, idp)				        __libc_ksem_init(value, idp)
#define thr_ksem_open(name, oflag, mode, value, idp)	__libc_ksem_open(name, oflag, mode, value, idp)
#define thr_ksem_post(id) 					            __libc_ksem_post(id)
#define thr_ksem_trywait(id)					        __libc_ksem_trywait(id)
#define thr_ksem_unlink(name) 					        __libc_ksem_unlink(name)
#define thr_ksem_wait(id) 					            __libc_ksem_wait(id)
#define	thr_msgrcv(msgid, msgp, msgsz, msgtyp, msgflg) 	__libc_msgrcv(msgid, msgp, msgsz, msgtyp, msgflg)
#define	thr_msgsnd(msgid, msgp, msgsz, msgflg)			__libc_msgsnd(msgid, msgp, msgsz, msgflg)
#define	thr_msync(addr, len, flags)				        __libc_msync(addr, len, flags)
#define thr_nanosleep(rqtp, rmtp)				        __libc_nanosleep(rqtp, rmtp)
#define	thr_open(path, flags, ap)				        __libc_open(path, flags, ap)
#define	thr_poll(fds, nfds, timeout)				    __libc_poll(fds, nfds, timeout)
#define	thr_pread(d, buf, nbytes, offset)			    __libc_pread(d, buf, nbytes, offset)
#define	thr_pwrite(d, buf, nbytes, offset)			    __libc_pwrite(d, buf, nbytes, offset)
#define	thr_read(d, buf, nbytes)				        __libc_read(d, buf, nbytes)
#define	thr_readv(d, iov, iovcnt)				        __libc_readv(d, iov, iovcnt)
#define	thr_select(nfds, readfds, writefds, exceptfds, timeout) __libc_select(nfds, readfds, writefds, exceptfds, timeout)
#define thr_setitimer(which, itv, oitv)				    __libc_setitimer(which, itv, oitv)
#define	thr_sigaction(sig, act, oact) 				    __libc_sigaction(sig, act, oact)
#define	thr_sigmask(how, set, oset) 				    __libc_sigprocmask(how, set, oset)
#define	thr_sigsuspend(sigmask) 				        __libc_sigsuspend(sigmask)
#define	thr_timedwait(set, signo, timeout) 			    __libc_sigtimedwait(set, signo, timeout)
#define	thr_wait4(wpid, status, options, rusage) 		__libc_wait4(wpid, status, options, rusage)
#define	thr_write(d, buf, nbytes) 				        __libc_write(d, buf, nbytes)
#define	thr_writev(d, iov, iovcnt) 				        __libc_writev(d, iov, iovcnt)

#else

#define	thr_accept(s, addr, addrlen)
#define	thr_atfork(prepare, parent, child)
#define	thr_clock_gettime(clock_id, tp)
#define	thr_clock_settime(clock_id, tp)
#define	thr_close(d)
#define	thr_connect(s, addr, namelen)
#define	thr_execve(fname, argp, envp)
#define	thr_fcntl(fd, cmd, ap)
#define thr_fork()
#define	thr_fsync(d)
#define	thr_fsync_range(d, f, s, e)
#define thr_getitimer(which, itv)
#define thr_ksem_close(id)
#define thr_ksem_destroy(id)
#define thr_ksem_getvalue(id, value)
#define thr_ksem_init(value, idp)
#define thr_ksem_open(name, oflag, mode, value, idp)
#define thr_ksem_post(id)
#define thr_ksem_trywait(id)
#define thr_ksem_unlink(name)
#define thr_ksem_wait(id)
#define	thr_msgrcv(msgid, msgp, msgsz, msgtyp, msgflg)
#define	thr_msgsnd(msgid, msgp, msgsz, msgflg)
#define	thr_msync(addr, len, flags)
#define thr_nanosleep(rqtp, rmtp)
#define	thr_open(path, flags, ap)
#define	thr_poll(fds, nfds, timeout)
#define	thr_pread(d, buf, nbytes, offset)
#define	thr_pwrite(d, buf, nbytes, offset)
#define	thr_read(d, buf, nbytes)
#define	thr_readv(d, iov, iovcnt)
#define	thr_select(nfds, readfds, writefds, exceptfds, timeout)
#define thr_setitimer(which, itv, oitv)
#define	thr_sigaction(sig, act, oact)
#define	thr_sigmask(how, set, oset)
#define	thr_sigsuspend(sigmask)
#define	thr_timedwait(set, signo, timeout)
#define	thr_wait4(wpid, status, options, rusage)
#define	thr_write(d, buf, nbytes)
#define	thr_writev(d, iov, iovcnt)

#endif /* _REENTRANT */
#endif	/* _LIBC_THREAD_PRIVATE_H_ */
