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
	char			*target;		/* Target of loopback  */
};

struct ufml_mount {
	struct mount	*ufmlm_vfs;
	struct vnode	*ufmlm_rootvp;	/* Reference to root ufml_node */
};

//#ifdef KERNEL
/*
 * A cache of vnode references
 */
struct ufml_node {
	LIST_ENTRY(ufml_node)	ufml_hash;		/* Hash list */
	struct vnode	        *ufml_lowervp;	/* VREFed once */
	struct vnode			*ufml_vnode;	/* Back pointer */

	struct ufml_metadata	*ufml_meta;		/* Node metadata */
	struct ufmlops			*ufml_op		/* UFML operations vector */
};

extern int ufml_node_create (struct mount *mp, struct vnode *target, struct vnode **vpp);

#define	MOUNTTOUFMLMOUNT(mp) 	((struct ufml_mount *)((mp)->mnt_data))
#define	VTOUFML(vp) 			((struct ufml_node *)(vp)->v_data)
#define	UFMLTOV(xp) 			((xp)->ufml_vnode)
//#define UFMLTOMETA(mp)			((mp)->ufml_meta)
#ifdef UFMLFS_DIAGNOSTIC
extern struct vnode *ufml_checkvp (struct vnode *vp, char *fil, int lno);
#define	UFMLVPTOLOWERVP(vp) 	ufml_checkvp((vp), __FILE__, __LINE__)
#else
#define	UFMLVPTOLOWERVP(vp) 	(VTOUFML(vp)->ufml_lowervp)
#endif
extern struct vnodeops ufml_vnodeops;
extern struct vfsops ufml_vfsops;

#endif /* KERNEL */


