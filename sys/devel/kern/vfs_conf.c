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
#include <devel/sys/vnodeopv.h>

struct vnodeopv_desc_list vfs_opv_descs = LIST_HEAD_INITIALIZER(vfs_opv_descs);

VNODEOPV_DESC_STRUCT(ffs, vnodeops);
VNODEOPV_DESC_STRUCT(ffs, specops);
VNODEOPV_DESC_STRUCT(ffs, fifoops);
VNODEOPV_DESC_STRUCT(lfs, vnodeops);
VNODEOPV_DESC_STRUCT(lfs, specops);
VNODEOPV_DESC_STRUCT(lfs, fifoops);
VNODEOPV_DESC_STRUCT(mfs, vnodeops);
VNODEOPV_DESC_STRUCT(ufs211, vnodeops);
VNODEOPV_DESC_STRUCT(ufs211, specops);
VNODEOPV_DESC_STRUCT(ufs211, fifoops);
VNODEOPV_DESC_STRUCT(cd9660, vnodeops);
VNODEOPV_DESC_STRUCT(cd9660, specops);
VNODEOPV_DESC_STRUCT(cd9660, fifoops);
VNODEOPV_DESC_STRUCT(msdosfs, vnodeops);
VNODEOPV_DESC_STRUCT(lofs, vnodeops);
VNODEOPV_DESC_STRUCT(fdesc, vnodeops);
VNODEOPV_DESC_STRUCT(nfs, vnodeops);
VNODEOPV_DESC_STRUCT(nfs, specops);
VNODEOPV_DESC_STRUCT(nfs, fifoops);
VNODEOPV_DESC_STRUCT(ufml, vnodeops);
VNODEOPV_DESC_STRUCT(union, vnodeops);

void
vfs_opv_init()
{
	/* Fast Filesystem */
	vnodeopv_desc_create(&ffs_vnodeops_opv_desc, "ffs", D_VNODEOPS, &ffs_vnodeops);
	vnodeopv_desc_create(&ffs_specops_opv_desc, "ffs", D_SPECOPS, &ffs_specops);
	vnodeopv_desc_create(&ffs_fifoops_opv_desc, "ffs", D_FIFOOPS, &ffs_fifoops);
	/* Log-based Filesystem */
	vnodeopv_desc_create(&lfs_vnodeops_opv_desc, "lfs", D_VNODEOPS, &lfs_vnodeops);
	vnodeopv_desc_create(&lfs_specops_opv_desc, "lfs", D_SPECOPS, &lfs_specops);
	vnodeopv_desc_create(&lfs_fifoops_opv_desc, "lfs", D_FIFOOPS, &lfs_fifoops);
	/* Memory-based Filesystem */
	vnodeopv_desc_create(&mfs_vnodeops_opv_desc, "mfs", D_VNODEOPS, &mfs_vnodeops);
	/* 2.11BSD UFS Filesystem  */
	vnodeopv_desc_create(&ufs211_vnodeops_opv_desc, "ufs211", D_VNODEOPS, &ufs211_vnodeops);
	vnodeopv_desc_create(&ufs211_specops_opv_desc, "ufs211", D_SPECOPS, &ufs211_specops);
	vnodeopv_desc_create(&ufs211_fifoops_opv_desc, "ufs211", D_FIFOOPS, &ufs211_fifoops);
	/* ISO9660 (aka CDROM) Filesystem */
	vnodeopv_desc_create(&cd9660_vnodeops_opv_desc, "cd9660", D_VNODEOPS, &cd9660_vnodeops);
	vnodeopv_desc_create(&cd9660_specops_opv_desc, "cd9660", D_SPECOPS, &cd9660_specops);
	vnodeopv_desc_create(&cd9660_fifoops_opv_desc, "cd9660", D_FIFOOPS, &cd9660_fifoops);
	/* MSDOS Filesystem */
	vnodeopv_desc_create(&msdosfs_vnodeops_opv_desc, "msdos", D_VNODEOPS, &msdosfs_vnodeops);
	/* Loopback Filesystem */
	vnodeopv_desc_create(&lofs_vnodeops_opv_desc, "loopback", D_VNODEOPS, &lofs_vnodeops);
	/* File Descriptor Filesystem */
	vnodeopv_desc_create(&fdesc_vnodeops_opv_desc, "fdesc", D_VNODEOPS, &fdesc_vnodeops);
	/* Sun-compatible Network Filesystem */
	vnodeopv_desc_create(&nfs_vnodeops_opv_desc, "nfs", D_VNODEOPS, &nfs_vnodeops);
	vnodeopv_desc_create(&nfs_specops_opv_desc, "nfs", D_SPECOPS, &nfs_specops);
	vnodeopv_desc_create(&nfs_fifoops_opv_desc, "nfs", D_FIFOOPS, &nfs_fifoops);
	/* UFML Filesystem  */
	vnodeopv_desc_create(&ufml_vnodeops_opv_desc, "ufml", D_VNODEOPS, &ufml_vnodeops);
	/* Union (translucent) Filesystem */
	vnodeopv_desc_create(&union_vnodeops_opv_desc, "union", D_VNODEOPS, &union_vnodeops);
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
vnodeopv_desc_lookup(copv, fsname, voptype)
	struct vnodeopv_desc *copv;
	const char *fsname;
    int voptype;
{
    register struct vnodeopv_desc *opv;
    LIST_FOREACH(opv, &vfs_opv_descs, opv_entry) {
    	if ((copv != NULL && copv == opv) || ((strcmp(opv->opv_fsname, fsname) == 0) && ((opv->opv_voptype == voptype) || (opv->opv_voptype == D_NOOPS)))) {
    		return (opv);
    	}
    }
    return (NULL);
}

union vnodeopv_entry_desc
vnodeopv_entry_desc(copv, fsname, voptype)
	struct vnodeopv_desc *copv;
	const char *fsname;
	int voptype;
{
	return (vnodeopv_desc_lookup(copv, fsname, voptype)->opv_desc_ops);
}

struct vnodeops *
vnodeopv_entry_desc_get_vnodeops(copv, fsname, voptype)
	struct vnodeopv_desc *copv;
	const char *fsname;
	int voptype;
{
	struct vnodeops *v;
	v = vnodeopv_entry_desc(copv, fsname, voptype).opve_vops;
	return (v);
}

struct vnodeop_desc *
vnodeopv_entry_desc_get_vnodeop_desc(copv, fsname, voptype)
	struct vnodeopv_desc *copv;
	const char *fsname;
	int voptype;
{
	struct vnodeop_desc *v;
	v = vnodeopv_entry_desc(copv, fsname, voptype).opve_op;
	return (v);
}

