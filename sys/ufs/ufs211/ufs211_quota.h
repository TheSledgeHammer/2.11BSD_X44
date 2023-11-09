/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
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
 *	@(#)quota.h	8.3 (Berkeley) 8/19/94
 */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)quota.h	7.1.1 (2.11BSD) 1996/1/23
 */

/*
 * MELBOURNE DISC QUOTAS
 *
 * Various junk to do with various quotas (etc) imposed upon
 * the average user (big brother finally hits UNIX).
 *
 * The following structure exists in core for each logged on user.
 * It contains global junk relevant to that user's quotas.
 *
 * The u_quota field of each user struct contains a pointer to
 * the quota struct relevant to the current process, this is changed
 * by 'setuid' sys call, &/or by the Q_SETUID quota() call.
 */

#ifndef _UFS_UFS211_QUOTA_H_
#define	_UFS_UFS211_QUOTA_H_

//#include <sys/queue.h>

struct ufs211_quota {
    LIST_ENTRY(ufs211_quota) 	q_hash;			/* hash chain, MUST be first */
    short	                	q_cnt;			/* ref count (# processes) */
    uid_t	                	q_uid;			/* real uid of owner */
    int	                    	q_flags;		/* struct management flags */
#define	Q_LOCK	            	0x01			/* quota struct locked (for disc i/o) */
#define	Q_WANT	            	0x02			/* issue a wakeup when lock goes off */
#define	Q_NEW	            	0x04			/* new quota - no proc1 msg sent yet */
#define	Q_NDQ	            	0x08			/* account has NO disc quota */
    TAILQ_ENTRY(ufs211_quota) 	q_freelist;		/* free list */
    struct ufs211_dquot    		*q_dq;			/* disc quotas for mounted filesys's */
};

#define	NOQUOTA	((struct ufs211_quota *) 0)

/*
 * The following structure defines the format of the disc quota file
 * (as it appears on disc) - the file is an array of these structures
 * indexed by user number.  The setquota sys call establishes the inode
 * for each quota file (a pointer is retained in the mount structure).
 *
 * The following constants define the number of warnings given a user
 * before the soft limits are treated as hard limits (usually resulting
 * in an allocation failure).  The warnings are normally manipulated
 * each time a user logs in through the Q_DOWARN quota call.  If
 * the user logs in and is under the soft limit the warning count
 * is reset to MAX_*_WARN, otherwise a message is printed and the
 * warning count is decremented.  This makes MAX_*_WARN equivalent to
 * the number of logins before soft limits are treated as hard limits.
 */
#define	MAX_IQ_WARN	3
#define	MAX_DQ_WARN	3

#define	MAXQUOTAS	2
#define	USRQUOTA	0	/* element used for user quotas */
#define	GRPQUOTA	1	/* element used for group quotas */

/*
 * Definitions for the default names of the quotas files.
 */
#define INITQFNAMES { 			\
	"user",		/* USRQUOTA */ 	\
	"group",	/* GRPQUOTA */ 	\
	"undefined", 				\
};

#define	QUOTAFILENAME	"quotas"
#define	QUOTAGROUP	    "operator"

struct ufs211_dqblk {
	u_long	dqb_bhardlimit;	/* absolute limit on disc blks alloc */
	u_long	dqb_bsoftlimit;	/* preferred limit on disc blks */
	u_long	dqb_curblocks;	/* current block count */
	u_short	dqb_ihardlimit;	/* maximum # allocated inodes + 1 */
	u_short	dqb_isoftlimit;	/* preferred inode limit */
	u_short	dqb_curinodes;	/* current # allocated inodes */
	u_char	dqb_bwarn;	    /* # warnings left about excessive disc use */
	u_char	dqb_iwarn;	    /* # warnings left about excessive inodes */
};

/*
 * The following structure records disc usage for a user on a filesystem.
 * There is one allocated for each quota that exists on any filesystem
 * for the current user. A cache is kept of other recently used entries.
 */
struct ufs211_dquot {
	LIST_ENTRY(ufs211_dquot) 			dq_hash;
    union	{
        struct	ufs211_quota 			*Dq_own;
        struct {
        	TAILQ_ENTRY(ufs211_dquot) 	Dq_freelist;
        } dq_f;
    } dq_u;
    short				dq_flags;
#define	DQ_LOCK			0x01		/* this quota locked (no MODS) */
#define	DQ_WANT			0x02		/* wakeup on unlock */
#define	DQ_MOD			0x04		/* this quota modified since read */
#define	DQ_FAKE			0x08		/* no limits here, just usage */
#define	DQ_BLKS			0x10		/* has been warned about blk limit */
#define	DQ_INODS		0x20		/* has been warned about inode limit */
	short				dq_cnt;		/* count of active references */
	short 				dq_type;	/* quota type of this dquot */
	uid_t				dq_id;		/* user this applies to */
    struct ufs211_mount *dq_ump;	/* filesystem that this is taken from */
    struct ufs211_dqblk dq_dqb;		/* actual usage & quotas */
};
#define	dq_own		    dq_u.Dq_own
#define	dq_freelist    	dq_u.dq_f.Dq_freelist
#define	dq_bhardlimit	dq_dqb.dqb_bhardlimit
#define	dq_bsoftlimit	dq_dqb.dqb_bsoftlimit
#define	dq_curblocks	dq_dqb.dqb_curblocks
#define	dq_ihardlimit	dq_dqb.dqb_ihardlimit
#define	dq_isoftlimit	dq_dqb.dqb_isoftlimit
#define	dq_curinodes	dq_dqb.dqb_curinodes
#define	dq_bwarn	    dq_dqb.dqb_bwarn
#define	dq_iwarn	    dq_dqb.dqb_iwarn

