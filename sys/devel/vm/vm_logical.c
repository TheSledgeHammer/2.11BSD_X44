/*
 * vm_logical.c
 *
 *  Created on: 21 Oct 2020
 *      Author: marti
 */
#include <vm_segment.h>
#include <vm_logical.h>

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


vm_logical_create(first, last)
	caddr_t first, last;
{
	struct vm_logical *lseg;
}
