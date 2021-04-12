/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)null.h	8.3 (Berkeley) 8/20/94
 *
 * $Id: lofs.h,v 1.8 1992/05/30 10:05:43 jsp Exp jsp $
 */

#include <sys/queue.h>

struct ufml_args {
	char					*target;		/* Target of loopback  */
	int						mntflags;		/* Options on the mount */
};

#define UNMNT_ABOVE			0x0001		/* Target appears below mount point */
#define UNMNT_BELOW			0x0002		/* Target appears below mount point */
#define UNMNT_REPLACE		0x0003		/* Target replaces mount point */
#define UNMNT_OPMASK		0x0003

struct ufml_mount {
	struct vnode			*ufmlm_uppervp;
	struct vnode			*ufmlm_lowervp;

	struct mount			*ufmlm_vfs;
	struct vnode			*ufmlm_rootvp;	/* Reference to root ufml_node */

	struct ucred			*ufmlm_cred;	/* Credentials of user calling mount */
	int						ufmlm_cmode;	/* cmask from mount process */
	int						ufmlm_op;		/* Operation mode */
};

/*
 * DEFDIRMODE is the mode bits used to create a shadow directory.
 */
#define VRWXMODE 			(VREAD|VWRITE|VEXEC)
#define VRWMODE 			(VREAD|VWRITE)
#define UN_DIRMODE 			((VRWXMODE)|(VRWXMODE>>3)|(VRWXMODE>>6))
#define UN_FILEMODE 		((VRWMODE)|(VRWMODE>>3)|(VRWMODE>>6))

#ifdef KERNEL
/*
 * A cache of vnode references
 */
struct ufml_node {
	LIST_ENTRY(ufml_node)	ufml_cache;		/* Hash list */
	struct vnode			*ufml_vnode;	/* Back pointer */

	struct vnode	        *ufml_uppervp;	/* overlaying object */
	struct vnode	        *ufml_lowervp;	/* underlying object */
	struct vnode			*ufml_dirvp;	/* Parent dir of uppervp */
	struct vnode			*ufml_pvp;		/* Parent vnode */
	char					*ufml_path;		/* saved component name */
	int						ufml_hash;		/* saved un_path hash value */
	int						ufml_openl;		/* # of opens on lowervp */
	unsigned int			ufml_flags;
	struct vnode			**ufml_dircache;/* cached union stack */
	off_t					ufml_uppersz;	/* size of upper object */
	off_t					ufml_lowersz;	/* size of lower object */

	struct ufml_metadata	*ufml_meta;		/* Node metadata (from ufmlops) */
	struct ufmlops			*ufml_op;		/* UFML operations vector */
#ifdef DIAGNOSTIC
	pid_t					ufml_pid;
#endif
};

#define UFML_WANT			0x01
#define UFML_LOCKED			0x02
#define UFML_ULOCK			0x04			/* Upper node is locked */
#define UFML_KLOCK			0x08			/* Keep upper node locked on vput */
#define UFML_CACHED			0x10			/* In union cache */

extern int 			ufml_allocvp (struct vnode **, struct mount *, struct vnode *, struct vnode *, struct componentname *, struct vnode *, struct vnode *, int);
extern int 			ufml_copyfile (struct vnode *, struct vnode *, struct ucred *, struct proc *);
extern int 			ufml_copyup (struct ufml_node *, int, struct ucred *, struct proc *);
extern int 			ufml_dowhiteout (struct ufml_node *, struct ucred *, struct proc *);
extern int 			ufml_mkshadow (struct ufml_node *, struct vnode *, struct componentname *, struct vnode **);
extern int 			ufml_mkwhiteout (struct ufml_node *, struct vnode *, struct componentname *, char *);
extern int 			ufml_vn_create (struct vnode **, struct ufml_node *, struct proc *);
extern int 			ufml_cn_close (struct vnode *, int, struct ucred *, struct proc *);
extern void 		ufml_removed_upper (struct ufml_node *);
extern struct vnode *ufml_lowervp (struct vnode *);
extern void 		ufml_newlower (struct ufml_node *, struct vnode *);
extern void 		ufml_newupper (struct ufml_node *, struct vnode *);
extern void 		ufml_newsize (struct vnode *, off_t, off_t);

extern int 			ufml_node_create (struct mount *mp, struct vnode *target, struct vnode **vpp);

#define	MOUNTTOUFMLMOUNT(mp) 	((struct ufml_mount *)((mp)->mnt_data))
#define	VTOUFML(vp) 			((struct ufml_node *)(vp)->v_data)
#define	UFMLTOV(xp) 			((xp)->ufml_vnode)
#define	LOWERVP(vp) 			(VTOUFML(vp)->un_lowervp)
#define	UPPERVP(vp) 			(VTOUFML(vp)->un_uppervp)
#define OTHERVP(vp) 			(UPPERVP(vp) ? UPPERVP(vp) : LOWERVP(vp))

#ifdef UFMLFS_DIAGNOSTIC
extern struct vnode *ufml_checkvp (struct vnode *vp, char *fil, int lno);
#define	UFMLVPTOLOWERVP(vp) 	ufml_checkvp((vp), __FILE__, __LINE__)
#else
#define	UFMLVPTOLOWERVP(vp) 	(VTOUFML(vp)->ufml_lowervp)
#endif
extern struct vnodeops 	ufml_vnodeops;
extern struct vfsops 	ufml_vfsops;

#endif /* KERNEL */


