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

/*
 * TODO: change following to work with list
 * - vfs_syscalls.c: mount
 * - vfs_subr.c: vfs_rootmountalloc, vfs_mountroot, vfs_sysctl
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <devel/sys/vfs_vdesc.h>

LIST_HEAD(, vfsconf) vfslist = LIST_HEAD_INITIALIZER(vfslist);

void
vfsops_init(vfsp)
	struct vfsconf *vfsp;
{
	(*vfsp->vfc_vfsops->vfs_init)(vfsp);
}

void
vfsconf_init(vfsp)
	struct vfsconf *vfsp;
{
	int i, maxtypenum;

	maxtypenum = 0;
	for(i = 0; i < maxvfsconf; i++) {
		vfsconf_attach(&vfsp[i]);
		if (maxtypenum <= &vfsp[i].vfc_typenum) {
			maxtypenum = &vfsp[i].vfc_typenum + 1;
		}
		vfsops_init(&vfsp[i]);
	}

	/* next vfc_typenum to be used */
	maxvfsconf = maxtypenum;
}

void
vfsconf_attach(vfsp)
	struct vfsconf *vfsp;
{
	if (vfsp != NULL) {
		LIST_INSERT_HEAD(&vfslist, vfsp, vfc_entry);
	}
}

void
vfsconf_detach(vfsp)
	struct vfsconf *vfsp;
{
	if(vfsp != NULL) {
		LIST_REMOVE(vfsp, vfc_entry);
	}
}

struct vfsconf *
vfsconf_find_by_name(name)
	const char *name;
{
	struct vfsconf *vfsp;
	LIST_FOREACH(vfsp, &vfslist, vfc_entry) {
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

	LIST_FOREACH(vfsp, &vfslist, vfc_entry) {
		if (typenum == vfsp->vfc_typenum)
			break;
	}
	return (vfsp);
}

struct vnodeopv_desc_list vfs_opv_descs = LIST_HEAD_INITIALIZER(vfs_opv_descs);

VNODEOPV_DESC_STRUCT(ffs, vnodeops);
VNODEOPV_DESC_STRUCT(ffs, specops);
VNODEOPV_DESC_STRUCT(ffs, fifoops);
VNODEOPV_DESC_STRUCT(lfs, vnodeops);
VNODEOPV_DESC_STRUCT(lfs, specops);
VNODEOPV_DESC_STRUCT(lfs, fifoops);
VNODEOPV_DESC_STRUCT(mfs, vnodeops);
VNODEOPV_DESC_STRUCT(mfs, specops);
VNODEOPV_DESC_STRUCT(mfs, fifoops);
VNODEOPV_DESC_STRUCT(ufs211, vnodeops);
VNODEOPV_DESC_STRUCT(ufs211, specops);
VNODEOPV_DESC_STRUCT(ufs211, fifoops);

void
vfs_opv_init()
{
	vnodeopv_desc_create(&ffs_vnodeops_opv_desc, "ffs", D_VNODEOPS, &ffs_vnodeops);
	vnodeopv_desc_create(&ffs_specops_opv_desc, "ffs", D_SPECOPS, &ffs_specops);
	vnodeopv_desc_create(&ffs_fifoops_opv_desc, "ffs", D_FIFOOPS, &ffs_fifoops);

	vnodeopv_desc_create(&lfs_vnodeops_opv_desc, "lfs", D_VNODEOPS, &lfs_vnodeops);
	vnodeopv_desc_create(&lfs_specops_opv_desc, "lfs", D_SPECOPS, &lfs_specops);
	vnodeopv_desc_create(&lfs_fifoops_opv_desc, "lfs", D_FIFOOPS, &lfs_fifoops);

	vnodeopv_desc_create(&mfs_vnodeops_opv_desc, "mfs", D_VNODEOPS, &mfs_vnodeops);
	vnodeopv_desc_create(&mfs_specops_opv_desc, "mfs", D_SPECOPS, &mfs_specops);
	vnodeopv_desc_create(&mfs_fifoops_opv_desc, "mfs", D_FIFOOPS, &mfs_fifoops);

	vnodeopv_desc_create(&ufs211_vnodeops_opv_desc, "ufs211", D_VNODEOPS, &ufs211_vnodeops);
	vnodeopv_desc_create(&ufs211_specops_opv_desc, "ufs211", D_SPECOPS, &ufs211_specops);
	vnodeopv_desc_create(&ufs211_fifoops_opv_desc, "ufs211", D_FIFOOPS, &ufs211_fifoops);
}

void
vnodeopv_desc_create(opv, fsname, voptype, vops, op)
	struct vnodeopv_desc    *opv;
    const char              *fsname;
    int                     voptype;
    struct vnodeops         *vops;
    struct vnodeop_desc 	*op;
{
    opv->opv_fsname = fsname;
    opv->opv_voptype = voptype;
    opv->opv_desc_ops.opve_vops = vops;
    opv->opv_desc_ops.opve_op = op;
    LIST_INSERT_HEAD(&vfs_opv_descs, opv, opv_entry);
}

struct vnodeopv_desc *
vnodeopv_desc_lookup(fsname, voptype)
	const char *fsname;
    int voptype;
{
    struct vnodeopv_desc *opv;
    LIST_FOREACH(opv, &vfs_opv_descs, opv_entry) {
    	if ((strcmp(opv->opv_fsname, fsname) == 0) && (opv->opv_voptype == voptype)) {
    		return (opv);
    	}
    }
    return (NULL);
}

struct vnodeops *
vnodeopv_desc_get_vnodeops(fsname, voptype)
	const char *fsname;
	int voptype;
{
	struct vnodeops *v;
	v = vnodeopv_desc_lookup(fsname, voptype)->opv_desc_ops.opve_vops;
	return (v);
}

struct vnodeop_desc *
vnodeopv_desc_get_vnodeop_desc(fsname, voptype)
	const char *fsname;
	int voptype;
{
	struct vnodeop_desc *v;
	v = vnodeopv_desc_lookup(fsname, voptype)->opv_desc_ops.opve_op;
	return (v);
}
