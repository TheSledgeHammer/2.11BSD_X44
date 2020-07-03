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

#ifndef SYS_DEVEL_VM_EXTENTS_VM_EXTENT_H_
#define SYS_DEVEL_VM_EXTENTS_VM_EXTENT_H_

#include <sys/extent.h>

struct vm_extent {
	struct extent		*vext_ext;
	struct vm_extentops *vext_op;
};

struct vm_extentops {
    int (* vm_extent_create)(struct vm_extent *vext, struct extent *ext, char *name, vm_offset_t start, vm_offset_t end, int mtype, caddr_t storage, vm_size_t storagesize, int flags);
    int (* vm_extent_mallocok)(struct vm_extent *vext, int mallocok);
	int (* vm_extent_alloc)(struct vm_extent *vext, vm_offset_t start, vm_offset_t end, int flags);
	int (* vm_extent_suballoc)(struct vm_extent *vext, vm_offset_t start, vm_offset_t end, vm_size_t size, int malloctypes, int mallocflags, u_long alignment, u_long boundary, int flags, u_long *result);
	int (* vm_extent_free)(struct vm_extent *vext, vm_offset_t start, vm_size_t size, int malloctypes, int flags);
	int (* vm_extent_destroy)(struct vm_extent *vext);
};
extern struct vm_extentops vextops;

#define VM_EXTENT_CREATE(vext, name, start, end, mtype, storage, storagesize, flags) 								(*((vext)->vext_op->vm_extent_create))(vext, name, start, end, mtype, storage, storagesize, flags)
#define VM_EXTENT_MALLOCOK(vext, mallocok)																			(*((vext)->vext_op->vm_extent_mallocok))(vext, mallocok)
#define VM_EXTENT_ALLOC(vext, start, size, flags) 																	(*((vext)->vext_op->vm_extent_alloc))(vext, start, size, flags)
#define VM_EXTENT_SUBALLOC(vext, start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result)	(*((vext)->vext_op->vm_extent_suballoc))(vext, start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result)
#define VM_EXTENT_FREE(vext, start, size, malloctypes, flags)														(*((vext)->vext_op->vm_extent_free))(vext, start, size, malloctypes, flags)
#define VM_EXTENT_DESTROY(vext)																						(*((vext)->vext_op->vm_extent_destroy))(vext)

struct vm_extentops_generic_args {
	struct vm_extentops					*a_ops;
};

struct vm_extentops_create_args {
	struct vm_extentops_generic_args	a_head;
	struct vm_extent					*a_vext;
	char								*a_name;
	vm_offset_t 						a_start;
	vm_offset_t							a_end;
	int 								a_mtype;
	caddr_t								a_storage;
	vm_size_t 							a_storagesize;
	int									a_flags;
};

struct vm_extentops_mallocok_args {
	struct vm_extentops_generic_args	a_head;
	struct vm_extent					*a_vext;
	int									a_mallocok;
};

struct vm_extentops_alloc_args {
	struct vm_extentops_generic_args	a_head;
	struct vm_extent					*a_vext;
	vm_offset_t 						a_start;
	vm_offset_t             			a_addr;
	vm_size_t 							a_size;
	int 								a_flags;
};

struct vm_extentops_suballoc_args {
	struct vm_extentops_generic_args	a_head;
	struct vm_extent					*a_vext;
	vm_offset_t 						a_start;
	vm_offset_t							a_end;
	vm_size_t 							a_size;
	int 								a_malloctypes;
	int 								a_mallocflags;
	u_long								a_alignment;
	u_long								a_boundary;
	int 								a_flags;
	u_long 								*a_result;
};

struct vm_extentops_free_args {
	struct vm_extentops_generic_args	a_head;
	struct vm_extent					*a_vext;
	vm_offset_t 						a_start;
	vm_size_t 							a_size;
	int 								a_malloctypes;
	int									a_flags;
};

struct vm_extentops_destroy_args	{
	struct vm_extentops_generic_args	a_head;
	struct vm_extent					*a_vext;
};

#define VEXTTOARGS(ap) ((ap)->a_head.a_ops)
#define EXTENTOPS(ap, ops_field)				\
			VEXTTOARGS(ap)->ops_field

void vm_extops_init();
void vm_extops_malloc(struct vm_extentops *vextops);

#endif /* _VM_EXTOPS_H_ */
