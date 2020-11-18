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

#include <sys/tree.h>
#include <sys/fnv_hash.h>
#include <sys/malloc.h>
#include <sys/map.h>

#include <sys/user.h>

#include <devel/vm/ovl/ovl.h>
#include <devel/vm/ovl/ovl_segment.h>

struct ovseglist			*ovl_segment_buckets;
int							ovl_segment_bucket_count = 0;	/* How big is array? */
int							ovl_segment_hash_mask;			/* Mask for hash function */
simple_lock_data_t			ovl_segment_bucket_lock;		/* lock for all segment buckets XXX */

struct ovseglist  			ovl_segment_list;
simple_lock_data_t			ovl_segment_list_lock;

struct vsegment_hash_head 	*ovl_vsegment_hashtable;

void
ovl_segment_init(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	int	i;

	simple_lock_init(&ovl_segment_list_lock, "ovl_segment_list_lock");

	CIRCLEQ_INIT(&ovl_segment_list);

	if (ovl_segment_bucket_count == 0) {
		ovl_segment_bucket_count = 1;
		while (ovl_segment_bucket_count < atos(*end - *start))
			ovl_segment_bucket_count <<= 1;
	}

	ovl_segment_hash_mask = ovl_segment_bucket_count - 1;

	for(i = 0; i < ovl_segment_hash_mask; i++) {
		CIRCLEQ_INIT(&ovl_segment_buckets[i]);
		TAILQ_INIT(&ovl_vsegment_hashtable[i]);
	}
	simple_lock_init(&ovl_segment_bucket_lock, "ovl_segment_bucket_lock");
}

unsigned long
ovl_segment_hash(object, offset)
	ovl_object_t	object;
	vm_offset_t 	offset;
{
	Fnv32_t hash1 = fnv_32_buf(&object, (sizeof(&object)+offset)&ovl_segment_hash_mask, FNV1_32_INIT)%ovl_segment_hash_mask;
	Fnv32_t hash2 = (((unsigned long)object+(unsigned long)offset)&ovl_segment_hash_mask);
    return (hash1^hash2);
}

void
ovl_segment_insert(segment, object, offset)
	ovl_segment_t 	segment;
	ovl_object_t	object;
	vm_offset_t 	offset;
{
	register struct ovseglist *bucket;

	if(segment == NULL) {
		return;
	}
	if(segment->ovs_flags & SEG_ALLOCATED) {
		panic("ovl_segment_insert: already inserted");
	}

	segment->ovs_object = object;
	segment->ovs_offset = offset;

	bucket = &vm_segment_buckets[vm_segment_hash(object, offset)];

	simple_lock(&ovl_segment_bucket_lock);
	CIRCLEQ_INSERT_TAIL(bucket, segment, ovs_hashlist);
    simple_unlock(&ovl_segment_bucket_lock);

    CIRCLEQ_INSERT_TAIL(&object->ovo_ovseglist, segment, ovs_seglist);
    segment->ovs_flags |= SEG_ALLOCATED;

    object->ovo_segment_count++;
}

void
ovl_segment_remove(segment)
	register ovl_segment_t 	segment;
{
	register struct ovseglist 	*bucket;

	if(!(segment->ovs_flags & SEG_ALLOCATED)) {
		return;
	}

	bucket = &vm_segment_buckets[vm_segment_hash(segment->ovs_object, segment->ovs_offset)];

	simple_lock(&ovl_segment_bucket_lock);
	CIRCLEQ_REMOVE(bucket, segment, ovs_hashlist);
	simple_unlock(&ovl_segment_bucket_lock);

	CIRCLEQ_REMOVE(&segment->ovs_object->ovo_ovseglist, segment, ovs_seglist);

	segment->ovs_object->ovo_segment_count--;
	segment->ovs_flags &= ~SEG_ALLOCATED;
}

ovl_segment_t
ovl_segment_lookup(object, offset)
	ovl_object_t	object;
	vm_offset_t offset;
{
	register struct ovseglist 	*bucket;
	ovl_segment_t 				segment, first, last;

	bucket = &vm_segment_buckets[ovl_segment_hash(object, offset)];
	first = CIRCLEQ_FIRST(bucket);
	last = CIRCLEQ_LAST(bucket);

	simple_lock(&ovl_segment_bucket_lock);
	if(first->ovs_offset != last->ovs_offset) {
		if(first->ovs_offset >= offset) {
			segment = ovl_segment_search_next(object, offset);
			simple_unlock(&ovl_segment_bucket_lock);
			return (segment);
		}
		if(last->ovs_offset >= offset) {
			segment = ovl_segment_search_prev(object, offset);
			simple_unlock(&ovl_segment_bucket_lock);
			return (segment);
		}
	}
	if (first->ovs_offset == last->ovs_offset && first->ovs_object == object) {
		simple_unlock(&segment_bucket_lock);
		return (first);
	}
	simple_unlock(&ovl_segment_bucket_lock);
	return (NULL);
}

