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

/*
 * XXX MULTICS VM: (segmented paging)
 * page multiplexing: core blocks among active segments.
 * least-recently-used algorithm
 * caches: most used pages
 * - supervisor;
 * 		- segment control; 	(SC)
 *	 	- page control; 	(PC)
 *	 	- directory control;(DC)
 */

#ifndef _VM_SEGMENT_H_
#define _VM_SEGMENT_H_

#include <devel/vm/include/vm_object.h>
#include <devel/vm/include/vm_page.h>

struct seglist;
CIRCLEQ_HEAD(seglist, vm_segment);
struct vm_segment {
	struct pglist				sg_memq;				/* list of all pages in segment */

	CIRCLEQ_ENTRY(vm_segment)	sg_hlist;				/* hash table links (O) */
	CIRCLEQ_ENTRY(vm_segment)	sg_list;				/* segments in same object (O) */

	vm_object_t					sg_object;				/* which object am I in (O,S)*/
	vm_offset_t 				sg_offset;				/* offset into object (O,S) */
	vm_size_t					sg_size;				/* size of segment */
	int							sg_flags;				/* see below */

	int							sg_resident_page_count;	/* number of resident pages */

	caddr_t						sg_addr;				/* segment addr */
	int							sg_types;				/* see below (segment types) */

	caddr_t						sg_lv_addr;
};

#define VM_LVSEG_MAX 8
/*
 * vm_lvseg: describes logical memory
 */
struct vm_lvseg {
	caddr_t				start;
	caddr_t 			end;
	caddr_t				avail_start;
	caddr_t 			avail_end;
	struct vm_segment 	*segs;
	struct vm_segment 	*lastseg;

	segsz_t 			sg_dsize; /* data size */
	segsz_t 			sg_ssize; /* stack size */
	segsz_t 			sg_tsize; /* text size */
	caddr_t 			sg_daddr; /* virtual address of data */
	caddr_t 			sg_saddr; /* virtual address of stack */
	caddr_t 			sg_taddr; /* virtual address of text */
};

extern struct vm_lvseg vm_lvmem[VM_LVSEG_MAX];
extern int vm_nlvseg;

#define SEGMENT_DATA(dsize, daddr)
#define SEGMENT_STACK(ssize, saddr)
#define SEGMENT_TEXT(tsize, taddr)

/* flags */
#define SEG_ACTIVE		0x001	/* segment is active */
#define SEG_INACTIVE	0x002	/* segment is inactive */
#define SEG_RO			0x004	/* read-only */
#define SEG_WO			0x006	/* write-only */
#define SEG_RW			0x008	/* read-write */
#define SEG_ALLOCATED	0x010	/* segment has been allocated */
#define	SEG_BUSY		0x020	/* segment is in transit (O) */
#define	SEG_CLEAN		0x040	/* segment has not been modified */

/* types */
#define SEG_DATA		1		/* data segment */
#define SEG_STACK		2		/* stack segment */
#define SEG_TEXT		3		/* text segment */

#define	VM_SEGMENT_CHECK(seg) { 											\
	if ((((unsigned int) seg) < ((unsigned int) &vm_segment_array[0])) || 	\
	    (((unsigned int) seg) > 											\
		((unsigned int) &vm_segment_array[last_segment-first_segment])) || 	\
	    ((seg->sg_flags & (SEG_ACTIVE | SEG_INACTIVE)) == 					\
		(SEG_ACTIVE | SEG_INACTIVE))) 										\
		panic("vm_segment_check: not valid!"); 								\
}

extern
struct seglist  	vm_segment_list;			/* free list */
extern
struct seglist		vm_segment_list_active;		/* active list */
extern
struct seglist		vm_segment_list_inactive;	/* inactive list */
extern
vm_segment_t		vm_segment_array;			/* First segment in table */

extern
long 				first_segment;				/* first logical segment number */

extern
long 				last_segment;				/* last logical segment number */

extern
vm_offset_t			first_logical_addr;			/* logical address for first_segment */
extern
vm_offset_t			last_logical_addr;			/* logical address for last_segment */

extern
simple_lock_data_t	vm_segment_list_lock;
extern
simple_lock_data_t	vm_segment_list_activity_lock;

#define	VM_SEGMENT_INIT(seg, object, offset) { 			\
	(seg)->sg_flags = SEG_BUSY | SEG_CLEAN | SEG_RW; 	\
	vm_segment_insert((seg), (object), (offset)); 		\
}

#define VM_SEGMENT_TO_PHYS(seg, page) 	\
		vm_page_lookup(vm_segment_lookup((seg)->sg_object, (seg)->sg_offset), (page)->offset)->phys_addr;

#define PHYS_TO_VM_SEGMENT(pa) 			\
		(&vm_segment_array[atos[pa] - first_segment]);

#define	vm_segment_lock_lists()		simple_lock(&vm_segment_list_lock)
#define	vm_segment_unlock_lists()	simple_unlock(&vm_segment_list_lock)

#define vm_segment_set_modified(m)	{ (m)->sg_flags &= ~SEG_CLEAN; }

void		 	vm_segment_activate(vm_segment_t);
vm_segment_t 	vm_segment_alloc(vm_object_t, vm_offset_t);
void			vm_segment_deactivate(vm_segment_t);
void		 	vm_segment_free(vm_segment_t);
void			vm_segment_insert(vm_segment_t, vm_object_t, vm_offset_t);
void			vm_segment_remove(vm_segment_t);
vm_segment_t	vm_segment_lookup(vm_object_t, vm_offset_t);
void			vm_segment_startup(vm_offset_t, vm_offset_t);

#endif /* VM_SEGMENT_H_ */
