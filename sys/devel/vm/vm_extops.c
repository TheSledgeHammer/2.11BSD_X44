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

#include <sys/extent.h>
#include <sys/malloc.h>
#include <devel/vm/include/vm_extops.h>

#define EXTENT_OPS(extops, error, ap, extop_field)	\
	error = extops->extop_field(ap)

struct extent *
extent_load(vmel, name, start, end, mtype, storage, storagesize, flags)
	union vm_extentloader *vmel;
	char *name;
	vm_offset_t start, end;
	int mtype, flags;
	caddr_t	storage;
	vm_size_t storagesize;
{
	register struct vm_extent_create args = vmel->vmel_create;
	register struct extent *ext = vmel->vmel_ext;
	args.vmel_name = name;
	ext = extent_create(ext, name, start, end, mtype, storage, storagesize, flags);
	args.vmel_start = start;
	args.vmel_end = end;
	args.vmel_mtype = mtype;
	args.vmel_storage = storage;
	args.vmel_storagesize = storagesize;
	args.vmel_flags = flags;

	return (ext);
}

struct extent *
extops_create(ops, ext, name, start, end, mtype, storage, storagesize, flags)
	struct vm_extops *ops;
	struct extent 	*ext;
	char *name;
	vm_offset_t start, end;
	int mtype, flags;
	caddr_t	storage;
	vm_size_t storagesize;
{
	struct vm_extops_create_args args;
	int error;

	//ext = extent_create(ext, name, start, end, mtype, storage, storagesize, flags);

	args.a_head.a_ops = ops;
	args.a_ext = extent_create(ext, name, start, end, mtype, storage, storagesize, flags);
	args.a_name = name;
	args.a_start = start;
	args.a_end = end;
	args.a_mtype = mtype;
	args.a_storage = storage;
	args.a_storagesize = storagesize;
	args.a_flags = flags;

	union vm_extargs *vargs;
	if(error || ops->extops_create == NULL) {
		return (error);
	}

	return ((ops->extops_create)(&args));
}

int
extops_mallocok(ops, mallocok)
	struct vm_extops *ops;
	int mallocok;
{
	struct vm_extops_mallocok_args args;
	int error;
	args.a_head.a_ops = ops;
	args.a_mallocok = mallocok;

	error = (ops->extops_mallocok)(&args);
	if(error) {
		return (error);
	}
	return (0);
}

int
extops_alloc(ops, ext, start, size, flags)
	struct vm_extops *ops;
	struct extent *ext;
	vm_offset_t start;
	vm_size_t	size;
	int flags;
{
	struct vm_extops_alloc_args args;
	int error = extent_alloc_region(ext, start, size, flags);

	args.a_head.a_ops = ops;
	args.a_ext = ext;
	args.a_start = start;
	args.a_addr += args.a_start;
	args.a_size = size;
	args.a_flags = flags;

	error = (ops->extops_alloc)(&args);
	if(error) {
		return (error);
	}
	return (0);
}

int
extops_suballoc(ops, ext, start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result)
	struct vm_extops *ops;
	struct extent *ext;
	vm_offset_t start, end;
	vm_size_t	size;
	int flags, malloctypes, mallocflags;
	u_long *result, alignment, boundary;
{
	struct vm_extops_suballoc_args args;
	int error;// = extent_alloc_subregion(ext, start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result);

	args.a_head.a_ops = ops;
	args.a_ext = ext;
	args.a_start = start;
	args.a_end += args.a_start;
	args.a_size = size;
	args.a_malloctypes = malloctypes;
	args.a_mallocflags = mallocflags;
	args.a_alignment = alignment;
	args.a_boundary = boundary;
	args.a_flags = flags;
	args.a_result = result;

	error = (ops->extops_suballoc)(&args);
	if(error) {
		return (error);
	}
	return (0);
}

int
extops_free(ops, ext, start, size, malloctypes, flags)
	struct vm_extops *ops;
	struct extent *ext;
	vm_offset_t start;
	vm_size_t	size;
	int malloctypes, flags;
{
	struct vm_extops_free_args args;
	int error;// = extent_free(ext, start, size, flags);

	args.a_head.a_ops = ops;
	args.a_ext = ext;
	args.a_start = start;
	args.a_malloctypes = malloctypes;
	args.a_flags = flags;

	error = (ops->extops_free)(&args);
	if(error) {
		return (error);
	}
	return (0);
}

int
extops_destroy(ops, ext)
	struct vm_extops *ops;
	struct extent *ext;
{
	struct vm_extops_destroy_args args;
	int error;

	ext = extent_destroy(ext);

	args.a_head.a_ops = ops;
	args.a_ext = ext;

	error = (ops->extops_destroy)(&args);
	if(error) {
		return (error);
	}
	return (0);
}
