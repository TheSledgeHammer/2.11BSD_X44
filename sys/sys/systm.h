/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)systm.h	1.3 (2.11BSD GTE) 1996/5/9
 */

#ifndef _SYS_SYSTM_H_
#define	_SYS_SYSTM_H_

#include <machine/cpufunc.h>
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
extern int securelevel;		/* system security level */
extern const char *panicstr;/* panic message */
extern char version[];		/* system version */

extern int nblkdev;			/* number of entries in bdevsw */
extern int nchrdev;			/* number of entries in cdevsw */
extern int nswdev;			/* number of swap devices */
extern int nswap;			/* size of swap space */

extern int boothowto;		/* reboot flags, from boot */
extern int selwait;			/* select timeout address */

extern int maxmem;			/* actual max memory per process */
extern int physmem;			/* physical memory */

extern dev_t rootdev;		/* device of the root */
extern dev_t dumpdev;		/* device to take dumps on */
extern long	dumplo;			/* offset into dumpdev */
extern dev_t swapdev;		/* swapping device */
extern dev_t pipedev;		/* pipe device */
extern int	nodev();		/* no device function used in bdevsw/cdevsw */

extern int	mpid;			/* generic for unique process id's */
extern char	runin;			/* scheduling flag */
extern char	runout;			/* scheduling flag */
extern int	runrun;			/* scheduling flag */
extern char	curpri;			/* more scheduling */

int		updlock;			/* lock for sync */
daddr_t	rablock;			/* block to be read ahead */

extern	int icode[];		/* user init code */
extern	int szicode;		/* its size */

daddr_t	bmap();

/*
 * Structure of the system-entry table
 */
extern struct sysent
{
	char	sy_narg;		/* total number of arguments */
	int		(*sy_call)();	/* handler */
} sysent[];
extern int nsysent;
#define	SCARG(p,k)	((p)->k.datum)	/* get arg from args pointer */

int	noproc;				/* no one is running just now */

/* casts to keep lint happy */
#define	insque(q,p)	_insque((caddr_t)q,(caddr_t)p)
#define	remque(q)	_remque((caddr_t)q)

extern	bool_t	sep_id;		/* separate I/D */
extern	char	regloc[];	/* offsets of saved user registers (trap.c) */

/* Initialize the world */
extern void startup();
extern void cinit();
extern void pqinit();


#endif
