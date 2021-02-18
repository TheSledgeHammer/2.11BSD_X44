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

/*
 * A Simple Slab Allocator.
 * Slabs treats each bucket in kmembucket as pool with "x" slots available.
 *
 * Slots are determined by the bucket index and the object size being allocated.
 * All slot data and other misc information is held within slab_metadata.
 *
 * Slab allows for small and large objects by splitting kmembuckets into two.
 * Any objects with a bucket index less than 10 are flagged as small, whille objects
 * greater than or equal to 10 are flagged as large.
 */

#include <devel/vm/include/vm_slab.h>

struct slab_head *slab_buckets;

/*
 * TODO:
 * - initialization of slab allocator: in kern_malloc.c?
 * - slab cache
 * - detecting the state of each slab
 *    - dynamic to each segment/page
 */
void
slab_create(kbp, slabs)
    struct kmembuckets *kbp;
    struct slab_head *slabs;
{
    register int    i;

    CIRCLEQ_INIT(&slab_cache_list);
    CIRCLEQ_INIT(&slab_list);
    slab_count = 0;

    for (i = 0; i <  MINBUCKET + 16; i++) {
    	CIRCLEQ_INIT(&slab_buckets[i]);
    	/*  Change to:
    	 *  CIRCLEQ_INIT(bucket[i].kb_slabs);
    	 *  slab_buckets[i] = bucket[i].kb_slabs;
    	*/
    }
}

/* setup slab metadata information */
void
slabmeta(meta, size)
	slab_metadata_t meta;
    vm_size_t   size;
{
    meta->sm_slots = SLOTS(size);
    meta->sm_freeslots = SLOTSFREE(size);
    if(LARGE_OBJECT(size) != 0) {
        meta->sm_type = SLAB_SMALL;
    } else {
        meta->sm_type = SLAB_LARGE;
    }
}

/* TODO:
 * - slabs: using malloc seems silly.
 *      - Could make sense in the vm. Like FreeBSD.
 *      - slab uses malloc -> vm uses slab -> malloc relies on vm
 */
void
slab_insert(slab, size)
    slab_t  slab;
    long    size;
{
    struct slab_head       *slabs;
    register slab_entry_t  entry;

    if(slab == NULL) {
        return;
    }
    slab->s_size = size;
    slab->s_flags = flags;
    slabmeta(slab->s_meta, size);

    slabs = &slab_buckets[BUCKETINDX(size)];
    entry = (slab_entry_t) malloc(sizeof(*entry), M_VMSLAB, NULL);
    entry->se_slab = slab;

    if(slab->s_meta->sm_type == SLAB_SMALL) {
        CIRCLEQ_INSERT_HEAD(slabs, entry, se_link);
    } else {
        CIRCLEQ_INSERT_TAIL(slabs, entry, se_link);
    }
}

void
slab_remove(size)
	long    size;
{
	struct slab_head        *slabs;
    register slab_entry_t   entry;
    register slab_t         slab;

    slabs = &slab_buckets[BUCKETINDX(size)];
    if(slab->s_meta == SLAB_SMALL) {
    	for(entry = CIRCLEQ_FIRST(slabs); entry != NULL; entry = CIRCLEQ_NEXT(entry, se_link)) {
    		slab = entry->se_slab;
    		if(slab->s_size == size) {
    			goto remove;
    		}
    	}
    }
    if(slab->s_meta == SLAB_LARGE) {
    	for(entry = CIRCLEQ_LAST(slabs); entry != NULL; entry = CIRCLEQ_NEXT(entry, se_link)) {
    		slab = entry->se_slab;
    		if(slab->s_size == size) {
    			goto remove;
    		}
    	}
    }

remove:
	CIRCLEQ_REMOVE(slabs, entry, se_link);
	FREE(slab, M_VMSLAB);
}

slab_t
slab_lookup(size)
	long    size;
{
	struct slab_head        *slabs;
    register slab_entry_t   entry;
    register slab_t         slab;

    slabs = &slab_buckets[BUCKETINDX(size)];
    if(slab->s_meta == SLAB_SMALL) {
    	for(entry = CIRCLEQ_FIRST(slabs); entry != NULL; entry = CIRCLEQ_NEXT(entry, se_link)) {
    		slab = entry->se_slab;
    		if(slab->s_size == size) {
    			return (slab);
    		}
    	}
    }
    if(slab->s_meta == SLAB_LARGE) {
    	for(entry = CIRCLEQ_LAST(slabs); entry != NULL; entry = CIRCLEQ_NEXT(entry, se_link)) {
    		slab = entry->se_slab;
    		if(slab->s_size == size) {
    			return (slab);
    		}
    	}
    }
    return (NULL);
}
