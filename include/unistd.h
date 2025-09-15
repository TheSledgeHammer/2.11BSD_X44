/*-
 * Copyright (c) 1991, 1993, 1994
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
 *
 *	@(#)unistd.h	8.10.4 (2.11BSD) 1999/5/25
 */

/*
 * Modified for 2.11BSD by removing prototypes.  To save time and space
 * functions not returning 'int' and functions not present in the system
 * are not listed.
*/

#ifndef _UNISTD_H_
#define	_UNISTD_H_

#include <machine/ansi.h>
#include <machine/types.h>

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/unistd.h>

#if _FORTIFY_SOURCE > 0
#include <ssp/unistd.h>
#endif

#define STDIN_FILENO	0	/* standard input file descriptor */
#define	STDOUT_FILENO	1	/* standard output file descriptor */
#define	STDERR_FILENO	2	/* standard error file descriptor */

#include <sys/null.h>

__BEGIN_DECLS

void	 		_exit(int);
int	 			access(const char *, int);
unsigned int	alarm(int);
int	 			chdir(const char *);
int	 			chown(const char *, uid_t, gid_t);
int	 			close(int);
size_t			confstr(int, char *, size_t);
int	 			dup(int);
int	 			dup2(int, int);
int	            execl(const char *, const char *, ...) __null_sentinel;
int	            execle(const char *, const char *, ...);
int	            execlp(const char *, const char *, ...) __null_sentinel;
int	            execv(const char *, char * const *);
int	            execve(const char *, char * const *, char * const *);
int	            execvp(const char *, char * const *);
pid_t 		    fork(void);
long	 		fpathconf(int, int);
#if __SSP_FORTIFY_LEVEL == 0
char			*getcwd(char *, size_t);
#endif
gid_t	 		getegid(void);
uid_t			geteuid(void);
gid_t	 		getgid(void);
int	 		    getgroups(int, gid_t[]);
char			*getlogin(void);
pid_t	 		getpgrp(void);
pid_t	 		getpid(void);
pid_t	 		getppid(void);
uid_t	 		getuid(void);
int	 			isatty(int);
int	 			link(const char *, const char *);
off_t	 		lseek(int, off_t, int);
long	 		pathconf(const char *, int);
int	 			pause(void);
int	 			pipe(int *);
#if __SSP_FORTIFY_LEVEL == 0
ssize_t	 		read(int, void *, size_t);
#endif
int	 			rmdir(const char *);
int	 			setgid(gid_t);
int	 			setpgid(pid_t, pid_t);
pid_t		 	setsid(void);
int	 			setuid(uid_t);
unsigned int	sleep(unsigned int);
long	 		sysconf(int);
pid_t		 	tcgetpgrp(int);
int	 			tcsetpgrp(int, pid_t);
char			*ttyname(int);
int	 			unlink(const char *);
ssize_t			write(int, const void *, size_t);

#ifndef __SYS_SIGLIST_DECLARED
#define __SYS_SIGLIST_DECLARED
/* also in signal.h */
extern const char *sys_siglist[];
#endif /* __SYS_SIGLIST_DECLARED */
/*
 * IEEE Std 1003.2-92, adopted in X/Open Portability Guide Issue 4 and later
 */
#if (_POSIX_C_SOURCE - 0) >= 2 || defined(_XOPEN_SOURCE) || \
    defined(__BSD_VISIBLE)
extern	char	*optarg;		/* getopt(3) external variables */
extern	int	    opterr, optind, optopt;
extern	int     optreset;		/* getopt(3) external variable */
extern	char    *suboptarg;	    /* getsubopt(3) external variable */
int	 			getopt(int, char * const [], const char *);
#endif

/*
 * The Open Group Base Specifications, Issue 5; IEEE Std 1003.1-2001 (POSIX)
 */
#if (_POSIX_C_SOURCE - 0) >= 200112L || (_XOPEN_SOURCE - 0) >= 500 || \
    defined(__BSD_VISIBLE)
#if __SSP_FORTIFY_LEVEL == 0
ssize_t	 		readlink(const char * __restrict, char * __restrict, size_t);
#endif
#endif

/*
 * The Open Group Base Specifications, Issue 6; IEEE Std 1003.1-2001 (POSIX)
 */
#if (_POSIX_C_SOURCE - 0) >= 200112L || (_XOPEN_SOURCE - 0) >= 600 || \
    defined(__BSD_VISIBLE)
int	 			gethostname(char *, size_t);
int	 			setegid(gid_t);
int	 			seteuid(uid_t);
#endif

#if defined(_XOPEN_SOURCE) || (_POSIX_C_SOURCE - 0) >= 200809L || \
    defined(__BSD_VISIBLE)
pid_t	 		getsid(pid_t);
#endif

