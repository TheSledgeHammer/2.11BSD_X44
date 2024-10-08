/*
 * Copyright (c) 1992 The Regents of the University of California
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
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
 *	@(#)lofs.h	7.1 (Berkeley) 7/12/92
 *
 * $Id: lofs.h,v 1.8 1992/05/30 10:05:43 jsp Exp jsp $
 */

#ifndef _LOFS_H_
#define _LOFS_H_

#include <sys/queue.h>

struct lofs_args {
	char				*target;	/* Target of loopback  */
};

struct lofsmount {
	struct mount		*looped_vfs;
	struct vnode		*rootvp;	/* Reference to root lofsnode */
};

#ifdef _KERNEL
/*
 * A cache of vnode references
 */
struct lofsnode {
	LIST_ENTRY(lofsnode) a_hash;	/* Hash chain */
	struct vnode		*a_lofsvp;	/* Aliased vnode - VREFed once */
	struct vnode		*a_vnode;	/* Back pointer to vnode/lofsnode */
};

extern int make_lofs(struct mount *mp, struct vnode **vpp);

#define	VFSTOLOFS(mp) 	((struct lofsmount *)((mp)->mnt_data))
#define	VTOLOFS(vp) 	((struct lofsnode *)(vp)->v_data)
#define LOFSTOV(a)		((a)->a_vnode)
#define	LOFSP(vp) 		VTOLOFS(vp)
#ifdef LOFS_DIAGNOSTIC
extern struct vnode 	*lofs_checkvp (struct vnode *vp, char *fil, int lno);
#define	LOFSVP(vp) 		lofs_checkvp(vp, __FILE__, __LINE__)

#else
#define	LOFSVP(vp) 		(LOFSP(vp)->a_lofsvp)
#endif

extern struct vnodeops 	lofs_vnodeops;
extern struct vfsops 	lofs_vfsops;

#endif /* KERNEL */
#endif /* _LOFS_H_ */
