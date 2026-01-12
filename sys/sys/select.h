/*-
 * Copyright (c) 1992, 1993
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
 *	@(#)select.h	8.2.1 (2.11BSD) 2000/2/28
 */

#ifndef _SYS_SELECT_H_
#define	_SYS_SELECT_H_

#include <sys/types.h>
#include <sys/event.h>		/* for struct klist */

/*
 * Select uses bit masks of file descriptors in longs.
 * These macros manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here
 * should be >= NOFILE (param.h).
 */
#ifndef	FD_SETSIZE
#define	FD_SETSIZE		256
#endif

typedef long			fd_mask;/* 32 = 2 ^ 5 */
#define NFDBITS			(sizeof(fd_mask) * NBBY)	/* bits per mask */
#define	NFDSHIFT		(5)
#define	NFDMASK			(NFDBITS - 1)

#define	NFD_LEN(a)		(((a) + (NFDBITS - 1)) / NFDBITS)
#define	NFD_SIZE		NFD_LEN(FD_SETSIZE)
#define	NFD_BYTES(a)		(NFD_LEN(a) * sizeof(fd_mask))

typedef	struct fd_set {
	fd_mask		fds_bits[1];
} fd_set;

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1L << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1L << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1L << ((n) % NFDBITS)))
#if __GNUC_PREREQ__(2, 95)
#define	FD_ZERO(p)	(void)__builtin_memset((p), 0, sizeof(*(p)))
#else
#define FD_ZERO(p)	(void)bzero((char *)(p), sizeof(*(p)))
#endif /* GCC 2.95 */

#if defined(__BSD_VISIBLE)
#if __GNUC_PREREQ__(2, 95)
#define	FD_COPY(f, t)	(void)__builtin_memcpy((t), (f), sizeof(*(f)))
#else
#define	FD_COPY(f, t)	(void)bcopy((f), (t), sizeof(*(f)))
#endif /* GCC 2.95 */
#endif /* __BSD_VISIBLE */
/*
 * Used to maintain information about processes that wish to be
 * notified when I/O becomes possible.
 */
struct selinfo {
	struct klist	si_klist;	/* knotes attached to this selinfo */
	pid_t			si_pid;		/* process to be notified */
	short			si_flags;	/* see below */
};
#define	SI_COLL		0x0001		/* collision occurred */

#ifdef _KERNEL
#include <sys/cdefs.h>
struct proc;

void	selrecord(struct proc *, struct selinfo *);
void	selwakeup(struct proc *, long);
void	_selwakeup(struct selinfo *);
int		selscan(fd_set *, fd_set *, int, int *);
int		seltrue(dev_t, int );
void 	selnotify(struct selinfo *, long);

/* 4.4BSD compat */
#define selwakeup1(sel) \
	(_selwakeup(sel))

#else
__BEGIN_DECLS
struct timeval;
struct timespec;
typedef unsigned long sigset_t;

/* must define __SELECT_DECLARED to use select */
#ifdef __SELECT_DECLARED
int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int	pselect(int, fd_set *, fd_set *, fd_set *, const struct timespec *, const sigset_t *);
#endif /* __SELECT_DECLARED */
__END_DECLS
#endif /* !KERNEL */
#endif /* !_SYS_SELECT_H_ */
