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
#include <sys/tree.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <devel/vm/include/vm.h>
#include "vm_segment.h"

void
vm_segment_init(seg, name, start, end)
	vm_seg_t seg;
	char *name;
	vm_offset_t start, end;
{
	RB_INIT(&seg->seg_rbroot);
	VM_EXTENT_CREATE(seg->seg_extent, name, start, end, M_VMSEG, 0, 0, EX_WAITOK);
	seg->seg_nentries = 0;
	seg->seg_ref_count = 1;
	seg->seg_name = name;
	seg->seg_start = start;
	seg->seg_end = end;

	if(seg->seg_extent) {
		VM_EXTENT_ALLOC(seg->seg_extent, seg->seg_start, seg->seg_end, EX_WAITOK);
	}
}

vm_segment_entry_create(entry, start, end, size, malloctypes, mallocflags, alignment, boundary, flags)
	vm_seg_entry_t entry;
{
	entry->segs_start;
	entry->segs_end;
	entry->segs_addr;
	entry->segs_size;
}


struct vm_segment *
vm_segment_init(rmalloc, segmented)
	boolean_t rmalloc, segmented;
{
	struct vm_segment *seg;
	if(rmalloc) {
		if(segmented) {
			RMALLOC3(seg, struct vm_segment *, d_size, s_size, t_size, sizeof(struct vm_segment *));
		} else {
			RMALLOC(seg, struct vm_segment *, sizeof(struct vm_segment *));
		}
	} else {
		MALLOC(seg, struct vm_segment *, sizeof(struct vm_segment *), M_WAITOK, M_VMSEG);
	}
	return (seg);
}

struct extent *
vm_segment_create(rmalloc, segmented)
	boolean_t rmalloc, segmented;
{
	register struct vm_segment *seg;
	if(seg == NULL) {
		seg = vm_segment_alloc(rmalloc, segmented);
	} else {
		memset(seg, 0, sizeof(struct vm_segment *));
	}

	seg->seg_extent = extent_create(name, start, end, mtype, storage, storagesize, flags);

	return (seg->seg_extent);
}

vm_segment_alloc(min, max, flags)
{
	register struct vm_segment *seg;
	if(seg->seg_extent) {
		if (extent_alloc_region(seg->seg_extent, min, max, flags)) {

		}
	}
}
