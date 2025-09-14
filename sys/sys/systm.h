/*-
 * Copyright (c) 1982, 1988, 1991, 1993
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
 *	@(#)systm.h	8.7 (Berkeley) 3/29/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)systm.h	1.3 (2.11BSD GTE) 1996/5/9
 */

#ifndef	_SYS_SYSTEM_H_
#define	_SYS_SYSTEM_H_

#include <sys/stdarg.h>

/*
 * The `securelevel' variable controls the security level of the system.
 * It can only be decreased by process 1 (/sbin/init).
 *
 * Security levels are as follows:
 *   -1	permannently insecure mode - always run system in level 0 mode.
 *    0	insecure mode - immutable and append-only flags make be turned off.
 *	All devices may be read or written subject to permission modes.
 *    1	secure mode - immutable and append-only flags may not be changed;
 *	raw disks of mounted filesystems, /dev/mem, and /dev/kmem are
 *	read-only.
 *    2	highly secure mode - same as (1) plus raw disks are always
 *	read-only whether mounted or not. This level precludes tampering 
 *	with filesystems by unmounting them, but also inhibits running
 *	newfs while the system is secured.
 *
 * In normal operation, the system runs in level 0 mode while single user
 * and in level 1 mode while multiuser. If level 2 mode is desired while
 * running multiuser, it can be set in the multiuser startup script
 * (/etc/rc.local) using sysctl(8). If it is desired to run the system
 * in level 0 mode while multiuser, initialize the variable securelevel
 * in /sys/kern/kern_sysctl.c to -1. Note that it is NOT initialized to
 * zero as that would allow the vmunix binary to be patched to -1.
 * Without initialization, securelevel loads in the BSS area which only
 * comes into existence when the kernel is loaded and hence cannot be
 * patched by a stalking hacker.
 */

extern int securelevel;				/* system security level */
extern const char *panicstr;		/* panic message */
extern char version[];				/* system (kernel) version */
extern char ostype[];				/* system (kernel) type */
extern char osversion[];            /* OS version */
extern char osrelease[];            /* OS release */
extern long osrevision[];           /* OS revision */

extern int nblkdev;					/* number of entries in bdevsw */
extern int nchrdev;					/* number of entries in cdevsw */
extern int nswdev;					/* number of swap devices */
extern int nswap;					/* size of swap space */
extern int ntext;					/* number of texts (object cache) */

extern int boothowto;				/* reboot flags, from boot */
extern int bootverbose;				/* nonzero to print verbose messages */
extern int selwait;					/* select timeout address */

extern int maxmem;					/* actual max memory per process */
extern int physmem;					/* physical memory */

extern dev_t rootdev;				/* device of the root */
extern struct vnode *rootvp;		/* vnode equivalent to above */

extern dev_t dumpdev;				/* device to take dumps on */
extern long	 dumplo;				/* offset into dumpdev */

extern dev_t swapdev;				/* swapping device */
extern struct vnode *swapdev_vp;	/* vnode equivalent to above */

extern int	mpid;					/* generic for unique process id's */
extern int	runrun;					/* scheduling flag */
extern char runin;					/* scheduling flag */
extern char	runout;					/* scheduling flag */
extern char	curpri;					/* more scheduling */
extern int	noproc;					/* no one is running just now */

/* system call args */
#define	syscallarg(x)	union { x datum; register_t pad; }

#define	SCARG(p,k)		((p)->k.datum)	/* get arg from args pointer */

/*
 * exec system call, with and without environments.
 */
struct execa_args {
	syscallarg(char	*) 	fname;
	syscallarg(char	**) argp;
	syscallarg(char	**) envp;
};

void doexeca(struct execa_args *, char *, char **, char **);
int  execa(struct execa_args *);

/*
 * Structure of the system-entry table
 */
extern struct sysent {
	char	sy_narg;				/* total number of arguments */
	short	sy_argsize;				/* total size of arguments */
	int		(*sy_call)();			/* handler */
} sysent[];
extern int nsysent;

/* Initialize the world */
#ifdef _KERNEL
extern void startup(void);			/* cpu startup */
extern void consinit(void);			/* console startup */
extern void cinit(void);			/* clist startup */
extern void kmstartup(void);		/* gprof startup */
extern void kmeminit(void);			/* kmem startup (malloc) */
extern void mbinit(void);			/* mbuf startup */
extern void netstart(void);			/* network startup */
extern void rmapinit(void);			/* rmap startup (map) */
extern void vfsinit(void);			/* vfs startup */
#endif

