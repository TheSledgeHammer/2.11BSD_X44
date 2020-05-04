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

#include <sys/param.h>
#include <sys/proc.h>

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_seg.h>
#include <devel/vm/include/vm_extent.h>
#include <devel/vm/include/vm_extops.h>

void
vm_segment_init(ext, ext_name, start, end)
	struct vm_extent *ext;
	char *ext_name;
	vm_offset_t start, end;
{
	vm_extent_mallocok();

	vm_extent_init(ext, ext_name, start, end);
}

/* Allocate a new segment extent */
struct vm_segment *
vm_segment_malloc(vmseg, size)
	struct vm_segment *vmseg;
	vm_size_t size;
{
	register struct vm_extent *ext = vmseg->seg_extent;
	int error;

	error = vm_extent_alloc(ext, size);

	if(error) {
		printf("%#lx-%#lx of %s couldn't be allocated\n", ext->addr, ext->addr + size, ext->name);
	}

	return (vmseg);
}

/* Should return the new extent / segment_space */
/* Allocate a new segment extent region */
void *
vm_segmentspace_malloc(vmseg, start, end, size, flag, type, result)
	struct vm_segment *vmseg;
	//struct vm_segmentspace *vmsegs;
	vm_offset_t start, end;
	vm_size_t size;
	int flag, type;
	u_long *result;
{
	register struct vm_extent *ext = vmseg->seg_extent;
	int error;

	error = vm_extent_suballoc(ext, start, end, size, flag, type, result);

	if(error) {
		return error;
		//printf("%#lx-%#lx of %s couldn't be allocated\n", ext->addr, ext->addr + size, ext->name);
	}

	return (0);
}

/* Free an Allocated Segment extent region */
void
vm_segment_free(ext, size, type)
	struct vm_extent *ext;
	vm_size_t size;
	int type;
{
	vm_extent_free(ext, size, type);
}
