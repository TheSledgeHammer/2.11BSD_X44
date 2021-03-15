/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)systm.h	1.3 (2.11BSD GTE) 1996/5/9
 */
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

extern int nblkdev;					/* number of entries in bdevsw */
extern int nchrdev;					/* number of entries in cdevsw */
extern int nswdev;					/* number of swap devices */
extern int nswap;					/* size of swap space */

extern int boothowto;				/* reboot flags, from boot */
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

extern int icode[];					/* user init code */
extern int szicode;					/* its size */

#define	SCARG(p,k)		((p)->k.datum)	/* get arg from args pointer */
/* system call args */
#define	syscallarg(x)	union { x datum; register_t pad; }

/*
 * Structure of the system-entry table
 */
extern struct sysent {
	char	sy_narg;			/* total number of arguments */
	int		(*sy_call)();		/* handler */
} sysent[];

int	noproc;							/* no one is running just now */

/* Initialize the world */
extern void startup();

/* General Function Declarations */
int 	nodev (void);
int 	nulldev (void);
int 	nullop (void);
int 	enodev (void);
int 	enoioctl (void);
int 	enxio (void);
int 	eopnotsupp (void);
int 	einval (void);
int 	nonet (void);

int		selscan (fd_set *ibits, *obits, int nfd, int retval);
int		seltrue (dev_t dev, int flag);
void	selwakeup (struct proc *p, long coll);

void 	*hashinit (int count, int type, u_long *hashmask);

#ifdef __GNUC__
volatile void panic (const char *, ...);
#else
void 	panic (const char *, ...);
#endif
void	tablefull (const char *);
void	addlog (const char *, ...);
void	log (int, const char *, ...);
void	printf (const char *, ...);
int		sprintf (char *buf, const char *, ...);
void	ttyprintf (struct tty *, const char *, ...);

void 	bcopy (const void *from, void *to, u_int len);
void 	ovbcopy (const void *from, void *to, u_int len);
void 	bzero (void *buf, u_int len);

int		copystr (void *kfaddr, void *kdaddr, u_int len, u_int *done);
int		copyinstr (void *udaddr, void *kaddr, u_int len, u_int *done);
int		copyoutstr (void *kaddr, void *udaddr, u_int len, u_int *done);
int		copyin (void *udaddr, void *kaddr, u_int len);
int		copyout (void *kaddr, void *udaddr, u_int len);

int		fubyte (void *base);
#ifdef notdef
int		fuibyte (void *base);
#endif
int		subyte (void *base, int byte);
int		suibyte (void *base, int byte);
int		fuword (void *base);
int		fuiword (void *base);
int		suword (void *base, int word);
int		suiword (void *base, int word);

int		hzto (struct timeval *tv);
void 	timeout (void (*func)(void *), void *arg, int ticks);
void 	untimeout (void (*func)(void *), void *arg);
void	realitexpire (void *);

void 	hardclock (dev_t dev, caddr_t sp, int r1, int ov, int nps, int r0, caddr_t pc, int ps);
void 	softclock (caddr_t pc, int ps);
void	initclocks (void);

/* kern_environment.c / kenv.h */
char	*kern_getenv(const char *name);
void	freeenv(char *env);
int		getenv_int(const char *name, int *data);
int		getenv_uint(const char *name, unsigned int *data);
int		getenv_long(const char *name, long *data);
int		getenv_ulong(const char *name, unsigned long *data);
int		getenv_string(const char *name, char *data, int size);
int		getenv_int64(const char *name, int64_t *data);
int		getenv_uint64(const char *name, uint64_t *data);
int		getenv_quad(const char *name, quad_t *data);
int		kern_setenv(const char *name, const char *value);
int		kern_unsetenv(const char *name);
int		testenv(const char *name);
int		getenv_array(const char *name, void *data, int size, int *psize, int type_size, bool allow_signed);

#define	GETENV_UNSIGNED	false	/* negative numbers not allowed */
#define	GETENV_SIGNED	true	/* negative numbers allowed */

#include <lib/libkern/libkern.h>

/* casts to keep lint happy */
#ifdef lint
#define	insque(q,p)	_insque((caddr_t)q,(caddr_t)p)
#define	remque(q)	_remque((caddr_t)q)
#endif
