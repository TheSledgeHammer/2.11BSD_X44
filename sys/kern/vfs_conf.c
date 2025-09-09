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
#include <sys/queue.h>
#include <sys/null.h>
#include <sys/user.h>

/*
 * These define the root filesystem, device, and root filesystem type.
 */
struct mount *rootfs;
struct vnode *rootvnode;
//int (*mountroot)() = NULL;

/*
 * Set up the initial array of known filesystem types.
 */
extern	struct vfsops ufs_vfsops;
extern	int ffs_mountroot(void);
extern	struct vfsops lfs_vfsops;
extern	int lfs_mountroot(void);
extern	struct vfsops mfs_vfsops;
extern	struct vfsops ufs211_vfsops;
extern	int mfs_mountroot(void);
extern	struct vfsops cd9660_vfsops;
extern	int cd9660_mountroot(void);
extern	struct vfsops msdosfs_vfsops;
extern	struct vfsops lofs_vfsops;
extern	struct vfsops nfs_vfsops;
extern	int nfs_mountroot(void);
extern	struct vfsops ufml_vfsops;
extern	struct vfsops union_vfsops;

/*
 * Set up the filesystem operations for vnodes.
 */
void
vfsconf_fs_init(void)
{
	/* Fast Filesystem */
#ifdef FFS
    	vfsconf_fs_create(&ufs_vfsops, "ufs", VT_UFS, 0, MNT_LOCAL, ffs_mountroot);
#endif

	/* Log-based Filesystem */
#ifdef LFS
    	vfsconf_fs_create(&lfs_vfsops, "lfs", VT_LFS, 0, MNT_LOCAL, lfs_mountroot);
#endif

	/* Memory-based Filesystem */
#ifdef MFS
    	vfsconf_fs_create(&mfs_vfsops, "mfs", VT_MFS, 0, MNT_LOCAL, mfs_mountroot);
#endif

	/* 2.11BSD UFS Filesystem  */
#ifdef UFS211
    vfsconf_fs_create(&ufs211_vfsops, "ufs211", VT_UFS211, 0, MNT_LOCAL, NULL);
#endif

	/* ISO9660 (aka CDROM) Filesystem */
#ifdef CD9660
    	vfsconf_fs_create(&cd9660_vfsops, "cd9660", VT_ISOFS, 0, MNT_LOCAL, cd9660_mountroot);
#endif

	/* MSDOS Filesystem */
#ifdef MSDOS
	vfsconf_fs_create(&msdosfs_vfsops, "msdos", VT_MSDOSFS, 0, MNT_LOCAL, NULL);
#endif

	/* Loopback Filesystem */
#ifdef LOFS
	vfsconf_fs_create(&lofs_vfsops, "loopback", VT_LOFS, 0, 0, NULL);
#endif

	/* File Descriptor Filesystem */
#ifdef FDESC
	vfsconf_fs_create(&fdesc_vfsops, "fdesc", VT_FDESC, 0, 0, NULL);
#endif

	/* Sun-compatible Network Filesystem */
#ifdef NFS
	vfsconf_fs_create(&nfs_vfsops, "nfs", VT_NFS, 0, 0, nfs_mountroot);
#endif

	/* UFML Filesystem  */
#ifdef UFML
	vfsconf_fs_create(&ufml_vfsops, "ufml", VT_UFML, 0, 0, NULL);
#endif

	/* Union (translucent) Filesystem */
#ifdef UNION
	vfsconf_fs_create(&union_vfsops, "union", VT_UNION, 0, 0, NULL);
#endif
}

/*
 * Initially the size of the list, vfs_init will set maxvfsconf
 * to the highest defined type number.
 */
static struct vfsconf vfssw;
struct vfs_list vfsconflist = LIST_HEAD_INITIALIZER(vfsconflist);
int maxvfsconf = 0;

void
vfsconf_fs_create(vfsop, name, index, typenum, flags, mountroot)
    struct vfsops *vfsop;
    const char *name;
    int index, typenum, flags;
    int (*mountroot)(void);
{   	
    register struct vfsconf *vfsp;
	
    vfsp = &vfssw;
    vfsp->vfc_vfsops = vfsop;
    vfsp->vfc_name =  name;
    vfsp->vfc_index = index;
    vfsp->vfc_typenum = typenum;
    vfsp->vfc_flags = flags;
    vfsp->vfc_mountroot = mountroot;

    vfsconf_attach(vfsp);
}

struct vfsconf *
vfsconf_find_by_name(name)
	const char *name;
{
	struct vfsconf *vfsp;
	LIST_FOREACH(vfsp, &vfsconflist, vfc_next) {
		if (strcmp(name, vfsp->vfc_name) == 0) {
			break;
		}
	}
	return (vfsp);
}

struct vfsconf *
vfsconf_find_by_typenum(typenum)
	int typenum;
{
	struct vfsconf *vfsp;

	LIST_FOREACH(vfsp, &vfsconflist, vfc_next) {
		if (typenum == vfsp->vfc_typenum) {
			break;
		}
	}
	return (vfsp);
}

void
vfsconf_attach(vfsp)
	struct vfsconf *vfsp;
{
	if (vfsp != NULL) {
		LIST_INSERT_HEAD(&vfsconflist, vfsp, vfc_next);
		maxvfsconf++;
	}
}

void
vfsconf_detach(vfsp)
	struct vfsconf *vfsp;
{
	if(vfsp != NULL) {
		LIST_REMOVE(vfsp, vfc_next);
		maxvfsconf--;
	}
}
