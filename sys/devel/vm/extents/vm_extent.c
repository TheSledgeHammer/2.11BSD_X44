/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "vm_extent.h"

#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/user.h>

struct vextops vextops;

/* initilize vextops */
void
vextops_init()
{
	vop_malloc(&vextops);
}

/* allocate vextops */
void
vextops_malloc(vextops)
	struct vextops *vextops;
{
	MALLOC(vextops, struct vextops *, sizeof(struct vextops *), M_VEXTOPS, M_WAITOK);
}

int
vm_extent_create(vext, ext, name, start, end, mtype, storage, storagesize, flags)
	struct vm_extent *vext;
	struct extent *ext;
	char *name;
	vm_offset_t start, end;
	int mtype, flags;
	caddr_t	storage;
	vm_size_t storagesize;
{
    struct vextops_create_args vap;
    int error;

    vap.a_head.a_ops = &vextops;
    vap.a_vext = vext;
    vap.a_name = name;
    vap.a_start = start;
    vap.a_end = end;
    vap.a_mtype = mtype;
    vap.a_storage = storage;
    vap.a_storagesize = storagesize;
    vap.a_flags = flags;

    if (vextops.vm_extent_create == NULL) {
    	return (EOPNOTSUPP);
    }

	vext->vext_ext = ext;
    error = vextops.vm_extent_create(ext, name, start, end, mtype, storage, storagesize, flags);

    return (error);
}

int
vm_extent_mallocok(vext, mallocok)
	struct vm_extent *vext;
	int mallocok;
{
	struct vextops_mallocok_args vap;
	int error;

	vap.a_head.a_ops = &vextops;
	vap.a_vext = vext;
	vap.a_mallocok = mallocok;

    if (vextops.vm_extent_mallocok == NULL) {
    	return (EOPNOTSUPP);
    }

	error = vextops.vm_extent_mallocok(vext, mallocok);

	if(mallocok != 0) {
		error = 1;
	}

	return (error);
}

int
vm_extent_alloc(vext, start, size, flags)
	struct vm_extent *vext;
	vm_offset_t start;
	vm_size_t	size;
	int flags;
{
	struct vextops_alloc_args vap;
	int error;

	vap.a_head.a_ops = &vextops;
	vap.a_vext = vext;
	vap.a_start = start;
	vap.a_addr += vap.a_start;
	vap.a_size = size;
	vap.a_flags = flags;

    if (vextops.vm_extent_alloc == NULL) {
    	return (EOPNOTSUPP);
    }

	error = vextops.vm_extent_alloc(vext, start, size, flags);

	return (error);
}


int
vm_extent_suballoc(vext, start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result)
	struct vm_extent *vext;
	vm_offset_t start, end;
	vm_size_t	size;
	int flags, malloctypes, mallocflags;
	u_long *result, alignment, boundary;
{
	struct vextops_suballoc_args vap;
	int error;

	vap.a_head.a_ops = &vextops;
	vap.a_vext = vext;
	vap.a_start = start;
	vap.a_end += vap.a_start;
	vap.a_size = size;
	vap.a_malloctypes = malloctypes;
	vap.a_mallocflags = mallocflags;
	vap.a_alignment = alignment;
	vap.a_boundary = boundary;
	vap.a_flags = flags;
	vap.a_result = result;

    if (vextops.vm_extent_suballoc == NULL) {
    	return (EOPNOTSUPP);
    }

	error = vextops.vm_extent_suballoc(vext, start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result);

	return (error);
}

int
vm_extent_free(vext, start, size, malloctypes, flags)
	struct vm_extent *vext;
	vm_offset_t start;
	vm_size_t	size;
	int malloctypes, flags;
{
	struct vextops_free_args vap;
	int error;

	vap.a_head.a_ops = &vextops;
	vap.a_vext = vext;
	vap.a_start = start;
	vap.a_malloctypes = malloctypes;
	vap.a_flags = flags;

    if (vextops.vm_extent_free == NULL) {
    	return (EOPNOTSUPP);
    }

	error = vextops.vm_extent_free(vext, start, size, malloctypes, flags);

	return (error);
}

int
vm_extent_destroy(vext)
	struct vm_extent *vext;
{
	struct vextops_destroy_args vap;
	int error;

	vap.a_head.a_ops = &vextops;
	vap.a_vext = vext;

    if (vextops.vm_extent_destroy == NULL) {
    	return (EOPNOTSUPP);
    }

	error = vextops.vm_extent_destroy(vext);

	return (error);
}
