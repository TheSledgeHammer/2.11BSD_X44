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
	struct ovpglist					ovsg_ovpglist; 	/* Pages in Resident memory */

	CIRCLEQ_ENTRY(ovl_segment) 		ovsg_hashlist;	/* hash table links (O) */
	CIRCLEQ_ENTRY(ovl_segment) 		ovsg_seglist;	/* segments in same object (O) */

	int								ovsg_flags;
	ovl_object_t					ovsg_object;	/* which object am I in (O,S)*/
	vm_offset_t 					ovsg_offset;	/* offset into object (O,S) */

	int								ovsg_resident_page_count;	/* number of resident pages */
};

extern
struct ovseglist  	ovl_segment_list;
extern
simple_lock_data_t	ovl_segment_list_lock;

#define	ovl_segment_lock_lists()	simple_lock(&ovl_segment_list_lock)
#define	ovl_segment_unlock_lists()	simple_unlock(&ovl_segment_list_lock)

void				ovl_segment_insert(ovl_segment_t, ovl_object_t, vm_offset_t);
void				ovl_segment_remove(ovl_segment_t);
ovl_segment_t		ovl_segment_lookup(ovl_object_t, vm_offset_t);
void				ovl_segment_init(vm_offset_t, vm_offset_t);

#endif /* _OVL_SEGMENT_H_ */