/* General Function Declarations */
struct clockframe;
struct timeval;

int 	nodev(void);
int 	nulldev(void *);
int 	nullop(void *);
int 	enodev(void);
int 	enoioctl(void);
int 	enxio(void);
int 	eopnotsupp(void *);
int 	einval(void);
int 	nonet(void);
int 	nosys();

#ifdef _KERNEL
void 	*hashinit(int, int, u_long *);
void 	*hashfree(void *, int, int, u_long *);

int	ureadc(int, struct uio *);
#endif

void 	panic(const char *, ...) __attribute__((__noreturn__,__format__(__printf__,1,2)));
void	tablefull(const char *);
void	addlog(const char *, ...) __attribute__((__format__(__printf__,1,2)));
void	log(int, const char *, ...);
void	vlog(int, const char *, va_list);

#ifdef _KERNEL
/* subr_prf.c */
void	printf(const char *, ...) __attribute__((__format__(__printf__,1,2)));
int	sprintf(char *, const char *, ...)  __attribute__((__format__(__printf__,2,3)));
int	snprintf(char *, size_t, const char *, ...) __attribute__((__format__(__printf__,3,4)));
void	vprintf(const char *, va_list);
int	vsprintf(char *, const char *, va_list);
int	vsnprintf(char *, size_t, const char *, va_list);
#endif

void	ttyprintf(struct tty *, const char *, ...);
void	uprintf(const char *, ...) __attribute__((__format__(__printf__,1,2)));
void    printn(long, u_int, int, struct tty *);
char 	*bitmask_snprintf(u_quad_t, const char *, char *, size_t);

#ifdef _KERNEL
void 	ovbcopy(const void *, void *, u_int);
void 	bcopy(const void *, void *, u_int);
void 	bzero(void *, u_int);
int	bcmp(const void *, const void *, size_t);
#endif

int	copystr(const void *, void *, size_t, size_t *);
int	copyinstr(const void *, void *, size_t, size_t *);
int	copyoutstr(const void *, void *, size_t, size_t *);
int	copyin(const void *, void *, size_t);
int	copyout(const void *, void *, size_t);

int	fubyte(const void *);
int	fuibyte(void *);
int	fuword(const void *);
int	fuiword(void *);
int 	fusword(const void *);
int	subyte(void *, int);
int	suibyte(void *, int);
int	suword(void *, int);
int	suiword(void *, int);
int	susword(void *, int);

int	hzto(struct timeval *tv);
void 	timeout(void (*func)(void *), void *, int);
void 	untimeout(void (*func)(void *), void *);
void	realitexpire(void *);

void	initclocks(void);
void 	hardclock(struct clockframe *, caddr_t);
void 	softclock(struct clockframe *, caddr_t);
void	gatherstats(struct clockframe *);
void    startprofclock(struct proc *);
void    stopprofclock(struct proc *);

/* kern_environment.c / kenv.h */
char	*kern_getenv(const char *);
void	freeenv(char *);
int	getenv_int(const char *, int *);
int	getenv_uint(const char *, unsigned int *);
int	getenv_long(const char *, long *);
int	getenv_ulong(const char *, unsigned long *);
int	getenv_string(const char *, char *, int );
int	getenv_int64(const char *, int64_t *);
int	getenv_uint64(const char *, uint64_t *);
int	getenv_quad(const char *, quad_t *);
int	kern_setenv(const char *, const char *);
int	kern_unsetenv(const char *);
int	testenv(const char *);
int	getenv_array(const char *, void *, int, int *, int, bool_t);

#define	GETENV_UNSIGNED	false	/* negative numbers not allowed */
#define	GETENV_SIGNED	true	/* negative numbers allowed */

#ifdef _KERNEL
#include <lib/libkern/libkern.h>
#endif

extern const char hexdigits[];	/* "0123456789abcdef" in subr_prf.c */
extern const char HEXDIGITS[];	/* "0123456789ABCDEF" in subr_prf.c */

extern	void	_insque(void *, void *);
extern	void	_remque(void *);

/* casts to keep lint happy */
//#ifdef lint
#define	insque(q,p)	_insque(q, p)
#define	remque(q)	_remque(q)
//#endif
#endif /* !_SYS_SYSTEM_H_ */
