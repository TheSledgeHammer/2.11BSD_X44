/*
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994 Jan-Simon Pendry.
 * All rights reserved.
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
 *	@(#)union.h	8.9 (Berkeley) 12/10/94
 */

#ifndef _UFML_UNION_H_
#define _UFML_UNION_H_

#include <ufml.h>

#define UNMNT_ABOVE			0x0001		/* Target appears below mount point */
#define UNMNT_BELOW			0x0002		/* Target appears below mount point */
#define UNMNT_REPLACE		0x0003		/* Target replaces mount point */
#define UNMNT_OPMASK		0x0003

struct ufml_union_mount {
	struct vnode			*uum_uppervp;
	struct vnode			*uum_lowervp;

	struct ucred			*uum_cred;	/* Credentials of user calling mount */
	int						uum_cmode;	/* cmask from mount process */
	int						uum_op;		/* Operation mode */
};

/*
 * DEFDIRMODE is the mode bits used to create a shadow directory.
 */
#define VRWXMODE 			(VREAD|VWRITE|VEXEC)
#define VRWMODE 			(VREAD|VWRITE)
#define UN_DIRMODE 			((VRWXMODE)|(VRWXMODE>>3)|(VRWXMODE>>6))
#define UN_FILEMODE 		((VRWMODE)|(VRWMODE>>3)|(VRWMODE>>6))

struct ufml_union_node {
	struct vnode	        *uun_uppervp;	/* overlaying object */
	struct vnode	        *uun_lowervp;	/* underlying object */

	struct vnode			*uun_dirvp;		/* Parent dir of uppervp */
	struct vnode			*uun_pvp;		/* Parent vnode */
	char					*uun_path;		/* saved component name */
	int						uun_hash;		/* saved un_path hash value */
	int						uun_openl;		/* # of opens on lowervp */
	unsigned int			uun_flags;
	struct vnode			**uun_dircache; /* cached union stack */
	off_t					uun_uppersz;	/* size of upper object */
	off_t					uun_lowersz;	/* size of lower object */
#ifdef DIAGNOSTIC
	pid_t					uun_pid;
#endif
};

#define UFML_WANT			0x01
#define UFML_LOCKED			0x02
#define UFML_ULOCK			0x04			/* Upper node is locked */
#define UFML_KLOCK			0x08			/* Keep upper node locked on vput */
#define UFML_CACHED			0x10			/* In union cache */

#define UFML_UNION(xp)		(xp)->ufml_union
#define UFML_UNIONMNT(xp)	(xp)->ufmlm_unionmnt
#define	LOWERVP(vp) 		(VTOUFML(vp)->ufml_union->uun_lowervp)
#define	UPPERVP(vp) 		(VTOUFML(vp)->ufml_union->uun_uppervp)
#define OTHERVP(vp) 		(UPPERVP(vp) ? UPPERVP(vp) : LOWERVP(vp))

extern int 					ufml_allocvp (struct vnode **, struct mount *, struct vnode *, struct vnode *, struct componentname *, struct vnode *, struct vnode *, int);
extern int 					ufml_copyfile (struct vnode *, struct vnode *, struct ucred *, struct proc *);
extern int 					ufml_copyup (struct ufml_node *, int, struct ucred *, struct proc *);
extern int 					ufml_dowhiteout (struct ufml_node *, struct ucred *, struct proc *);
extern int 					ufml_mkshadow (struct ufml_node *, struct vnode *, struct componentname *, struct vnode **);
extern int 					ufml_mkwhiteout (struct ufml_node *, struct vnode *, struct componentname *, char *);
extern int 					ufml_vn_create (struct vnode **, struct ufml_node *, struct proc *);
extern int 					ufml_cn_close (struct vnode *, int, struct ucred *, struct proc *);
extern void 				ufml_removed_upper (struct ufml_node *);
extern struct vnode 		*ufml_lowervp (struct vnode *);
extern void 				ufml_newlower (struct ufml_node *, struct vnode *);
extern void 				ufml_newupper (struct ufml_node *, struct vnode *);
extern void 				ufml_newsize (struct vnode *, off_t, off_t);

#endif /* _UFML_UNION_H_ */