ovl_segment_t
ovl_segment_search_next(object, offset)
	ovl_object_t	object;
	vm_offset_t offset;
{
	register struct ovseglist 	*bucket;
	ovl_segment_t 				segment;

	bucket = &vm_segment_buckets[ovl_segment_hash(object, offset)];

	simple_lock(&ovl_segment_bucket_lock);
	for(segment = CIRCLEQ_FIRST(bucket); segment != NULL; segment = CIRCLEQ_NEXT(segment, ovs_hashlist)) {
		if (segment->ovs_object == object && segment->ovs_offset == offset) {
			simple_unlock(&ovl_segment_bucket_lock);
			return (segment);
		}
	}
	simple_unlock(&ovl_segment_bucket_lock);
	return (NULL);
}

ovl_segment_t
ovl_segment_search_prev(object, offset)
	ovl_object_t object;
	vm_offset_t offset;
{
	register struct ovseglist *bucket;
	ovl_segment_t segment;

	bucket = &vm_segment_buckets[ovl_segment_hash(object, offset)];

	simple_lock(&ovl_segment_bucket_lock);
	for (segment = CIRCLEQ_FIRST(bucket); segment != NULL; segment = CIRCLEQ_PREV(segment, ovs_hashlist)) {
		if (segment->ovs_object == object && segment->ovs_offset == offset) {
			simple_unlock(&ovl_segment_bucket_lock);
			return (segment);
		}
	}
	simple_unlock(&ovl_segment_bucket_lock);
	return (NULL);
}

/* vm segments */
u_long
ovl_vsegment_hash(osegment, vsegment)
	ovl_segment_t 	osegment;
	vm_segment_t 	vsegment;
{
	Fnv32_t hash1 = fnv_32_buf(&osegment, sizeof(&osegment), FNV1_32_INIT) % ovl_segment_hash_mask;
	Fnv32_t hash2 = fnv_32_buf(&vsegment, sizeof(&vsegment), FNV1_32_INIT) % ovl_segment_hash_mask;
	return (hash1 ^ hash2);
}

void
ovl_segment_insert_vm_segment(osegment, vsegment)
	ovl_segment_t 	osegment;
	vm_segment_t 	vsegment;
{
    struct vsegment_hash_head 	*vbucket;

    if(vsegment == NULL) {
        return;
    }

    vbucket = &ovl_vsegment_hashtable[ovl_vsegment_hash(osegment, vsegment)];
    osegment->ovs_vm_segment = vsegment;
    osegment->ovs_flags |= OVL_SEG_VM_SEG;
    osegment->ovs_vm_segment;

    TAILQ_INSERT_HEAD(vbucket, osegment, ovs_vsegment_hlist);
    ovl_vsegment_count++;
}

vm_segment_t
ovl_segment_lookup_vm_segment(osegment)
	ovl_segment_t 				osegment;
{
	register vm_segment_t 	  	vsegment;
    struct vsegment_hash_head 	*vbucket;

    vbucket = &ovl_vsegment_hashtable[ovl_vsegment_hash(osegment, vsegment)];
    TAILQ_FOREACH(osegment, vbucket, ovs_vsegment_hlist) {
    	if(vsegment == TAILQ_NEXT(osegment, ovs_vsegment_hlist)->ovs_vm_segment) {
    		vsegment = TAILQ_NEXT(osegment, ovs_vsegment_hlist)->ovs_vm_segment;
    		return (vsegment);
    	}
    }
    return (NULL);
}

void
ovl_segment_remove_vm_segment(vsegment)
	vm_segment_t 	  			vsegment;
{
	register ovl_segment_t 		osegment;
    struct vsegment_hash_head 	*vbucket;

    vbucket = &ovl_vsegment_hashtable[ovl_vsegment_hash(osegment, vsegment)];
	TAILQ_FOREACH(osegment, vbucket, ovs_vsegment_hlist) {
		if (vsegment == TAILQ_NEXT(osegment, ovs_vsegment_hlist)->ovs_vm_segment) {
			vsegment = TAILQ_NEXT(osegment, ovs_vsegment_hlist)->ovs_vm_segment;
			if (vsegment != NULL) {
				TAILQ_REMOVE(vbucket, osegment, ovs_vsegment_hlist);
				ovl_vsegment_count--;
			}
		}
	}
}
