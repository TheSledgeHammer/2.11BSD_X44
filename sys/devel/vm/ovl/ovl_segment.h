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

#include <devel/vm/ovl/ovl_object.h>
#include <sys/queue.h>

struct ovseglist;
CIRCLEQ_HEAD(ovseglist, ovl_segment);
struct ovl_segment {
	struct pglist					ovsg_pglist; 	/* Pages in Resident memory */

	CIRCLEQ_ENTRY(ovl_segment) 		ovsg_hashlist;	/* hash table links (O) */
	CIRCLEQ_ENTRY(ovl_segment) 		ovsg_seglist;	/* segments in same object (O) */

	simple_lock_data_t 				ovsg_lock;

	int								ovsg_flags;
	ovl_object_t					ovsg_object;	/* which object am I in (O,S)*/
	vm_offset_t 					ovsg_offset;	/* offset into object (O,S) */
	int 							ovsg_ref_count;

	vm_offset_t						ovsg_phys_addr;	/* physical address of segment */
};

/* flags */
#define SEG_ACTIVE		0x01	/* segment is active */
#define SEG_INACTIVE	0x02	/* segment is inactive */
#define SEG_RO			0x03	/* read-only */
#define SEG_WO			0x04	/* write-only */
#define SEG_RW			0x05	/* read-write */
#define SEG_ALLOCATED	0x06	/* allocated segment */

extern
struct ovseglist  	ovl_segment_list;

extern
long 				first_segment;	/* first physical segment number */

extern
long 				last_segment;	/* last physical segment number */

void				ovl_segment_insert(ovl_segment_t, ovl_object_t, vm_offset_t);
void				ovl_segment_remove(ovl_segment_t);
ovl_segment_t		ovl_segment_lookup(ovl_object_t, vm_offset_t);
void				ovl_segment_init(vm_offset_t, vm_offset_t);

#endif /* _OVL_SEGMENT_H_ */
