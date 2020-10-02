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
	struct pttree							sg_pgtable;		/* list of all page tables in segment */

	CIRCLEQ_ENTRY(vm_segment)				sg_list;
	simple_lock_data_t 						sg_lock;
	int										sg_flags;
	vm_object_t								sg_object;
	vm_offset_t 							sg_offset;
	vm_size_t								sg_size;		/* segment size */
	int 									sg_ref_count;
	CIRCLEQ_ENTRY(vm_segment)				sg_cached_list;	/* for persistence */
};

/* flags */
#define SEG_ACTIVE		0x01
#define SEG_INACTIVE	0x02
#define SEG_ALLOCATED	0x04

CIRCLEQ_HEAD(vm_segment_hash_head, vm_segment_hash_entry);
struct vm_segment_hash_entry {
    CIRCLEQ_ENTRY(vm_segment_hash_entry)   	sge_hlinks;
    vm_segment_t                      		sge_segment;
};
typedef struct vm_segment_hash_entry  		*vm_segment_hash_entry_t;

struct seglist  	vm_segment_list;

simple_lock_data_t	vm_segment_list_lock;

struct seglist  	vm_segment_cache_list;
int					vm_segment_cached;			/* size of cached list */
simple_lock_data_t	vm_segment_cache_lock;

#define	vm_segment_cache_lock()			simple_lock(&vm_segment_cache_lock)
#define	vm_segment_cache_unlock()		simple_unlock(&vm_segment_cache_lock)

#define	vm_segment_lock_init(segment)	simple_lock_init(&(segment)->sg_lock)
#define	vm_segment_lock(segment)		simple_lock(&(segment)->sg_lock)
#define	vm_segment_unlock(segment)		simple_unlock(&(segment)->sg_lock)
#define	vm_segment_lock_try(segment)	simple_lock_try(&(segment)->sg_lock)
#define	vm_segment_sleep(event, segment, interruptible) \
			thread_sleep((event), &(segment)->sg_lock, (interruptible))

void			vm_segment_setpager(vm_segment_t, boolean_t);

/* faults */



#endif /* VM_SEGMENT_H_ */
