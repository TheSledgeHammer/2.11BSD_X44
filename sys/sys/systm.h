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
extern char version[];				/* system version */
extern char ostype[];
extern char osversion[];
extern char osrelease[];
extern long osrevision[];

extern int nblkdev;					/* number of entries in bdevsw */
extern int nchrdev;					/* number of entries in cdevsw */
extern int nswdev;					/* number of swap devices */
extern int nswap;					/* size of swap space */

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
extern char	runin;					/* scheduling flag */
extern char	runout;					/* scheduling flag */
extern int	runrun;					/* scheduling flag */
extern char	curpri;					/* more scheduling */

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

/*
 * Structure of the system-entry table
 */
extern struct sysent {
	char	sy_narg;				/* total number of arguments */
	short	sy_argsize;				/* total size of arguments */
	int		(*sy_call)();			/* handler */
} sysent[];
extern int nsysent;

int	noproc;							/* no one is running just now */

/* Initialize the world */
int 				execa(struct execa_args *);
void				execa_set(struct execa_args *, char *, char **, char **);
struct execa_args 	*execa_get(void);

extern void startup(void);			/* cpu startup */
extern void consinit(void);			/* console startup */
extern void cinit(void);			/* clist startup */
extern void kmstartup(void);		/* gprof startup */
extern void kmeminit(void);			/* kmem startup (malloc) */
extern void mbinit(void);			/* mbuf startup */
extern void rmapinit(void);			/* rmap startup (map) */
extern void vfsinit(void);			/* vfs startup */

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

void 	*hashinit(int, int, u_long *);
void 	*hashfree(void *, int, int, u_long *);

int		ureadc(int, struct uio *);

void 	panic(const char *, ...);
void	tablefull(const char *);
void	addlog(const char *, ...);
void	log(int, const char *, ...);
void	vlog(int, const char *, va_list);

/* subr_prf.c */
void	printf(const char *, ...);
int		sprintf(char *, const char *, ...);
int		snprintf(char *, size_t, const char *, ...);
void	vprintf(const char *, va_list);
int		vsprintf(char *, const char *, va_list);
int		vsnprintf(char *, size_t, const char *, va_list);

void	ttyprintf(struct tty *, const char *, ...);
void	uprintf(const char *, ...);
void    printn(long, u_int, int, struct tty *);
char 	*bitmask_snprintf(u_quad_t, const char *, char *, size_t);

void 	bcopy(const void *, void *, u_int);
void 	ovbcopy(const void *, void *, u_int);
void 	bzero(void *, u_int);

int		copystr(const void *, void *, size_t, size_t *);
int		copyinstr(const void *, void *, size_t, size_t *);
int		copyoutstr(const void *, void *, size_t, size_t *);
int		copyin(const void *, void *, size_t);
int		copyout(const void *, void *, size_t);

int		fubyte(const void *);
int		fuibyte(void *);
int		fuword(const void *);
int		fuiword(void *);
int 		fusword(const void *);
int		subyte(void *, int);
int		suibyte(void *, int);
int		suword(void *, int);
int		suiword(void *, int);
int		susword(void *, int);

int		hzto(struct timeval *tv);
void 	timeout(void (*func)(void *), void *, int);
void 	untimeout(void (*func)(void *), void *);
void	realitexpire(void *);

void	initclocks(void);
void 	hardclock(struct clockframe *, caddr_t);
void 	softclock(struct clockframe *, caddr_t);
void	gatherstats(struct clockframe *);
void    startprofclock(struct proc *);
void    stopprofclock(struct proc *);

/* internal syscalls related */
void    syscall();

/* kern_environment.c / kenv.h */
char	*kern_getenv(const char *);
void	freeenv(char *);
int		getenv_int(const char *, int *);
int		getenv_uint(const char *, unsigned int *);
int		getenv_long(const char *, long *);
int		getenv_ulong(const char *, unsigned long *);
int		getenv_string(const char *, char *, int );
int		getenv_int64(const char *, int64_t *);
int		getenv_uint64(const char *, uint64_t *);
int		getenv_quad(const char *, quad_t *);
int		kern_setenv(const char *, const char *);
int		kern_unsetenv(const char *);
int		testenv(const char *);
int		getenv_array(const char *, void *, int, int *, int, bool_t);

#define	GETENV_UNSIGNED	false	/* negative numbers not allowed */
#define	GETENV_SIGNED	true	/* negative numbers allowed */

#ifdef _KERNEL
#include <lib/libkern/libkern.h>
#endif

/* casts to keep lint happy */
#ifdef lint
#define	insque(q,p)	_insque((caddr_t)q,(caddr_t)p)
#define	remque(q)	_remque((caddr_t)q)
#endif
#endif /* !_SYS_SYSTEM_H_ */
