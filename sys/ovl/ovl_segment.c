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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/fnv_hash.h>

#include <ovl/include/ovl.h>
#include <ovl/include/ovl_page.h>
#include <ovl/include/ovl_segment.h>
#include <ovl/include/ovl_map.h>
#include <vm/include/vm_pageout.h>

struct ovl_seglist				*ovl_segment_buckets;
int								ovl_segment_bucket_count = 0;	/* How big is array? */
int								ovl_segment_hash_mask;			/* Mask for hash function */
simple_lock_data_t				ovl_segment_bucket_lock;		/* lock for all segment buckets XXX */

struct ovl_seglist 				ovl_segment_list;
simple_lock_data_t				ovl_segment_list_lock;

long							ovl_first_segment;
long							ovl_last_segment;
vm_offset_t						ovl_first_logical_addr;
vm_offset_t						ovl_last_logical_addr;

struct ovl_vm_segment_hash_head	*ovl_vm_segment_hashtable;
long				           	ovl_vm_segment_count;
simple_lock_data_t				ovl_vm_segment_hash_lock;

static ovl_segment_t ovl_segment_search_next(ovl_object_t, vm_offset_t);
static ovl_segment_t ovl_segment_search_prev(ovl_object_t, vm_offset_t);

void
ovl_segment_startup(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	register ovl_segment_t		seg;
	register struct ovl_seglist	*bucket;
	vm_size_t					nsegments;
	vm_offset_t					la;
	int	i;

	simple_lock_init(&ovl_segment_list_lock, "ovl_segment_list_lock");
	simple_lock_init(&ovl_vm_segment_hash_lock, "ovl_vm_segment_hash_lock");

	CIRCLEQ_INIT(&ovl_segment_list);

	if (ovl_segment_bucket_count == 0) {
		ovl_segment_bucket_count = 1;
		while (ovl_segment_bucket_count < atos(*end - *start)) {
			ovl_segment_bucket_count <<= 1;
		}
	}

	ovl_segment_hash_mask = ovl_segment_bucket_count - 1;

	ovl_segment_buckets = (struct ovl_seglist *)pmap_overlay_bootstrap_alloc(ovl_segment_bucket_count * sizeof(struct ovl_seglist));
	bucket = ovl_segment_buckets;

	for (i = ovl_segment_bucket_count; i--;) {
			CIRCLEQ_INIT(bucket);
			bucket++;
	}

	for(i = 0; i < ovl_segment_hash_mask; i++) {
		CIRCLEQ_INIT(&ovl_segment_buckets[i]);
		TAILQ_INIT(&ovl_vm_segment_hashtable[i]);
	}

	simple_lock_init(&ovl_segment_bucket_lock, "ovl_segment_bucket_lock");

	*end = trunc_segment(*end);

	nsegments = (*end - *start + sizeof(struct ovl_segment)) / (SEGMENT_SIZE + sizeof(struct ovl_segment));

	ovl_first_segment = *start;
	ovl_first_segment += nsegments * sizeof(struct ovl_segment);
	ovl_first_segment = atos(round_segment(ovl_first_segment));
	ovl_last_segment = ovl_first_segment + nsegments - 1;

	ovl_first_logical_addr = stoa(ovl_first_segment);
	ovl_last_logical_addr  = stoa(ovl_last_segment) + SEGMENT_MASK;

	seg = (ovl_segment_t)pmap_overlay_bootstrap_alloc(nsegments * sizeof(struct ovl_segment));

	la = ovl_first_logical_addr;
	while (nsegments--) {
		seg->flags = 0;
		seg->object = NULL;
		seg->log_addr = la;
		seg++;
		la += SEGMENT_SIZE;
	}
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
	register struct ovl_seglist *bucket;

	if (segment == NULL) {
		return;
	}
	if (segment->flags & SEG_ALLOCATED) {
		panic("ovl_segment_insert: already inserted");
	}

	segment->object = object;
	segment->offset = offset;

	bucket = &ovl_segment_buckets[ovl_segment_hash(object, offset)];

	simple_lock(&ovl_segment_bucket_lock);
	CIRCLEQ_INSERT_TAIL(bucket, segment, hashlist);
    simple_unlock(&ovl_segment_bucket_lock);

    CIRCLEQ_INSERT_TAIL(&object->seglist, segment, seglist);
    segment->flags |= SEG_ALLOCATED;

    object->segment_count++;
}

void
ovl_segment_remove(segment)
	register ovl_segment_t 	segment;
{
	register struct ovl_seglist 	*bucket;

	if(!(segment->flags & SEG_ALLOCATED)) {
		return;
	}

	bucket = &ovl_segment_buckets[ovl_segment_hash(segment->object, segment->offset)];

	simple_lock(&ovl_segment_bucket_lock);
	CIRCLEQ_REMOVE(bucket, segment, hashlist);
	simple_unlock(&ovl_segment_bucket_lock);

	CIRCLEQ_REMOVE(&segment->object->seglist, segment, seglist);

	segment->object->segment_count--;
	segment->flags &= ~SEG_ALLOCATED;
}

