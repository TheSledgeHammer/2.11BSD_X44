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

#include <sys/queue.h>

extern struct extent vm_extent;

typedef struct vm_segment 		*vm_seg_t;
typedef struct vm_segment_entry *vm_seg_entry_t;

struct vm_seg_clist;
struct vm_seg_rbtree;
/* Segment Entry */
struct vm_segment_entry {
	RB_ENTRY(vm_segment_entry) 		seg_rbentry;		/* tree entries */

	/* Segment Entry (Extent Subregion) */
	vm_offset_t 					segs_start;			/* entry start address */
	vm_offset_t 					segs_end;			/* entry end address */
	caddr_t							segs_addr;			/* entry address */
	vm_size_t						segs_size;			/* entry size */

	/* VM Space Related Address Segments & Sizes */
	segsz_t 						segs_tsize;			/* text size (pages) XXX */
	segsz_t 						segs_dsize;			/* data size (pages) XXX */
	segsz_t 						segs_ssize;			/* stack size (pages) */
	caddr_t							segs_taddr;			/* user virtual address of text XXX */
	caddr_t							segs_daddr;			/* user virtual address of data XXX */
	caddr_t 						segs_minsaddr;		/* user VA at min stack growth */
	caddr_t 						segs_maxsaddr;		/* user VA at max stack growth */

	CIRCLEQ_ENTRY(vm_segment_entry) seg_clentry;		/* entries in a circular list */
};

RB_HEAD(vm_seg_rbtree, vm_segment_entry);
struct vm_segment {
	struct vm_extent				*seg_extent;		/* vm extent manager */

	struct vm_seg_rbtree 			seg_rbroot;			/* tree of segment entries */
	struct vm_seg_clist				seg_clheader;

	lock_data_t						seg_lock;			/* Lock for map data */
	int								seg_nentries;		/* Number of entries */
	int								seg_ref_count;		/* Reference count */
	simple_lock_data_t				seg_ref_lock;		/* Lock for ref_count field */
	vm_seg_entry_t					seg_hint;			/* hint for quick lookups */
	simple_lock_data_t				seg_hint_lock;		/* lock for hint storage */


	/* VM Segment Space */
	char 							*seg_name;			/* segment name */
	vm_offset_t 					seg_start;			/* segment start address */
	vm_offset_t 					seg_end;			/* segment end address */
	caddr_t							seg_addr;			/* segment address */
	vm_size_t						seg_size;			/* segment size */

	struct pmap						seg_pmap;			/* private physical map */
};

/* Segmented Space Address Layout */
#define VMSPACE_START
#define VMSPACE_END
#define AVMSPACE_START
#define AVMSPACE_END

#define OVLSPACE_START
#define OVLSPACE_END

#endif /* _VM_SEG_H_ */
