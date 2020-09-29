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

#include <sys/user.h>
#include <sys/tree.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/fnv_hash.h>
#include <devel/vm/include/vm.h>
#include "vm_segment.h"

struct seg_tree 		*vm_segment_bucket;
int						vm_segment_bucket_count = 0;	/* How big is array? */
int						vm_segment_hash_mask;			/* Mask for hash function */
simple_lock_data_t		bucket_lock;					/* lock for all buckets XXX */


vm_segment

unsigned long
vm_segment_hash(object, offset)
	vm_object_t		object;
	vm_offset_t 	offset;
{
	Fnv32_t hash1 = fnv_32_buf(&object, (sizeof(&object) + offset)&vm_segment_hash_mask, FNV1_32_INIT)%vm_segment_hash_mask;
	Fnv32_t hash2 = (((unsigned long)object+(unsigned long)offset)&vm_segment_hash_mask);
    return (hash1^hash2);
}

int
vm_segment_rb_compare(seg1, seg2)
	vm_segment_t seg1, seg2;
{
	if(seg1->index < seg2->index) {
		return(-1);
	} else if(seg1->index > seg2->index) {
		return(1);
	}
	return (0);
}

RB_PROTOTYPE(seg_tree, vm_segment, segt, vm_segment_rb_compare);
RB_GENERATE(seg_tree, vm_segment, segt, vm_segment_rb_compare);


/* insert segment into object */
void
vm_segment_insert(seg, object, offset)
	register vm_segment_t	seg;
	register vm_object_t	object;
	register vm_offset_t	offset;
{

	seg->object = object;
	seg->offset = offset;

	register struct seg_tree *bucket = &vm_segment_bucket[vm_segment_hash(object, offset)];

	RB_INSERT(seg_tree, bucket, seg);

	//seg->flags;
}

/* remove segment from object */
void
vm_segment_remove(seg)
	vm_segment_t seg;
{
	register struct seg_tree *bucket = &vm_segment_bucket[vm_segment_hash(seg->object, seg->offset)];

	if(bucket) {
		RB_REMOVE(seg_tree, bucket, seg);
	}
}

/* lookup segment from object/offset pair */
vm_segment_t
vm_segment_lookup(object, offset)
	register vm_object_t	object;
	register vm_offset_t	offset;
{
	register vm_segment_t		seg;
	register struct seg_tree 	*bucket;

	bucket = &vm_segment_bucket[vm_segment_hash(object, offset)];
	for(seg = RB_FIRST(seg_tree, bucket); seg != NULL; seg = RB_NEXT(seg_tree, bucket, seg)) {
		if(RB_FIND(seg_tree, bucket, seg)->index == vm_segment_hash(object, offset)) {
			if(RB_FIND(seg_tree, bucket, seg)->object == object && RB_FIND(seg_tree, bucket, seg)->offset == offset) {
				return (seg);
			}
		}
	}

	return (NULL);
}