#define	NODQUOT		((struct ufs211_dquot *) 0)
#define	LOSTDQUOT	((struct ufs211_dquot *) 1)

/*
 * Definitions for the 'quota' system call.
 */

#define SUBCMDMASK	0x00ff
#define SUBCMDSHIFT	8
#define	QCMD(cmd, type)	(((cmd) << SUBCMDSHIFT) | ((type) & SUBCMDMASK))

#define	Q_QUOTAON	0x0100	/* enable quotas */
#define	Q_QUOTAOFF	0x0200	/* disable quotas */
#define	Q_SETDLIM	0x0300	/* set disc limits & usage */
#define	Q_GETDLIM	0x0400	/* get disc limits & usage */
#define	Q_SETDUSE	0x0500	/* set disc usage only */
#define	Q_SYNC		0x0600	/* update disc copy of quota usages */
#define	Q_SETUID	0x0700	/* change proc to use quotas for uid */
#define	Q_SETWARN	0x0800	/* alter inode/block warning counts */
#define	Q_DOWARN	0x0900	/* warn user about excessive space/inodes */
#define	Q_SETUSE	0x1100	/* set usage */
#define	Q_GETQUOTA	0x1200	/* get limits and usage */
#define	Q_SETQUOTA	0x1300	/* set limits and usage */
/*
 * Used in Q_SETDUSE.
 */
struct ufs211_dqusage {
	u_short	du_curinodes;
	u_long	du_curblocks;
};

/*
 * Used in Q_SETWARN.
 */
struct ufs211_dqwarn {
	u_char	dw_bwarn;
	u_char	dw_iwarn;
};

#define	UFS211_NQHASH		16	/* small power of 2 */
#define	UFS211_NDQHASH		37	/* 4.3bsd used 51 which isn't even prime */

/*
 * Flags to chkdq() and chkiq()
 */
#define	FORCE	0x01	/* force usage changes independent of limits */
#define	CHOWN	0x02	/* (advisory) change initiated by chown */

/*
 * Macros to avoid subroutine calls to trivial functions.
 */
#ifdef DIAGNOSTIC
#define	DQREF(dq)	dqref(dq)
#else
#define	DQREF(dq)	(dq)->dq_cnt++
#endif

#include <sys/cdefs.h>

struct ufs211_dquot;
struct ufs211_inode;
struct mount;
struct proc;
struct ucred;
struct ufs211_mount;
struct vnode;

__BEGIN_DECLS
int		chkdq(struct ufs211_inode *, long, struct ucred *, int);
int		chkdqchg(struct ufs211_inode *, long, struct ucred *, int);
int		chkiq(struct ufs211_inode *, long, struct ucred *, int);
int		chkiqchg(struct ufs211_inode *, long, struct ucred *, int);
void 	dqflush(struct vnode *);
int		dqget(struct vnode *, u_long, struct ufs211_mount *, int, struct ufs211_dquot **);
void 	dqinit(void);
void 	dqref(struct ufs211_dquot *);
void 	dqrele(struct vnode *, struct ufs211_dquot *);
int		dqsync(struct vnode *, struct ufs211_dquot *);
int		getinoquota(struct ufs211_inode *);
int		getquota(struct mount *, u_long, int, caddr_t);
int		qsync(struct mount *);
int		quotaoff(struct proc *, struct mount *, int);
int		quotaon(struct proc *, struct mount *, int, caddr_t);
int		setquota(struct mount *, u_long, int, caddr_t);
int		setuse(struct mount *, u_long, int, caddr_t);
int 	setwarn(struct mount *, u_long, int, caddr_t);
int 	dowarn(struct mount *, u_long, int);
int		setduse(struct mount *, u_long, int, caddr_t);
int		ufs211_quotactl(struct mount *, int, uid_t, caddr_t, struct proc *);
__END_DECLS

#ifdef DIAGNOSTIC
__BEGIN_DECLS
void chkdquot(struct ufs211_inode *);
__END_DECLS
#endif

#endif /* _UFS_UFS211_QUOTA_H_ */
