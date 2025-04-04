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

#ifndef _LIB_PTHREAD_SYSCALLS_H
#define _LIB_PTHREAD_SYSCALLS_H

#include <netdb.h>
#include <stdarg.h>

struct iovec;
struct pollfd;
struct rusage;
struct sockaddr;
__BEGIN_DECLS
int		pthread_sys_accept(int, struct sockaddr *, socklen_t *);
int		pthread_sys_clock_gettime(clockid_t, struct timespec *);
int		pthread_sys_clock_settime(clockid_t, const struct timespec *);
int		pthread_sys_close(int);
int		pthread_sys_connect(int, const struct sockaddr *, socklen_t);
int		pthread_sys_execve(const char *, char *const *, char *const *);
int		pthread_sys_fcntl(int, int, va_list);
int		pthread_sys_fsync(int);
int		pthread_sys_fsync_range(int, int, off_t, off_t);
ssize_t		pthread_sys_msgrcv(int, void *, size_t, long, int);
int		pthread_sys_msgsnd(int, const void *, size_t, int);
int		pthread_sys_msync(void *, size_t, int);
int		pthread_sys_open(const char *, int, va_list);
int		pthread_sys_nanosleep(const struct timespec *, struct timespec *);
int		pthread_sys_poll(struct pollfd *, unsigned long, int);
ssize_t		pthread_sys_pread(int, void *, size_t, off_t);
ssize_t		pthread_sys_pwrite(int, const void *, size_t, off_t);
int		pthread_sys_read(int, void *, size_t);
int		pthread_sys_readv(int, const struct iovec *, int);
int		pthread_sys_select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int		pthread_sys_sigaction(int, const struct sigaction *, struct sigaction *);
int		pthread_sys_sigprocmask(int, sigset_t *, sigset_t *);
int		pthread_sys_sigsuspend(const sigset_t *);
int		pthread_sys_sigtimedwait(const sigset_t *, int *, struct timespec *);
int		pthread_sys_timer_create(int, int, int *);
int		pthread_sys_timer_gettime(int, struct itimerspec *);
int		pthread_sys_timer_settime(int, int, const struct itimerspec *, struct itimerspec *);
int		pthread_sys_wait4(pid_t, int *, int, struct rusage *);
int		pthread_sys_write(int, const void *, size_t);
int		pthread_sys_writev(int, const struct iovec *, int);

int 		pthread_sys_ksem_close(semid_t);
int 		pthread_sys_ksem_destroy(semid_t);
int 		pthread_sys_ksem_getvalue(semid_t, int *);
int 		pthread_sys_ksem_init(unsigned int, semid_t *);
int 		pthread_sys_ksem_open(const char *, int, mode_t, unsigned int, semid_t *);
int 		pthread_sys_ksem_post(semid_t);
int 		pthread_sys_ksem_trywait(semid_t);
int 		pthread_sys_ksem_unlink(const char *);
int 		pthread_sys_ksem_wait(semid_t);
__END_DECLS

#endif /* _LIB_PTHREAD_SYSCALLS_H */
