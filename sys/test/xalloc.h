/*
 * xalloc.h
 *
 *  Created on: 14 Nov 2019
 *      Author: marti
 */

#ifndef SYS_XALLOC_H_
#define SYS_XALLOC_H_

#define KiB		1024u			/* Kibi 0x0000000000000400 */
#define MiB		1048576u		/* Mebi 0x0000000000100000 */
#define GiB		1073741824u		/* Gibi 000000000040000000 */

#define	BY2PG		(4 * KiB)	/* bytes per page */

#define BI2BY		8			/* bits per byte */
#define BY2V		8			/* only used in xalloc.c */

#define NHOLE 		128
#define MAGIC_HOLE 	0x484F4C45

struct Hole {
	u_long addr;
	u_long size;
	u_long top;
	Hole *link;
};
typedef struct Hole Hole;

struct Xhdr {
	u_long size;
	u_long magix;
	char data[];
};
typedef struct Xhdr Xhdr;

struct Xalloc {
	//Lock;
	Hole hole[NHOLE];
	Hole *flist;
	Hole *table;
};
typedef struct Xalloc Xalloc;

static Xalloc xlists;

struct Conf
{
	u_long	nmach;		/* processors */
	u_long	nproc;		/* processes */
	u_long	monitor;	/* has monitor? */
	Confmem	mem[4];		/* physical memory */
	u_long	npage;		/* total physical pages of memory */
	u_long	upages;		/* user page pool */
	u_long	nimage;		/* number of page cache image headers */
	u_long	nswap;		/* number of swap pages */
	int		nswppo;		/* max # of pageouts per segment pass */
	u_long	base0;		/* base of bank 0 */
	u_long	base1;		/* base of bank 1 */
	u_long	copymode;	/* 0 is copy on write, 1 is copy on reference */
	u_long	ialloc;		/* max interrupt time allocation in bytes */
	u_long	pipeqsize;	/* size in bytes of pipe queues */
	int		nuart;		/* number of uart devices */
};

struct Confmem
{
	u_long	base;
	u_long	npage;
	u_long	kbase;
	u_long	klimit;
};

struct Pallocmem
{
	u_long base;
	u_long npage;
};

struct Palloc
{
	Lock;
	Pallocmem	mem[4];
	Page		*head;			/* most recently used */
	Page		*tail;			/* least recently used */
	u_long		freecount;		/* how many pages on free list now */
	Page		*pages;			/* array of all pages */
	u_long		user;			/* how many user pages */
	Page		*hash[PGHSIZE];
	Lock		hashlock;
	Rendez		r;				/* Sleep for free mem */
	QLock		pwait;			/* Queue of procs waiting for memory */

	struct proc	*proc;
};

typedef struct Pool Pool;
typedef struct Conf	Conf;
typedef struct Confmem Confmem;
typedef struct Pallocmem Pallocmem;
typedef struct Palloc Palloc;

#endif /* SYS_XALLOC_H_ */
