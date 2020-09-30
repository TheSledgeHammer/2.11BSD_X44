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
#include <devel/vm/segment/vm_page_table.h>
#include <devel/vm/segment/vm_segment.h>

struct vm_page_table_hash_root 	vm_pagetable_buckets;
int								vm_pagetable_bucket_count = 0;	/* How big is array? */
int								vm_pagetable_hash_mask;			/* Mask for hash function */

int
vm_page_table_rb_compare(pt1, pt2)
	vm_page_table_t pt1, pt2;
{
	if(pt1->pt_offset < pt2->pt_offset) {
		return(-1);
	} else if(pt1->pt_offset > pt2->pt_offset) {
		return(1);
	}
	return (0);
}

RB_PROTOTYPE(pttree, vm_page_table, pt_tree, vm_page_table_rb_compare);
RB_GENERATE(pttree, vm_page_table, pt_tree, vm_page_table_rb_compare);
RB_PROTOTYPE(vm_page_table_hash_root, vm_page_table_hash_entry, pte_hlinks, vm_page_table_rb_compare);
RB_GENERATE(vm_page_table_hash_root, vm_page_table_hash_entry, pte_hlinks, vm_page_table_rb_compare);

void
vm_page_table_init(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	register int i;

	if (vm_pagetable_bucket_count == 0) {
		vm_pagetable_bucket_count = 1;
		while (vm_pagetable_bucket_count < atop(*end - *start))
			vm_pagetable_bucket_count <<= 1;
	}

	vm_pagetable_hash_mask = vm_pagetable_bucket_count - 1;

	for(i = 0; i < vm_pagetable_hash_mask; i++) {
		RB_INIT(&vm_pagetable_buckets[i]);
	}
}

void
_vm_page_table_allocate(size, pagetable)
	vm_size_t					size;
	register vm_page_table_t 	pagetable;
{
	RB_INIT(&pagetable->pt_pagetree);

	pagetable->pt_size = size;

	RB_INSERT(pttree, &vm_pagetable_tree, pagetable);
}

vm_page_table_t
vm_page_table_allocate(size)
	vm_size_t	size;
{
	register vm_page_table_t result;

	result = (vm_page_table_t)malloc((u_long)sizeof(*result), M_VMPGTABLE, M_WAITOK);

	_vm_page_table_allocate(size, result);

	return (result);
}

unsigned long
vm_page_table_hash(segment, offset)
    vm_segment_t    segment;
    vm_offset_t     offset;
{
    Fnv32_t hash1 = fnv_32_buf(&segment, (sizeof(&segment) + offset)&vm_pagetable_hash_mask, FNV1_32_INIT)%vm_pagetable_hash_mask;
    Fnv32_t hash2 = (((unsigned long)segment+(unsigned long)offset)&vm_pagetable_hash_mask);
    return (hash1^hash2);
}

/* insert page table into segment */
void
vm_page_table_enter(pagetable, segment, offset)
	register vm_page_table_t 	pagetable;
	register vm_segment_t		segment;
	register vm_offset_t		offset;
{
	register struct vm_page_table_hash_root *bucket;

	pagetable->pt_segment = segment;
	pagetable->pt_offset = offset;

	bucket = &vm_pagetable_buckets[vm_page_table_hash(segment, offset)];

	RB_INSERT(vm_page_table_hash_root, bucket, pagetable);
}

/* remove page table from segment */
void
vm_page_table_remove(pagetable)
	register vm_page_table_t pagetable;
{
	register struct vm_page_table_hash_root *bucket = &vm_pagetable_buckets[vm_page_table_hash(pagetable->pt_segment, pagetable->pt_offset)];

	if(bucket) {
		RB_REMOVE(seg_tree, bucket, pagetable);
	}
}

/* lookup page table from segment/offset pair */
vm_page_table_t
vm_page_table_lookup(segment, offset)
	register vm_segment_t	segment;
	register vm_offset_t	offset;
{
	register struct vm_page_table_hash_root *bucket;
	register vm_page_table_t 				pagetable;

	bucket = &vm_pagetable_buckets[vm_page_table_hash(segment, offset)];
//	RB_FOREACH(pagetable, vm_page_table_hash_root, bucket) {

//	}

	for (pagetable = RB_FIRST(vm_page_table_hash_root, bucket); pagetable != NULL; pagetable = RB_NEXT(vm_page_table_hash_root, bucket, pagetable)) {
		if ((RB_FIND(vm_page_table_hash_root, bucket, pagetable)->pte_pgtable == pagetable)) {
			pagetable = RB_FIND(vm_page_table_hash_root, bucket, pagetable)->pte_pgtable;
			if(pagetable->pt_segment == segment && pagetable->pt_offset == offset) {
				return (pagetable);
			}
		}
	}
	return (NULL);
}
