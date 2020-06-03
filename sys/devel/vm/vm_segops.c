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

#include <sys/user.h>
#include <include/vm_extent.h>

struct vm_extentops vm_segmentops[] = {
	.vm_extent_create =	vm_segment_create,
	.vm_extent_mallocok = vm_segment_mallocok,
	.vm_extent_alloc = vm_segment_malloc,
	.vm_extent_suballoc = vm_segment_suballoc,
	.vm_extent_free = vm_segment_free,
	.vm_extent_destroy = vm_segment_destroy
};

int
vm_segment_create(ap)
	struct vm_extentops_create_args *ap;
{
	struct vm_extent *vext = ap->a_vext;
	struct extent *ext = vext->vext_ext;

	if(vext == NULL) {
		MALLOC(vext, struct vm_extent *, sizeof(struct vm_extent *), M_VMEXTENT, M_WAITOK);
	}

	ext = extent_create(ext, ap->a_name, ap->a_start, ap->a_end, ap->a_mtype, ap->a_storage, ap->a_storagesize, ap->a_flags);

	return (0);
}

int
vm_segment_mallocok(ap)
	struct vm_extentops_mallocok_args *ap;
{
	struct vm_extent *vext = ap->a_vext;
	int mallocok = ap->a_mallocok;

	if(vext != NULL && mallocok != 0) {
		return (1);
	}
	return (0);
}

int
vm_segment_malloc(ap)
	struct vm_extentops_alloc_args *ap;
{
	struct vm_extent *vext = ap->a_vext;
	struct extent *ext = vext->vext_ext;
	int error;

	if(ext != NULL) {
		error = extent_alloc_region(ext, ap->a_start, ap->a_size, ap->a_flags);
	}
	return (error);
}

int
vm_segment_suballoc(ap)
	struct vm_extentops_suballoc_args *ap;
{
	struct vm_extent *vext = ap->a_vext;
	struct extent *ext = vext->vext_ext;
	int mallocflags = ap->a_mallocflags;
	int malloctypes = ap->a_malloctypes;
	int error;

	if(ext != NULL && (mallocflags & malloctypes)) {
		error = extent_alloc_subregion(ext, ap->a_start, ap->a_end, ap->a_size, ap->a_malloctypes, ap->a_mallocflags, ap->a_alignment, ap->a_boundary, ap->a_flags, ap->a_result);
	}
	return (error);
}

int
vm_segment_free(ap)
	struct vm_extentops_free_args *ap;
{
	struct vm_extent *vext = ap->a_vext;
	struct extent *ext = vext->vext_ext;
	int flags = ap->a_flags;
	int malloctypes = ap->a_malloctypes;
	int error;

	if(ext != NULL && (flags & malloctypes)) {
		error = extent_free(ext, ap->a_start, ap->a_size, flags);
	}
	return (error);
}

int
vm_segment_destroy(ap)
	struct vm_extentops_destroy_args *ap;
{
	struct vm_extent *vext = ap->a_vext;
	struct extent *ext = vext->vext_ext;
	int error;

	if(ext != NULL) {
		error = extent_destroy(ext);
	}
	return (error);
}
