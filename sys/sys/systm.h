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

#define	SCARG(p,k)	((p)->k.datum)	/* get arg from args pointer */

extern int icode[];					/* user init code */
extern int szicode;					/* its size */

int	noproc;							/* no one is running just now */

/* casts to keep lint happy */
#define	insque(q,p)	_insque((caddr_t)q,(caddr_t)p)
#define	remque(q)	_remque((caddr_t)q)

/* Initialize the world */
extern void startup();
extern void cinit();
extern void pqinit();

/* General Function Declarations */
int 	nodev __P((void));
int 	nulldev __P((void));
int 	nullop __P((void));
int 	enodev __P((void));
int 	enoioctl __P((void));
int 	enxio __P((void));
int 	eopnotsupp __P((void));
int 	einval __P((void));
int 	nonet __P((void));
int		seltrue __P((dev_t dev, int flag));
void 	*hashinit __P((int count, int type, u_long *hashmask));

#ifdef __GNUC__
volatile void panic __P((const char *, ...));
#else
void panic __P((const char *, ...));
#endif
void	tablefull __P((const char *));
//void	addlog __P((const char *, ...));
void	log __P((int, const char *, ...));
void	printf __P((const char *, ...));
int		sprintf __P((char *buf, const char *, ...));
void	ttyprintf __P((struct tty *, const char *, ...));

void 	bcopy __P((const void *from, void *to, u_int len));
void 	ovbcopy __P((const void *from, void *to, u_int len));
void 	bzero __P((void *buf, u_int len));

int		copystr __P((void *kfaddr, void *kdaddr, u_int len, u_int *done));
int		copyinstr __P((void *udaddr, void *kaddr, u_int len, u_int *done));
//int		copyoutstr __P((void *kaddr, void *udaddr, u_int len, u_int *done));
int		copyin __P((void *udaddr, void *kaddr, u_int len));
int		copyout __P((void *kaddr, void *udaddr, u_int len));

int		fubyte __P((void *base));
#ifdef notdef
int		fuibyte __P((void *base));
#endif
int		subyte __P((void *base, int byte));
int		suibyte __P((void *base, int byte));
int		fuword __P((void *base));
int		fuiword __P((void *base));
int		suword __P((void *base, int word));
int		suiword __P((void *base, int word));

int		hzto __P((struct timeval *tv));
void 	timeout __P((void (*func)(void *), void *arg, int ticks));
void 	untimeout __P((void (*func)(void *), void *arg));

void 	hardclock __P((dev_t dev, caddr_t sp, int r1, int ov, int nps, int r0, caddr_t pc, int ps));
void 	softclock __P((caddr_t pc, int ps));
//void 	statclock __P((struct clockframe *frame));

#include <sys/libkern.h>
