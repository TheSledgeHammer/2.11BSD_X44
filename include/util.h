/*	$NetBSD: util.h,v 1.2 1996/05/16 07:00:22 thorpej Exp $	*/

/*-
 * Copyright (c) 1995
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

#ifndef _UTIL_H_
#define _UTIL_H_


#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/ansi.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <pwd.h>
#include <termios.h>
#include <utmp.h>

#ifdef  _BSD_TIME_T_
typedef _BSD_TIME_T_    time_t;
#undef  _BSD_TIME_T_
#endif
#ifdef  _BSD_SIZE_T_
typedef _BSD_SIZE_T_    size_t;
#undef  _BSD_SIZE_T_
#endif

#if defined(_POSIX_C_SOURCE)
#ifndef __VA_LIST_DECLARED
typedef __va_list va_list;
#define __VA_LIST_DECLARED
#endif
#endif

#define	PIDLOCK_NONBLOCK	1
#define	PIDLOCK_USEHOSTNAME	2

#define	PW_POLICY_BYSTRING	0
#define	PW_POLICY_BYPASSWD	1
#define	PW_POLICY_BYGROUP	2

__BEGIN_DECLS
struct disklabel;
struct iovec;
struct passwd;
struct termios;
struct utmp;
struct winsize;

void	login(struct utmp *);
int		login_tty(int);
int		logout(const char *);
void	logwtmp(const char *, const char *, const char *);
int		pw_lock(int retries);
int		pw_mkdb(void);
int		pw_abort(void);
void	pw_init(void);
void	pw_edit(int notsetuid, const char *filename);
void	pw_prompt(void);
void	pw_copy(int ffd, int tfd, struct passwd *pw);
int		pw_scan(char *bp, struct passwd *pw, int *flags);
void	pw_error(const char *name, int err, int eval);
int		openpty(int *, int *, char *, struct termios *, struct winsize *);
pid_t	forkpty(int *, char *, struct termios *, struct winsize *);
int		getmaxpartitions(void);
int		getrawpartition(void);
__END_DECLS

#endif /* !_UTIL_H_ */
