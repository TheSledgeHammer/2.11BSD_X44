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

struct segmentops *segmenttab[] = {

};

struct segmentops *dfltsegmentops = NULL;	/* default segment */

vm_segment_init()
{

}

vm_segment_t
vm_segment_allocate(type, handle, size, prot, off)
	int type;
	caddr_t handle;
	vm_size_t size;
	vm_prot_t prot;
	vm_offset_t off;
{
	struct segmentops *ops;

	ops = (type == SEG_DFLT) ? dfltsegmentops : segmenttab[type];
	if (ops)
		return ((*ops->sgo_alloc)(handle, size, prot, off));
	return (NULL);
}

void
vm_segment_deallocate(segment)
	vm_segment_t segment;
{
	if (segment == NULL)
		panic("vm_pager_deallocate: null pager");

	(*segment->sg_ops->sgo_dealloc)(segment);
}

int
vm_segment_get_segments(segment, mlist, npages, sync)
	vm_segment_t	segment;
	vm_page_t		*mlist;
	int				npages;
	boolean_t		sync;
{
	int rv;

	if (segment == NULL) {
		rv = VM_PAGER_OK;
		while (npages--)
			if (!vm_page_zero_fill(*mlist)) {
				rv = VM_PAGER_FAIL;
				break;
			} else
				mlist++;
		return (rv);
	}
	return ((*segment->sg_ops->sgo_getsegments)(segment, mlist, npages, sync));
}

int
vm_segment_put_segments(segment, mlist, npages, sync)
	vm_segment_t	segment;
	vm_page_t		*mlist;
	int				npages;
	boolean_t		sync;
{
	if (segment == NULL)
		panic("vm_segment_put_pages: null pager");
	return ((*segment->sg_ops->sgo_putsegments)(segment, mlist, npages, sync));
}
