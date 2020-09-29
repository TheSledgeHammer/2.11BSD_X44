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

#ifndef _VM_SEG_H_
#define _VM_SEG_H_

#include <sys/queue.h>
#include <sys/tree.h>

struct vm_segment;
typedef struct vm_segment 	*vm_segment_t;

struct seg_tree;
RB_HEAD(seg_tree, vm_segment);
struct vm_segment {
	struct pgtree			memt;					/* Resident memory (red-black tree) */
	RB_ENTRY(vm_segment)	segt;
	u_long					index;
	int						flags;
	vm_object_t				object;
	vm_offset_t 			offset;
	int						ref_count;				/* How many refs?? */
	vm_size_t				size;					/* segment size */
	int						resident_page_count;	/* number of resident pages */
	vm_pager_t				pager;					/* Where to get data */
	vm_offset_t				paging_offset;			/* Offset into paging space */
};

/* flags */
#define SEG_ACTIVE
#define SEG_INACTIVE

/* faults */
//MULTICS VM: (segmented paging)
//page multiplexing: core blocks among active segments.
//least-recently-used algorithm
//supervisor;
//segment control; 		(SC)
//page control; 		(PC)
//directory control; 	(DC)

#endif /* _VM_SEG_H_ */
