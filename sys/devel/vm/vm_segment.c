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

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_segment.h>

struct seglist  	*vm_segment_buckets;
int					vm_segment_bucket_count = 0;	/* How big is array? */
int					vm_segment_hash_mask;			/* Mask for hash function */
simple_lock_data_t	segment_bucket_lock;			/* lock for all segment buckets XXX */

boolean_t 			vm_segment_startup_initialized;

struct seglist		vm_segment_list;
struct seglist		vm_segment_list_active;
struct seglist		vm_segment_list_inactive;
simple_lock_data_t	vm_segment_list_lock;
simple_lock_data_t	vm_segment_list_activity_lock;

long				first_segment;
long				last_segment;
vm_offset_t			first_logical_addr;
vm_offset_t			last_logical_addr;
vm_size_t			segment_mask;
int					segment_shift;

void
vm_set_segment_size()
{
	if (cnt.v_segment_size == 0)
		cnt.v_segment_size = DEFAULT_SEGMENT_SIZE;
	segment_mask = cnt.v_segment_size - 1;
	if ((segment_mask & cnt.v_segment_size) != 0)
		panic("vm_set_segment_size: segment size not a power of two");
	for (segment_shift = 0;; segment_shift++)
		if ((1 << segment_shift) == cnt.v_segment_size)
			break;
}

void
vm_segment_startup(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	register vm_segment_t		seg;
	register struct seglist		*bucket;
	vm_size_t					nsegments;
	int							i;
	vm_offset_t					la;

	simple_lock_init(&vm_segment_list_lock);
	simple_lock_init(&vm_segment_list_activity_lock);

	CIRCLEQ_INIT(&vm_segment_list);
	CIRCLEQ_INIT(&vm_segment_list_active);
	CIRCLEQ_INIT(&vm_segment_list_inactive);

	if (vm_segment_bucket_count == 0) {
		vm_segment_bucket_count = 1;
		while (vm_segment_bucket_count < atos(*end - *start))
			vm_segment_bucket_count <<= 1;
	}

	vm_segment_hash_mask = vm_segment_bucket_count - 1;

	vm_segment_buckets = (struct seglist *)pmap_bootstrap_alloc(vm_segment_bucket_count * sizeof(struct seglist));
	bucket = vm_segment_buckets;

	for(i = vm_segment_bucket_count; i--;) {
		CIRCLEQ_INIT(bucket);
		bucket++;
	}

	simple_lock_init(&segment_bucket_lock);

	/*
	 *	Truncate the remainder of memory to our segment size.
	 */
	*end = trunc_segment(*end);

	/*
 	 *	Compute the number of segments.
	 */
	cnt.v_segment_count = nsegments = (*end - *start + sizeof(struct vm_segment)) / (SEGMENT_SIZE + sizeof(struct vm_segment));

	/*
	 *	Record the extent of physical memory that the
	 *	virtual memory system manages. taking into account the overhead
	 *	of a segment structure per segment.
	 */
	first_segment = *start;
	first_segment += nsegments*sizeof(struct vm_segment);
	first_segment = atos(round_segment(first_segment));
	last_segment = first_segment + nsegments - 1;

	first_logical_addr = stoa(first_segment);
	last_logical_addr  = stoa(last_segment) + SEGMENT_MASK;

	seg = vm_segment_array = (vm_segment_t)pmap_bootstrap_alloc(nsegments * sizeof(struct vm_segment));

	/*
	 *	Allocate and clear the mem entry structures.
	 */

	la = first_logical_addr;
	while (nsegments--) {
		seg->sg_flags = 0;
		seg->sg_object = NULL;
		seg++;
		la += SEGMENT_SIZE;
	}

	seg->sg_resident_page_count = 0;

	vm_segment_startup_initialized = TRUE;
}

unsigned long
vm_segment_hash(object, offset)
	vm_object_t	object;
	vm_offset_t offset;
{
	Fnv32_t hash1 = fnv_32_buf(&object, (sizeof(&object)+offset)&vm_segment_hash_mask, FNV1_32_INIT)%vm_segment_hash_mask;
	Fnv32_t hash2 = (((unsigned long)object+(unsigned long)offset)&vm_segment_hash_mask);
    return (hash1^hash2);
}