ovl_segment_t
ovl_segment_lookup(object, offset)
	ovl_object_t	object;
	vm_offset_t offset;
{
	register struct ovl_seglist 	*bucket;
	ovl_segment_t 				segment, first, last;

	bucket = &ovl_segment_buckets[ovl_segment_hash(object, offset)];
	first = CIRCLEQ_FIRST(bucket);
	last = CIRCLEQ_LAST(bucket);

	simple_lock(&ovl_segment_bucket_lock);
	if (first->object == object) {
		if (first->offset == last->offset) {
			simple_unlock(&ovl_segment_bucket_lock);
			return (first);
		}
	}
	if (last->object == object) {
		if (first->offset == last->offset) {
			simple_unlock(&ovl_segment_bucket_lock);
			return (last);
		}
	}

	if (first->offset != last->offset) {
		if (first->offset >= offset) {
			segment = ovl_segment_search_next(object, offset);
			simple_unlock(&ovl_segment_bucket_lock);
			return (segment);
		}
		if (last->offset <= offset) {
			segment = ovl_segment_search_prev(object, offset);
			simple_unlock(&ovl_segment_bucket_lock);
			return (segment);
		}
	}
	simple_unlock(&ovl_segment_bucket_lock);
	return (NULL);
}

static ovl_segment_t
ovl_segment_search_next(object, offset)
	ovl_object_t	object;
	vm_offset_t offset;
{
	register struct ovl_seglist 	*bucket;
	ovl_segment_t 				segment;

	bucket = &ovl_segment_buckets[ovl_segment_hash(object, offset)];

	simple_lock(&ovl_segment_bucket_lock);
	CIRCLEQ_FOREACH(segment, bucket, hashlist) {
		if (segment->object == object && segment->offset == offset) {
			simple_unlock(&ovl_segment_bucket_lock);
			return (segment);
		}
	}
	simple_unlock(&ovl_segment_bucket_lock);
	return (NULL);
}

static ovl_segment_t
ovl_segment_search_prev(object, offset)
	ovl_object_t object;
	vm_offset_t offset;
{
	register struct ovl_seglist *bucket;
	ovl_segment_t segment;

	bucket = &ovl_segment_buckets[ovl_segment_hash(object, offset)];

	simple_lock(&ovl_segment_bucket_lock);
	CIRCLEQ_FOREACH_REVERSE(segment, bucket, hashlist) {
		if (segment->object == object && segment->offset == offset) {
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
    struct ovl_vm_segment_hash_head 	*vbucket;

    if(vsegment == NULL) {
        return;
    }

    vbucket = &ovl_vm_segment_hashtable[ovl_vsegment_hash(osegment, vsegment)];
    osegment->vm_segment = vsegment;
    osegment->flags |= OVL_SEG_VM_SEG;

    ovl_vm_segment_hash_lock();
    TAILQ_INSERT_HEAD(vbucket, osegment, vm_segment_hlist);
    ovl_vm_segment_hash_unlock();

    ovl_vm_segment_count++;
}

vm_segment_t
ovl_segment_lookup_vm_segment(osegment)
	ovl_segment_t 				osegment;
{
	register vm_segment_t 	  	vsegment;
    struct ovl_vm_segment_hash_head 	*vbucket;

    vbucket = &ovl_vm_segment_hashtable[ovl_vsegment_hash(osegment, vsegment)];
    ovl_vm_segment_hash_lock();
    TAILQ_FOREACH(osegment, vbucket, vm_segment_hlist) {
    	if(vsegment == TAILQ_NEXT(osegment, vm_segment_hlist)->vm_segment) {
    		vsegment = TAILQ_NEXT(osegment, vm_segment_hlist)->vm_segment;
    		 ovl_vm_segment_hash_unlock();
    		return (vsegment);
    	}
    }
    ovl_vm_segment_hash_unlock();
    return (NULL);
}

void
ovl_segment_remove_vm_segment(vsegment)
	vm_segment_t 	  			vsegment;
{
	register ovl_segment_t 		osegment;
    struct ovl_vm_segment_hash_head 	*vbucket;

    vbucket = &ovl_vm_segment_hashtable[ovl_vsegment_hash(osegment, vsegment)];
	TAILQ_FOREACH(osegment, vbucket, vm_segment_hlist) {
		if (vsegment == TAILQ_NEXT(osegment, vm_segment_hlist)->vm_segment) {
			vsegment = TAILQ_NEXT(osegment, vm_segment_hlist)->vm_segment;
			if (vsegment != NULL) {
				TAILQ_REMOVE(vbucket, osegment, vm_segment_hlist);
				ovl_vm_segment_count--;
			}
		}
	}
}
