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
#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/vnode_if.h>
#include <devel/sys/vnodeopv.h>

struct vnodeopv_desc_list vfs_opv_descs;
struct vattr va_null;

int vfs_opv_numops;

void
vfs_opv_init()
{
	int i, j, k;
	int (***opv_desc_vector_p)();
	int (**opv_desc_vector)();

	struct vnodeopv_entry_desc 	*opve_descp;

	register struct vnodeopv_desc *opv;
	LIST_FOREACH(opv, &vfs_opv_descs, opv_entry) {
		opv_desc_vector_p = opv->opv_desc_vector_p;
		if (*opv_desc_vector_p == NULL) {
			*opv_desc_vector_p = (opv_desc_vector_t)calloc(vfs_opv_numops, sizeof(pfi_t), M_VNODE, M_WAITOK);
			bzero (*opv_desc_vector_p, vfs_opv_numops * sizeof(pfi_t));
		}
		opv_desc_vector = *opv_desc_vector_p;
		opve_descp = opv->opv_desc_ops;
		if (opve_descp->opve_op->vdesc_offset == 0 && opve_descp->opve_op->vdesc_offset != VOFFSET(vop_default)) {
			printf("operation %s not listed in %s.\n", opve_descp->opve_op->vdesc_name, "vfs_op_descs");
			panic("vfs_opv_init: bad operation");
			opv_desc_vector[opve_descp->opve_op->vdesc_offset] = opve_descp->opve_impl;
		}
	}

	LIST_FOREACH(opv, &vfs_opv_descs, opv_entry) {
		opv_desc_vector = *opv->opv_desc_vector_p;
		/*
		 * Force every operations vector to have a default routine.
		 */
		if (opv_desc_vector[VOFFSET(vop_default)] == NULL) {
			panic("vfs_opv_init: operation vector without default routine.");
		}
		for (k = 0; k < vfs_opv_numops; k++) {
			if (opv_desc_vector[k] == NULL) {
				opv_desc_vector[k] = opv_desc_vector[VOFFSET(vop_default)];
			}
		}
	}
}