void
vm_segment_insert(segment, object, offset)
	vm_segment_t 	segment;
	vm_object_t		object;
	vm_offset_t 	offset;
{
	register struct seglist *bucket;

	if (segment->sg_flags & SEG_ALLOCATED) {
		panic("vm_segment_insert: already inserted");
	}

	segment->sg_object = object;
	segment->sg_offset = offset;

	bucket = &vm_segment_buckets[vm_segment_hash(object, offset)];

	simple_lock(&segment_bucket_lock);
	CIRCLEQ_INSERT_TAIL(bucket, segment, sg_hlist);
	simple_unlock(&segment_bucket_lock);

	CIRCLEQ_INSERT_TAIL(&object->seglist, segment, sg_list);
	segment->sg_flags |= SEG_ALLOCATED;
}

void
vm_segment_remove(segment)
	register vm_segment_t 	segment;
{
	register struct seglist *bucket;

	VM_SEGMENT_CHECK(segment);

	if (!(segment->sg_flags & SEG_ALLOCATED)) {
		return;
	}

	bucket = &vm_segment_buckets[vm_segment_hash(segment->sg_object, segment->sg_offset)];

	simple_lock(&segment_bucket_lock);
	CIRCLEQ_REMOVE(bucket, segment, sg_hlist);
	simple_unlock(&segment_bucket_lock);

	CIRCLEQ_REMOVE(&segment->sg_object->seglist, segment, sg_list);

	segment->sg_flags &= ~SEG_ALLOCATED;
}

vm_segment_t
vm_segment_lookup(object, offset)
	vm_object_t	object;
	vm_offset_t offset;
{
	register struct seglist 	*bucket;
	vm_segment_t 				segment;

	bucket = &vm_segment_buckets[vm_segment_hash(object, offset)];

	simple_lock(&segment_bucket_lock);
	for(segment = CIRCLEQ_FIRST(bucket); segment != NULL; segment = CIRCLEQ_NEXT(segment, sg_hlist)) {
		if (segment->sg_object == object && segment->sg_offset == offset) {
			simple_unlock(&segment_bucket_lock);
			return (segment);
		}
	}
	simple_unlock(&segment_bucket_lock);
	return (NULL);
}

vm_segment_t
vm_segment_alloc(object, offset)
	vm_object_t	object;
	vm_offset_t	offset;
{
	register vm_segment_t seg;

	simple_lock(&vm_segment_list_activity_lock);
	if(CIRCLEQ_FIRST(&vm_segment_list) == NULL) {
		simple_unlock(&vm_segment_list_activity_lock);
		return (NULL);
	}
	seg = CIRCLEQ_FIRST(&vm_segment_list);
	CIRCLEQ_REMOVE(&vm_segment_list, seg, sg_list);

	cnt.v_segment_count--;
	simple_unlock(&vm_segment_list_activity_lock);

	VM_SEGMENT_INIT(seg, object, offset)

	return (seg);
}

void
vm_segment_free(object, segment)
	vm_object_t	object;
	register vm_segment_t segment;
{
	vm_segment_remove(object, segment);
	if(segment->sg_flags & SEG_ACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_active, segment, sg_list);
		segment->sg_flags &= SEG_ACTIVE;
		cnt.v_segment_active_count--;
	}
	if(segment->sg_flags & SEG_INACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_inactive, segment, sg_list);
		segment->sg_flags &= SEG_INACTIVE;
		cnt.v_segment_inactive_count--;
	}
}

void
vm_segment_deactivate(segment)
	register vm_segment_t segment;
{
	VM_SEGMENT_CHECK(segment);

	if(segment->sg_flags & SEG_ACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_active, segment, sg_list);
		CIRCLEQ_INSERT_TAIL(&vm_segment_list_inactive, segment, sg_list);
		segment->sg_flags &= SEG_ACTIVE;
		segment->sg_flags |= SEG_INACTIVE;
		cnt.v_segment_active_count--;
		cnt.v_segment_inactive_count++;
	}
}

void
vm_segment_activate(segment)
	register vm_segment_t segment;
{
	VM_SEGMENT_CHECK(segment);

	if(segment->sg_flags & SEG_INACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_inactive, segment, sg_list);
		cnt.v_segment_inactive_count--;
		segment->sg_flags &= SEG_INACTIVE;
	}
	if(segment->sg_flags & SEG_ACTIVE) {
		panic("vm_segment_activate: already active");
	}
	CIRCLEQ_INSERT_TAIL(&vm_segment_list_active, segment, sg_list);
	segment->sg_flags |= SEG_ACTIVE;
	cnt.v_segment_active_count++;
}
