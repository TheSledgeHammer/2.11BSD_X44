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

struct vm_segment_hash_head  	*vm_segment_hashtable;
int								vm_segment_bucket_count = 0;	/* How big is array? */
int								vm_segment_hash_mask;			/* Mask for hash function */

unsigned long
alloc_hash(segment)
	vm_segment_t segment;
{
	Fnv32_t seg_hash = fnv_32_buf(&segment, sizeof(&segment), FNV1_32_INIT)%vm_segment_hash_mask;
	return (seg_hash);
}

void
vm_segment_init(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	register int	i;

	if (vm_segment_bucket_count == 0) {
		vm_segment_bucket_count = 1;
		while (vm_segment_bucket_count < atop(*end - *start))
			vm_segment_bucket_count <<= 1;
	}

	vm_segment_hash_mask = vm_segment_bucket_count - 1;

	for(i = 0; i < vm_segment_hash_mask; i++) {
		CIRCLEQ_INIT(&vm_segment_hashtable[i]);
	}
}

void
_vm_segment_allocate(size, segment)
	vm_size_t				size;
	register vm_segment_t 	segment;
{
	RB_INIT(segment->sg_pgtable);

	segment->sg_size = size;

	if(size > alloc_hash(segment)) {
		CIRCLEQ_INSERT_TAIL(&vm_segment_list, segment, sg_list);
	} else {
		CIRCLEQ_INSERT_HEAD(&vm_segment_list, segment, sg_list);
	}
}

vm_segment_t
vm_segment_allocate(size)
	vm_size_t		size;
{
	register vm_segment_t result;

	result = (vm_segment_t)malloc((u_long)sizeof(*result), M_VMSEG, M_WAITOK);
	_vm_segment_allocate(size, result);

	return (result);
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

vm_segment_t
vm_segment_lookup(object, offset)
	vm_object_t	object;
	vm_offset_t offset;
{
	register struct vm_segment_hash_head 	*bucket;
	register vm_segment_hash_entry_t		entry;
	vm_segment_t 							segment;

	bucket = &vm_segment_hashtable[vm_segment_hash(object, offset)];
	entry = CIRCLEQ_FIRST(bucket);

	CIRCLEQ_FOREACH(entry, bucket, sge_hlinks) {
		if(CIRCLEQ_NEXT(entry, sge_hlinks) ) {
			segment = entry->sge_segment;

			return (segment);
		}
	}
	return (NULL);
}

void
vm_segment_enter(segment, object, offset)
	vm_segment_t 	segment;
	vm_object_t		object;
	vm_offset_t 	offset;
{
	register struct vm_segment_hash_head *bucket;
	register vm_segment_hash_entry_t	entry;

	if(segment == NULL) {
		return;
	}

	bucket = &vm_segment_hashtable[vm_segment_hash(object, offset)];
	entry = (vm_segment_hash_entry_t)malloc((u_long)sizeof *entry);
	entry->sge_segment = segment;
	segment->sg_flags = SEG_TABLED;

	CIRCLEQ_INSERT_TAIL(bucket, entry, sge_hlinks);
}

void
vm_segment_remove(segment)
	vm_segment_t 	segment;
{
	register struct vm_segment_hash_head 	*bucket;
	register vm_segment_hash_entry_t		entry;

	bucket = &vm_segment_hashtable[vm_segment_hash(segment->sg_object, segment->sg_offset)];

	for(entry = CIRCLEQ_FIRST(bucket); entry != NULL; entry = CIRCLEQ_NEXT(entry, sge_hlinks)) {


	}
}

vm_segment_hash_entry_t
vm_segment_search_next(entry, object)
	vm_segment_hash_entry_t entry;
	vm_object_t				object;
{
	register struct vm_segment_hash_head *bucket;
	register vm_segment_hash_entry_t cur;
	register vm_segment_hash_entry_t first = CIRCLEQ_FIRST(bucket);
	register vm_segment_hash_entry_t last = CIRCLEQ_LAST(bucket);

	bucket = &vm_segment_hashtable[vm_segment_hash(object)];

	CIRCLEQ_FOREACH(entry, bucket, sge_hlinks) {
		cur = CIRCLEQ_NEXT(first, sge_hlinks);
		if(entry == cur) {
			return (entry);
		}
	}
	return (NULL);
}
