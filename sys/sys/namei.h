/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)namei.h	1.3 (2.11BSD) 1997/1/18
 */

#ifndef _NAMEI_
#define	_NAMEI_

#include <sys/queue.h>

/*
 * Encapsulation of namei parameters.
 * One of these is located in the u. area to
 * minimize space allocated on the kernel stack.
 */
struct nameidata {
	caddr_t				ni_dirp;			/* pathname pointer */
	enum	uio_seg		ni_segflg;			/* segment flag */
	short				ni_nameiop;			/* see below */
	struct	vnode 		*ni_cdir;			/* current directory */
	struct	vnode 		*ni_rdir;			/* root directory, if not normal root */

	/* Arguments to lookup */
	struct	vnode 		*ni_startdir;		/* starting directory */
	struct	vnode 		*ni_rootdir;		/* logical root directory */

	/* Results: returned from/manipulated by lookup  */
	struct	vnode 		*ni_vp;				/* vnode of result */
	struct	vnode 		*ni_dvp;			/* vnode of intermediate directory */

	/* Shared between namei and lookup/commit routines. */
	long				ni_pathlen;			/* remaining chars in path */
	char				*ni_next;			/* next location in pathname */
	u_long				ni_loopcnt;			/* count of symlinks encountered */

	struct componentname {
		/* Arguments to lookup */
		u_long			cn_nameiop;			/* namei operation */
		u_long			cn_flags;			/* flags to namei */
		struct	proc 	*cn_proc;			/* process requesting lookup */
		struct	ucred 	*cn_cred;			/* credentials */

		/* Shared between lookup and commit routines */
		char			*cn_pnbuf;			/* pathname buffer */
		char			*cn_nameptr;		/* pointer to looked up name */
		long			cn_namelen;			/* length of looked up component */
		u_long			cn_hash;			/* hash value of looked up name */
		long			cn_consume;			/* chars to consume in lookup() */
	} ni_cnd;
};

#ifdef KERNEL
/*
 * namei operations and modifiers, stored in ni_cnd.flags
 */
#define	LOOKUP		0		/* perform name lookup only */
#define	CREATE		1		/* setup for file creation */
#define	DELETE		2		/* setup for file deletion */
#define	RENAME		3		/* setup for file renaming */
#define	OPMASK		3		/* mask for operation */


#define	LOCKLEAF	0x0004	/* lock inode on return */
#define	LOCKPARENT	0x0008	/* want parent vnode returned locked */
#define	WANTPARENT	0x0010	/* want parent vnode returned unlocked */
#define NOCACHE		0x0020	/* name must not be left in cache */
#define FOLLOW		0x0040	/* follow symbolic links */
#define	NOFOLLOW	0x0000	/* don't follow symbolic links (pseudo) */
#define	MODMASK		0x00fc	/* mask of operational modifiers */

/*
 * Namei parameter descriptors.
 *
 * SAVENAME may be set by either the callers of namei or by VOP_LOOKUP.
 * If the caller of namei sets the flag (for example execve wants to
 * know the name of the program that is being executed), then it must
 * free the buffer. If VOP_LOOKUP sets the flag, then the buffer must
 * be freed by either the commit routine or the VOP_ABORT routine.
 * SAVESTART is set only by the callers of namei. It implies SAVENAME
 * plus the addition of saving the parent directory that contains the
 * name in ni_startdir. It allows repeated calls to lookup for the
 * name being sought. The caller is responsible for releasing the
 * buffer and for vrele'ing ni_startdir.
 */
#define	NOCROSSMOUNT	0x00100	/* do not cross mount points */
#define	RDONLY			0x00200	/* lookup with read-only semantics */
#define	HASBUF			0x00400	/* has allocated pathname buffer */
#define	SAVENAME		0x00800	/* save pathanme buffer */
#define	SAVESTART		0x01000	/* save starting directory */
#define ISDOTDOT		0x02000	/* current component name is .. */
#define MAKEENTRY		0x04000	/* entry is to be added to name cache */
#define ISLASTCN		0x08000	/* this is last component of pathname */
#define ISSYMLINK		0x10000	/* symlink needs interpretation */
#define	ISWHITEOUT		0x20000	/* found whiteout */
#define	DOWHITEOUT		0x40000	/* do whiteouts */
#define PARAMASK		0xfff00	/* mask of parameter descriptors */

#define NDINIT(ndp, op, flags, segflg, namep, p) { 	\
	(ndp)->ni_cnd.cn_nameiop = op; 					\
	(ndp)->ni_cnd.cn_flags = flags; 				\
	(ndp)->ni_segflg = segflg; 						\
	(ndp)->ni_dirp = namep; 						\
	(ndp)->ni_cnd.cn_proc = p; 						\
}
#endif

/*
 * This structure describes the elements in the cache of recent
 * names looked up by namei.
 */

#define	NCHNAMLEN	31	/* maximum name segment length we bother with */

struct	namecache {
	/* 2.11BSD hash chain */
	struct	namecache 		*nc_forw;			/* hash chain, MUST BE FIRST */
	struct	namecache 		*nc_back;			/* hash chain, MUST BE FIRST */
	struct	namecache 		*nc_nxt;			/* LRU chain */
	struct	namecache 		**nc_prev;			/* LRU chain */

	/* 4.4BSD hash chain */
	LIST_ENTRY(namecache) 	nc_hash;			/* hash chain */
	TAILQ_ENTRY(namecache) 	nc_lru;				/* LRU chain */
	struct	vnode 	  		*nc_dvp;			/* vnode of parent of name */
	u_long					nc_dvpid;			/* capability number of nc_dvp */
	struct	vnode 	  		*nc_vp;				/* vnode the name refers to */
	u_long					nc_vpid;			/* capability number of nc_vp */
	char					nc_nlen;			/* length of name */
	char					nc_name[NCHNAMLEN];	/* segment name */
};

#ifdef KERNEL
u_long	nextvnodeid;
struct namecache 	*namecache;
int	namei (struct nameidata *ndp);
int	lookup (struct nameidata *ndp);
int	relookup (struct vnode *dvp, struct vnode **vpp, struct componentname *cnp);
#endif

/*
 * Stats on usefulness of namei caches.
 */
struct	nchstats {
	long	ncs_goodhits;		/* hits that we can reall use */
	long	ncs_badhits;		/* hits we must drop */
	long	ncs_falsehits;		/* hits with id mismatch */
	long	ncs_miss;			/* misses */
	long	ncs_long;			/* long names that ignore cache */
	long	ncs_pass2;			/* names found with passes == 2 */
	long	ncs_2passes;		/* number of times we attempt it */
};
#endif
