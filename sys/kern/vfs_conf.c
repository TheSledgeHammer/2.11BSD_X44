/*
 * Copyright (c) 1989, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)vfs_conf.c	8.11 (Berkeley) 5/10/95
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/user.h>

/*
 * These define the root filesystem, device, and root filesystem type.
 */
struct mount *rootfs;
struct vnode *rootvnode;
int (*mountroot)() = NULL;

/*
 * Set up the initial array of known filesystem types.
 */
extern	struct vfsops ufs_vfsops;
extern	int ffs_mountroot();
extern	struct vfsops lfs_vfsops;
extern	int lfs_mountroot();
extern	struct vfsops mfs_vfsops;
extern	int mfs_mountroot();
extern	struct vfsops cd9660_vfsops;
extern	int cd9660_mountroot();
extern	struct vfsops msdos_vfsops;
extern	struct vfsops lofs_vfsops;
extern	struct vfsops nfs_vfsops;
//extern	int nfs_mountroot();
extern	struct vfsops ufml_vfsops;
extern	struct vfsops ufs211_vfsops;

/*
 * Set up the filesystem operations for vnodes.
 */
static struct vfsconf vfsconflist[] = {

	/* Fast Filesystem */
#ifdef FFS
	{ &ufs_vfsops, "ufs", 1, 0, MNT_LOCAL, ffs_mountroot, NULL },
#endif

	/* Log-based Filesystem */
#ifdef LFS
	{ &lfs_vfsops, "lfs", 5, 0, MNT_LOCAL, lfs_mountroot, NULL },
#endif

	/* Memory-based Filesystem */
#ifdef MFS
	{ &mfs_vfsops, "mfs", 3, 0, MNT_LOCAL, mfs_mountroot, NULL },
#endif

	/* ISO9660 (aka CDROM) Filesystem */
#ifdef CD9660
	{ &cd9660_vfsops, "cd9660", 14, 0, MNT_LOCAL, cd9660_mountroot, NULL },
#endif

	/* MSDOS Filesystem */
#ifdef MSDOS
	{ &msdos_vfsops, "msdos", 4, 0, MNT_LOCAL, NULL, NULL },
#endif

	/* Loopback Filesystem */
#ifdef LOFS
	{ &lofs_vfsops, "loopback", 6, 0, 0, NULL, NULL },
#endif

	/* File Descriptor Filesystem */
#ifdef FDESC
	{ &fdesc_vfsops, "fdesc", 7, 0, 0, NULL, NULL },
#endif

	/* Sun-compatible Network Filesystem */
#ifdef NFS
	{ &nfs_vfsops, "nfs", 2, 0, 0, nfs_mountroot, NULL },
#endif

	/* UFML Filesystem  */
#ifdef UFML
	{ &ufml_vfsops, "ufml", 16, 0, 0, NULL, NULL }
#endif

	/* 2.11BSD UFS Filesystem  */
#ifdef UFS211
	{ &ufs211_vfsops, "ufs211", 17, 0, MNT_LOCAL, NULL, NULL }
#endif
};

/*
 * Initially the size of the list, vfs_init will set maxvfsconf
 * to the highest defined type number.
 */
int maxvfsconf = sizeof(vfsconflist) / sizeof (struct vfsconf);
struct vfsconf *vfsconf = vfsconflist;
