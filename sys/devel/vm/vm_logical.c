/*
 * vm_logical.c
 *
 *  Created on: 21 Oct 2020
 *      Author: marti
 */
#include <vm_segment.h>
#include <vm_logical.h>

struct vm_logical vm_logical_segs[VM_LOGISEG_MAX];
int vm_logical_nsegs;

static void
vm_logical_freelist_add(vlf, segment, order, tail)
	struct vm_logical_freelist *vlf;
	vm_segment_t segment;
	int order, tail;
{
	if(tail) {
		CIRCLEQ_INSERT_TAIL(&vlf[order].vlf_seg, segment, sg_list);
	} else {
		CIRCLEQ_INSERT_HEAD(&vlf[order].vlf_seg, segment, sg_list);
	}
	vlf[order].vlf_cnt++;
}

static void
vm_logical_freelist_remove(vlf, segment, order)
	struct vm_logical_freelist *vlf;
	vm_segment_t segment;
	int order;
{
	CIRCLEQ_REMOVE(&vlf[order].vlf_seg, segment, sg_list);
	vlf[order].vlf_cnt--;
}

static void
vm_logical_segment_create(start, end)
	caddr_t start, end;
{
	struct vm_logical *lseg;

	KASSERT(vm_logical_nsegs < VM_LOGISEG_MAX, ("vm_logical_segment_create: increase VM_LOGISEG_MAX"));

	lseg = &vm_logical_segs[vm_logical_nsegs++];
	while (lseg > vm_logical_segs && (lseg - 1)->vl_start >= end) {
		*lseg = *(lseg - 1);
		lseg--;
	}

	lseg->vl_start = start;
	lseg->vl_end = end;
}


void
vm_logical_segment_add(start, end)
	caddr_t start, end;
{
	caddr_t paddr;
	paddr = start;
	vm_logical_segment_create(paddr, end);
}