/*
 * IEEE Std 1003.1-2024 (POSIX.1-2024)
 */
#if (_POSIX_C_SOURCE - 0) >= 202405L || (_XOPEN_SOURCE - 0 >= 800) || \
    defined(__BSD_VISIBLE)
int	 			getentropy(void *, size_t);
int	 			pipe2(int *, int);
#endif

#ifndef	_POSIX_SOURCE
#ifdef	__STDC__
struct timeval;				/* select(2) */
#endif

/*
 * X/Open Portability Guide >= Issue 4 Version 2
 */
#if (defined(_XOPEN_SOURCE) && defined(_XOPEN_SOURCE_EXTENDED)) || \
    (_XOPEN_SOURCE - 0) >= 500 || defined(__BSD_VISIBLE)

#define F_ULOCK		0
#define F_LOCK		1
#define F_TLOCK		2
#define F_TEST		3

int	 			acct(const char *, pid_t);
char			*brk(const char *);
int	 			chroot(const char *);
char			*crypt(const char *, const char *);
int	 			des_cipher(const char *, char *, long, int);
int	 			des_setkey(const char *);
int	 			encrypt(char *, int);
void	 		endusershell(void);
int	 			exect(const char *, char * const *, char * const *);
int	 			execvpe(const char *, char * const *, char * const *);
int	 			fchdir(int);
int	 			fchown(int, int, int);
int	 			fsync(int);
int	 			ftruncate(int, off_t);
int	 			getdomainname(char *, size_t);
int	            getgrouplist(char *, gid_t, gid_t *, int *);
int	 			getdtablesize(void);
unsigned long	gethostid(void);
mode_t	 		getmode(const void *, mode_t);
int	 	        getpagesize(void) __attribute__((__pure__));
pid_t			getpgid(pid_t);
char			*getpass(const char *);
char			*getusershell(void);
char			*getwd(char *);
int	 			initgroups(const char *, gid_t);
int	 			iruserok(unsigned long, int, const char *, const char *);
int             issetugid(void);
int	 			lchown(const char *, uid_t, gid_t);
int	 			lockf(int, int, off_t);
int	 			mknod(const char *, mode_t, dev_t);
char			*mktemp(char *);
int	 			nice(int);
int	 			profil(char *, int, int, int);
int		 		rcmd(char **, int, const char *, const char *, const char *, int *);
char			*re_comp(char *);
int 			re_exec(char *);
char			*sbrk(int);
int	 			reboot(int, char *);
int	 			revoke(const char *);
int	 			rresvport(int *);
int	 			ruserok(const char *, int, const char *, const char *);
#ifdef __SELECT_DECLARED
/* backwards-compatibility; also in select.h */
int				select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
#endif /* __SELECT_DECLARED */
int	 			setdomainname(const char *, size_t);
int	 			setgroups(int, const gid_t *);
unsigned long	sethostid(unsigned long);
int	 			sethostname(const char *, size_t);
int	 			setkey(const char *);
int	 			setlogin(const char *);
void			*setmode(const char *);
void            strmode(mode_t, char *);
#ifndef __STRSIGNAL_DECLARED
#define __STRSIGNAL_DECLARED
/* backwards-compatibility; also in string.h */
char 			*strsignal(int);
#endif /* __STRSIGNAL_DECLARED */
int	 			setpgrp(pid_t, pid_t);	/* obsoleted by setpgid() */
int	 			setregid(gid_t, gid_t);
int	 			setreuid(uid_t, uid_t);
int	 			setrgid(gid_t);
int	 			setruid(uid_t);
void	 		setusershell(void);
void	 		swab(const void *, void *, ssize_t);
int	 			symlink(const char *, const char *);
void	 		sync(void);
int	 			swapctl(int, void *, int);
int	 			syscall(int, ...);
quad_t	 		__syscall(quad_t, ...);
int	 			truncate(const char *, off_t);
int	 			ttyslot(void);
int	 			undelete(const char *);
unsigned int	ualarm(unsigned int, unsigned int);
void	 		usleep(long);
pid_t	 		vfork(void);

#endif /* _XOPEN_SOURCE_EXTENDED || _XOPEN_SOURCE >= 500 || __BSD_VISIBLE */

/*
 * X/Open CAE Specification Issue 5 Version 2
 */
#if (_POSIX_C_SOURCE - 0) >= 200112L || (_XOPEN_SOURCE - 0) >= 500 || \
    defined(__BSD_VISIBLE)
ssize_t	     pread(int, void *, size_t, off_t);
ssize_t	     pwrite(int, const void *, size_t, off_t);
#endif /* (_POSIX_C_SOURCE - 0) >= 200112L || ... */
#endif /* !_POSIX_SOURCE */
__END_DECLS

#endif /* !_UNISTD_H_ */
