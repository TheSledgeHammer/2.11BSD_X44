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

/* VM Segments */
/*
 * VM_Extent Memory Manager:
 * (pmap_segment -> vm_segment) -> memory to rest of VM (replaces MALLOC/ FREE)
 */

#ifndef _VM_SEG_H_
#define _VM_SEG_H_

/*
 * pmap_segment method: allocate physical memory or virtual memory
 *	TODO:
 *	- mapping vm_segmentspace to various spaces (vmspace, avmspace, ovlspace)
 */

struct vm_seg_rbtree;
RB_HEAD(vm_seg_rbtree, vm_segmentspace);
struct vm_segment {
	struct vm_seg_rbtree 		seg_rbtree;			/* tree of segments space entries */
	struct pmap					seg_pmap;			/* private physical map */
	struct extent 				*seg_extent;		/* segment extent manager */

	/* VM Segment Space */
	char 						*seg_name;			/* segment name */
	vm_offset_t 				seg_start;			/* start address */
	vm_offset_t 				seg_end;			/* end address */
	caddr_t						seg_addr;			/* virtual address */
	vm_size_t					seg_size;			/* virtual size */

	int (* seg_segspace)(void *);
};

/* Segmented Space */
struct vm_segmentspace {
	RB_ENTRY(vm_segmentspace) 	seg_rbentry;		/* tree entries */

	char 						*segs_name;
	vm_offset_t 				segs_start;			/* start address */
	vm_offset_t 				segs_end;			/* end address */
	caddr_t						segs_addr;
	vm_size_t					segs_size;			/* virtual size */

	/* VM Space Related Address Segments & Sizes */
	segsz_t 					segs_tsize;			/* text size (pages) XXX */
	segsz_t 					segs_dsize;			/* data size (pages) XXX */
	segsz_t 					segs_ssize;			/* stack size (pages) */
	caddr_t						segs_taddr;			/* user virtual address of text XXX */
	caddr_t						segs_daddr;			/* user virtual address of data XXX */
	caddr_t 					segs_minsaddr;		/* user VA at min stack growth */
	caddr_t 					segs_maxsaddr;		/* user VA at max stack growth */
};

/* Segment Extent Memory Management */
void 				vm_segment_init(struct vm_extent *, char *, vm_offset_t, vm_offset_t);
struct vm_segment 	*vm_segment_malloc(struct vm_extent *, vm_size_t);
int 				vm_segmentspace_malloc(struct vm_extent *, vm_offset_t, vm_offset_t, vm_size_t, int, int, u_long *);
void 				vm_segment_free(struct vm_extent *, vm_size_t, int);

#endif /* _VM_SEG_H_ */
