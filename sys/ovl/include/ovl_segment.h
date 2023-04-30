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

#ifndef _OVL_SEGMENT_H_
#define _OVL_SEGMENT_H_

#include <ovl/include/ovl_page.h>
#include <vm/include/vm_segment.h>

struct ovl_vm_segment_hash_head;
struct ovl_seglist;
TAILQ_HEAD(ovl_vm_segment_hash_head , ovl_segment);

CIRCLEQ_HEAD(ovl_seglist, ovl_segment);
struct ovl_segment {
	struct ovl_pglist				pglist; 				/* Pages in overlay pglist memory */

	CIRCLEQ_ENTRY(ovl_segment) 		hashlist;				/* hash table links (O) */
	CIRCLEQ_ENTRY(ovl_segment) 		seglist;				/* segments in same object (O) */

	int								flags;
	ovl_object_t					object;					/* which object am I in (O,S)*/
	vm_offset_t 					offset;					/* offset into object (O,S) */

	vm_offset_t						log_addr;				/* segment logical address */

	vm_segment_t					vm_segment;				/* a vm_segment being held */
	TAILQ_ENTRY(ovl_segment)    	vm_segment_hlist;		/* list of all associated vm_segments */
};

/* flags */
#define OVL_SEG_VM_SEG				0x16	/* overlay segment holds vm_segment */

#ifdef _KERNEL

extern
struct ovl_seglist 					ovl_segment_list;
extern
simple_lock_data_t					ovl_segment_list_lock;
extern
struct ovl_vm_segment_hash_head    	*ovl_vm_segment_hashtable;
extern
long				           		ovl_vm_segment_count;
extern
simple_lock_data_t					ovl_vm_segment_hash_lock;
extern
long								ovl_first_segment;
extern
long								ovl_last_segment;
extern
vm_offset_t							ovl_first_logical_addr;
extern
vm_offset_t							ovl_last_logical_addr;

#define	ovl_segment_lock_lists()	simple_lock(&ovl_segment_list_lock)
#define	ovl_segment_unlock_lists()	simple_unlock(&ovl_segment_list_lock)

#define	ovl_vm_segment_hash_lock()		simple_lock(&ovl_vm_segment_hash_lock)
#define	ovl_vm_segment_hash_unlock()	simple_unlock(&ovl_vm_segment_hash_lock)

void				ovl_segment_insert(ovl_segment_t, ovl_object_t, vm_offset_t);
void				ovl_segment_remove(ovl_segment_t);
ovl_segment_t		ovl_segment_lookup(ovl_object_t, vm_offset_t);
void				ovl_segment_startup(vm_offset_t *, vm_offset_t *);

#endif /* KERNEL */
#endif /* _OVL_SEGMENT_H_ */
