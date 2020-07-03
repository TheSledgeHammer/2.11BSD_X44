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

/* common code for vm_extent operations */

#include <sys/param.h>
#include <sys/proc.h>

#include <devel/vm/include/vm.h>
#include "vm_extent.h"
#include "vm_seg.h"

struct vm_extentops vextops = { vext_create, vext_alloc, vext_suballoc, vext_free, vext_destroy };

int
vext_create(vext, name, start, end, mallocok, mtype, storage, storagesize, flags)
	struct vm_extent *vext;
	char *name;
	vm_offset_t start, end;
	int mallocok, mtype, flags;
	caddr_t storage;
	vm_size_t storagesize;
{
	int error;

	if(mallocok == 1) {
		mallocok = VM_EXTENT_MALLOCOK(vext, 1);
		error = VM_EXTENT_CREATE(vext, name, start, end, mtype, storage, storagesize, EX_MALLOCOK | flags);
	} else {
		mallocok = VM_EXTENT_MALLOCOK(vext, 0);
		error = VM_EXTENT_CREATE(vext, name, start, end, mtype, storage, storagesize, flags);
	}
	return (error);
}

int
vext_alloc(start, size, flags)
	vm_offset_t start;
	vm_size_t size;
	int flags;
{
	struct vm_extent *vext;
	int error;
	error = VM_EXTENT_ALLOC(vext, start, size, flags);

	return (error);
}

int
vext_suballoc(start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result)
	vm_offset_t start, end;
	vm_size_t size;
	u_long alignment, boundary, *result;
	int malloctypes, mallocflags, flags;
{
	struct vm_extent *vext;
	int error;

	error = VM_EXTENT_SUBALLOC(vext, start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result);

	return (error);
}

int
vext_free(vext, start, size, malloctypes, flags)
	struct vm_extent *vext;
	vm_offset_t start;
	vm_size_t size;
	int malloctypes, flags;
{
	if(malloctypes & flags) {
		return (VM_EXTENT_FREE(vext, start, size, malloctypes, flags));
	}
	return (0);
}

int
vext_destroy(vext)
	struct vm_extent *vext;
{
	return (VM_EXTENT_DESTROY(vext));
}
