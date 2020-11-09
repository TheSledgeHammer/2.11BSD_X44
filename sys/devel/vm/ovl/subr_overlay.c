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

/* overlay subroutines & management */
/*
 * TODO: Finish interface routines:
 * - insertion into overlays list
 * - removal from overlays list
 * - overlays list lookup
 * - loading & unloading an overlay in list
 * - execution of an overlay in list
 *
 * - Adjust:
 * - overlay_table & overlay_struct
 * - lookup, removal and insertion is convoluted & does not flow naturally with overlaid vm components
 */

#include <sys/fnv_hash.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/queue.h>
#include <devel/vm/ovl/overlay.h>

#define OVERLAY_HASH_COUNT 128

struct overlaylst			overlays_hashtable[OVERLAY_HASH_COUNT];
simple_lock_data_t			overlays_hashtable_lock;
struct overlaylst 			overlays_list;
simple_lock_data_t			overlays_list_lock;

u_long
ovl_overlay_hash(ovltable)
	struct overlay_table *ovltable;
{
	Fnv32_t hash1 = fnv_32_buf(&ovltable, (sizeof(&ovltable))&OVERLAY_HASH_COUNT, FNV1_32_INIT)%OVERLAY_HASH_COUNT;
	return (hash1);
}

void
ovl_overlayer_init(start, end, size)
	vm_offset_t start, end;
	vm_size_t	size;
{
	register struct overlay_table 	*ovltable;
	register ovl_overlay_t 		  	ovl;
	int 							i;

	/* initialize overlay table */
	overlay_malloc(ovltable, sizeof(struct overlay_table *), OVL_OVTABLE, M_WAITOK);

	/* initialize overlay table private */
	overlay_malloc(ovltable->o_private.og_overlay, sizeof(ovl_overlay_t), OVL_OVERLAYER, M_WAITOK);
	overlay_malloc(ovltable->o_private.og_object, sizeof(struct overlay_object *), OVL_OVOBJECT, M_WAITOK);
	overlay_malloc(ovltable->o_private.og_segment, sizeof(struct overlay_segment *), OVL_OVSEGMENT, M_WAITOK);
	overlay_malloc(ovltable->o_private.og_page, sizeof(struct overlay_page *), OVL_OVPAGE, M_WAITOK);

	ovltable->o_start = start;
	ovltable->o_end = end;
	ovltable->o_size = size;
	ovltable->o_offset = 0;
	ovltable->o_mapper = ovlmem_suballoc(overlay_map, start, end, size);
	ovltable->o_area = ovl_allocate(ovltable->o_mapper, (caddr_t)0, size, 0);

	/* initialize overlays list, hashtables & locks */
	simple_lock_init(&overlays_list_lock, "overlays_list_lock");

	CIRCLEQ_INIT(&overlays_list);

	for (i = 0; i < OVERLAY_HASH_COUNT; i++) {
		CIRCLEQ_INIT(&overlays_hashtable[i]);
	}

	simple_lock_init(&overlays_hashtable_lock, "overlays_hashtable_lock");

	ovl = ovltable->o_private.og_overlay;
	ovl->o_novl = 0;
	ovl->o_type = OV_DFLT;
}

/* [Internal use only] */
void
ovl_overlayer_insert(ovltable, type)
	struct overlay_table *ovltable;
	int type;
{
	struct overlaylst *bucket = &overlays_hashtable[ovl_overlay_hash(ovltable)];
	register ovl_overlay_t ovl = ovltable->o_private.og_overlay;

	if(ovl->o_novl <= NOVL) {
		ovl->o_flags = OVL_INACTIVE;
		ovl->o_type = type;
		CIRCLEQ_INSERT_HEAD(bucket, ovl, o_list);
		ovl->o_novl++;
	} else {
		panic("overlays reached max number permitted: cur %d max %d", ovl->o_novl, NOVL);
	}
}

/* [Internal use only] */
void
ovl_overlayer_remove(ovltable, type)
	struct overlay_table *ovltable;
	int type;
{
	struct overlaylst *bucket = &overlays_hashtable[ovl_overlay_hash(ovltable)];
	register ovl_overlay_t ovl = ovltable->o_private.og_overlay;

	if(ovl->o_flags == OVL_ACTIVE && ovl->o_type == type) {
		ovl->o_flags = OVL_INACTIVE;
		CIRCLEQ_REMOVE(bucket, ovl, o_list);
		ovl->o_novl--;
	}
}

/* [Internal use only] */
ovl_overlay_t
ovl_overlayer_lookup(ovltable)
	struct overlay_table *ovltable;
{
	struct overlaylst *bucket;
	register ovl_overlay_t ovl;

	bucket = &overlays_hashtable[ovl_overlay_hash(ovltable)];
	for(ovl = CIRCLEQ_FIRST(bucket); ovl != NULL; ovl = CIRCLEQ_NEXT(ovl, o_list)) {
		if(ovl == ovltable->o_private.og_overlay) {
			return (ovl);
		}
	}
	return (NULL);
}

void
ovl_overlayer_insert_vm_object(ovltable, object, vobject)
	struct overlay_table 	*ovltable;
	ovl_object_t 			object;
	vm_object_t 			vobject;
{
	struct overlay_object *ovlobject = ovltable->o_private.og_object;

	if(object == NULL) {
		return;
	}
	if(vobject == NULL) {
		return;
	}

	ovlobject->oo_object = object;
	ovlobject->oo_overlay = object->ovo_overlay;

	ovlobject->oo_vobject = vobject;
	ovl_object_insert_vm_object(object, vobject);
}

void
ovl_overlayer_insert_vm_segment(ovltable, segment, vsegment)
	struct overlay_table 	*ovltable;
	ovl_segment_t 			segment;
	vm_segment_t 			vsegment;
{
	struct overlay_segment *ovlsegment = ovltable->o_private.og_segment;

	if(segment == NULL) {
		return;
	}
	if(vsegment == NULL) {
		return;
	}

	ovlsegment->os_segment = segment;
	ovlsegment->os_object = segment->ovs_object;
	ovlsegment->os_offset = segment->ovs_offset;

	ovlsegment->os_vsegment = vsegment;
	ovl_segment_insert_vm_segment(segment, vsegment);
}

void
ovl_overlayer_insert_vm_page(ovltable, page, vpage)
	struct overlay_table 	*ovltable;
	ovl_page_t 				page;
	vm_page_t				vpage;
{
	struct overlay_page *ovlpage = ovltable->o_private.og_page;

	if(page == NULL) {
		return;
	}
	if(vpage == NULL) {
		return;
	}

	ovlpage->op_page = page;
	ovlpage->op_offset = page->ovp_offset;
	ovlpage->op_segment = page->ovp_segment;

	ovlpage->op_vpage = vpage;
	ovl_page_insert_vm_page(page, vpage);
}

void
ovl_overlayer_remove_vm_page(ovltable)
	struct overlay_table 	*ovltable;
{
	struct overlay_page *ovlpage = ovltable->o_private.og_page;

	ovl_page_remove_vm_page(ovlpage->op_page, ovlpage->op_vpage);
}
