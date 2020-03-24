/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)param.c	2.2 (2.11BSD GTE) 1997/2/14
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/clist.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>

#include <ufs/ufs/quota.h>

#include <sys/param.h>
#include <sys/systm.h>
#include "sys/buf.h>
#include "sys/time.h>
#include "sys/resource.h>
#include "sys/proc.h>
//#include "h/text.h"
#include "sys/file.h>
#include "sys/dir.h>
#include "sys/mount.h"
#include "sys/callout.h"
#include "sys/map.h"
#include "sys/clist.h"

/*
 * System parameter formulae.
 *
 * This file is copied into each directory where we compile
 * the kernel; it should be modified there to suit local taste
 * if necessary.
 *
 */

#define	MAXUSERS %MAXUSERS%
#define	NBUF %NBUF%

int	hz = %LINEHZ%;
u_short	mshz = (1000000L + %LINEHZ% - 1) / %LINEHZ%;
struct	timezone tz = { %TIMEZONE%, %DST% };

#define	NPROC (10 + 7 * MAXUSERS)
int	nproc = NPROC;
#define NTEXT (26 + MAXUSERS)
int	ntext = NTEXT;
#define NINODE ((NPROC + 16 + MAXUSERS) + 22)
int	ninode = NINODE;
#define NFILE ((8 * NINODE / 10) + 20)
int	nfile = NFILE;
#define NCALL (16 + MAXUSERS)
int	ncallout = NCALL;
int	nbuf = NBUF;

#define NCLIST (20 + 8 * MAXUSERS)
#if NCLIST > (8192 / 32)		/* 8K / sizeof(struct cblock) */
#undef NCLIST
#define NCLIST (8192 / 32)
#endif
int	nclist = NCLIST;

/*
 * These have to be allocated somewhere; allocating
 * them here forces loader errors if this file is omitted
 * (if they've been externed everywhere else; hah!).
 */
struct	proc *procNPROC;
struct	text *textNTEXT;
struct	inode inode[NINODE], *inodeNINODE;
struct	file *fileNFILE;
struct	callout callout[NCALL];
struct	mount mount[NMOUNT];
struct	buf buf[NBUF], bfreelist[BQUEUES];
struct	bufhd bufhash[BUFHSZ];

/*
 * Remove the ifdef/endif to run the kernel in unsecure mode even when in
 * a multiuser state.  Normally 'init' raises the security level to 1 
 * upon transitioning to multiuser.  Setting the securelevel to -1 prevents
 * the secure level from being raised by init.
*/
#ifdef	PERMANENTLY_INSECURE
int	securelevel = -1;
#endif

#ifdef UCB_CLIST
	u_int clstdesc = ((((btoc(NCLIST*sizeof(struct cblock)))-1) << 8) | RW);
	int ucb_clist = 1;
#else
	struct cblock	cfree[NCLIST];
	int ucb_clist = 0;
#endif

#define CMAPSIZ	NPROC				/* size of core allocation map */
#define SMAPSIZ	((9 * NPROC) / 10)	/* size of swap allocation map */

struct mapent	_coremap[CMAPSIZ];
struct map	coremap[1] = {
	_coremap,
	&_coremap[CMAPSIZ],
	"coremap",
};

struct mapent	_swapmap[SMAPSIZ];
struct map	swapmap[1] = {
	_swapmap,
	&_swapmap[SMAPSIZ],
	"swapmap",
};

#ifdef QUOTA
#include "../h/quota.h"
struct BigQ {
	struct	quota xquota[NQUOTA];		/* the quotas themselves */
	struct	dquot *ixdquot[NINODE];		/* 2.11 equiv of i_dquot */
	struct	dquot xdquot[NDQUOT];		/* the dquots themselves */
	struct	qhash xqhash[NQHASH];
	struct	dqhead xdqhash[NDQHASH];
};

QUOini()
{
	extern struct qhash *qhash;
	extern struct dqhead *dqhead;

	quota = ((struct BigQ *)SEG5)->xquota;
	dquot = ((struct BigQ *)SEG5)->xdquot;
	qhash = ((struct BigQ *)SEG5)->xqhash;
	dqhead = ((struct BigQ *)SEG5)->xdqhash;
	ndquot = NDQUOT;
	nquota = NQUOTA;
	ix_dquot = ((struct BigQ *)SEG5)->ixdquot;
	dquotNDQUOT = &dquot[ndquot];
	quotaNQUOTA = &quota[nquota];
}
#endif

/*
 * Declarations of structures loaded last and allowed to reside in the
 * 0120000-140000 range (where buffers and clists are mapped).  These
 * structures must be extern everywhere else, and the asm output of cc
 * is edited to move these structures from comm to bss (which is last)
 * (see the script :comm-to-bss).  They are in capital letters so that
 * the edit script doesn't find some other occurrence.
 */
struct proc	PROC[NPROC];
struct file	FILE[NFILE];
struct text	TEXT[NTEXT];
