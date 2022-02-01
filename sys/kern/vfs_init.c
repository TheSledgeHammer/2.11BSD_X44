/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed
 * to Berkeley by John Heidemann of the UCLA Ficus project.
 *
 * Source: * @(#)i405_init.c 2.10 92/04/27 UCLA Ficus project
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
 *	@(#)vfs_init.c	8.5 (Berkeley) 5/11/95
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/malloc.h>

#if 0
#define DODEBUG(A) A
#else
#define DODEBUG(A)
#endif

extern struct vnodeop_desc *vfs_op_descs[]; /* and the operations they perform */
int vfs_opv_numops;

/*
 * Initialize known vnode operations vectors.
 */
void
vfs_op_init()
{
	int i;

	DODEBUG(printf("Vnode_interface_init.\n"));

	/*
	 * Figure out how many ops there are by counting the table,
	 * and assign each its offset.
	 */
	for (vfs_opv_numops = 0, i = 0; vfs_op_descs[i]; i++) {
		vfs_op_descs[i].vdesc_offset = vfs_opv_numops;
		vfs_opv_numops++;
	}
	DODEBUG(printf ("vfs_opv_numops=%d\n", vfs_opv_numops));
}

/*
 * Routines having to do with the management of the vnode table.
 */
struct vattr va_null;

/*
 * Initialize the vnode structures and initialize each file system type.
 */
void
vfsinit()
{
	struct vfsconf *vfsp;
	int i, maxtypenum;

	/*
	 * Initialize the vnode table
	 */
	vntblinit();
	/*
	 * Initialize the vnode name cache
	 */
	nchinit();
	/*
	 * Set up the filesystem operations for vnodes.
	 */
	vfsconf_fs_init();
	/*
	 * Build vnode operation vectors.
	 */
	vfs_op_init();
	/*
	 * Initialize each file system type.
	 */
	vattr_null(&va_null);
	maxtypenum = 0;
	LIST_FOREACH(vfsp, &vfsconflist, vfc_next) {
		if (maxtypenum <= vfsp->vfc_typenum) {
			maxtypenum = vfsp->vfc_typenum + 1;
		}
		(*vfsp->vfc_vfsops->vfs_init)(vfsp);
	}
	/* next vfc_typenum to be used */
	maxvfsconf = maxtypenum;
	/*
	 * Initialize the vnode advisory lock vfs_lockf.c
	 */
	lf_init();
}
