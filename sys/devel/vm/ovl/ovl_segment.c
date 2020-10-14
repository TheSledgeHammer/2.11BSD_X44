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
//#include <devel/vm/ovl/ovl_segment.h>
//#include <devel/vm/ovl/ovl_object.h>

struct ovseglist	*ovl_segment_buckets;
int					ovl_segment_bucket_count = 0;	/* How big is array? */
int					ovl_segment_hash_mask;			/* Mask for hash function */
simple_lock_data_t	ovl_bucket_lock;				/* lock for all buckets XXX */

struct ovseglist  	ovl_segment_list;
simple_lock_data_t	ovl_segment_list_lock;
vm_size_t			segment_mask;
int					segment_shift;

void
ovl_set_segment_size()
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
ovl_segment_init(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	int	i;

	simple_lock_init(&ovl_segment_list_lock);

	CIRCLEQ_INIT(&ovl_segment_list);

	if (ovl_segment_bucket_count == 0) {
		ovl_segment_bucket_count = 1;
		while (ovl_segment_bucket_count < atop(*end - *start))
			ovl_segment_bucket_count <<= 1;
	}

	ovl_segment_hash_mask = ovl_segment_bucket_count - 1;

	for(i = 0; i < ovl_segment_hash_mask; i++) {
		CIRCLEQ_INIT(&ovl_segment_buckets[i]);
	}
	simple_lock_init(&ovl_bucket_lock);
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
	if(segment->ovsg_flags & SEG_ALLOCATED) {
		panic("ovl_segment_insert: already inserted");
	}

	segment->ovsg_object = object;
	segment->ovsg_offset = offset;

	bucket = &vm_segment_buckets[vm_segment_hash(object, offset)];

	simple_lock(&ovl_bucket_lock);
	CIRCLEQ_INSERT_TAIL(bucket, segment, ovsg_hashlist);
    simple_unlock(&ovl_bucket_lock);

    CIRCLEQ_INSERT_TAIL(&object->ovo_ovseglist, segment, ovsg_seglist);
    segment->ovsg_flags |= SEG_ALLOCATED;
}

void
ovl_segment_remove(segment)
	register ovl_segment_t 	segment;
{
	register struct ovseglist 	*bucket;

	if(!(segment->ovsg_flags & SEG_ALLOCATED)) {
		return;
	}

	bucket = &vm_segment_buckets[vm_segment_hash(segment->ovsg_object, segment->ovsg_offset)];

	simple_lock(&ovl_bucket_lock);
	CIRCLEQ_REMOVE(bucket, segment, ovsg_hashlist);
	simple_unlock(&ovl_bucket_lock);

	CIRCLEQ_REMOVE(&segment->ovsg_object->ovo_ovseglist, segment, ovsg_seglist);

	segment->ovsg_flags &= ~SEG_ALLOCATED;
}

ovl_segment_t
ovl_segment_lookup(object, offset)
	ovl_object_t	object;
	vm_offset_t 	offset;
{
	register struct ovseglist 	*bucket;
	ovl_segment_t 				segment;

	bucket = &vm_segment_buckets[vm_segment_hash(object, offset)];

	simple_lock(&ovl_bucket_lock);
	for(segment = CIRCLEQ_FIRST(bucket); segment != NULL; segment = CIRCLEQ_NEXT(segment, ovsg_hashlist)) {
		if(segment->ovsg_object == object && segment->ovsg_offset == offset) {
			simple_unlock(&ovl_bucket_lock);
			return (segment);
		}
	}
	simple_unlock(&ovl_bucket_lock);
	return (NULL);
}

ovl_segment_t
ovl_segment_alloc(object, offset)
	ovl_object_t	object;
	vm_offset_t		offset;
{
	register ovl_segment_t 	segment;

	return (segment);
}

void
ovl_segment_free(segment)
	register ovl_segment_t 	segment;
{
	ovl_segment_remove(segment);
	if(segment->ovsg_flags & SEG_ACTIVE) {
		segment->ovsg_flags &= ~SEG_ACTIVE;
	}
	if(segment->ovsg_flags & SEG_INACTIVE) {
		segment->ovsg_flags &= ~SEG_INACTIVE;
	}
}
