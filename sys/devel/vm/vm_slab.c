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
 * Any objects with a bucket index less than 10 are flagged as small, while objects
 * greater than or equal to 10 are flagged as large.
 */

#include <sys/malloc.h>
#include <devel/sys/malloctypes.h>
#include <devel/vm/include/vm_slab.h>

struct slablist 	*slab_buckets;
simple_lock_data_t	slab_bucket_lock;

/*
 * TODO:
 * - initialization of slab allocator: in kern_malloc.c?
 * - slab cache
 * - detecting the state of each slab
 *    - dynamic to each segment/page
 */
void
slab_create()
{
    CIRCLEQ_INIT(&slab_cache_list);
    CIRCLEQ_INIT(&slab_list);
    slab_count = 0;
    simple_lock_init(&slab_bucket_lock, "slab_bucket_lock");
}

/*
 * TODO:
 * - Improve on allocation & freeing of slabs
 * - Take in account slabs that are EMPTY, PARTIAL or FULL
 * - Allocating to neighboring slab if designated slab is full or doesn't
 * have enough space.
 */
void
slab_malloc(size, type, flags)
	u_long  size;
	int		type, flags;
{
	slab_t slab;
	slab_metadata_t meta;

	if(slab == NULL) {
		meta = slab->s_meta;
		/* test if free bucket slots is between 25% to 75% */
		if(meta->sm_freeslots >= (meta->sm_bslots * 0.25) || meta->sm_freeslots <= (meta->sm_bslots * 0.75)) {
			slab->s_flags = SLAB_PARTIAL;
		/* test if free bucket slots is greater than 75% */
		} else if(meta->sm_freeslots > (meta->sm_bslots * 0.75)) {
			slab->s_flags = SLAB_FULL;
		} else {
			slab->s_flags = SLAB_EMPTY;
		}

		slab = (slab_t) malloc(sizof(*slab), type, flags);
	} else {
		slab_insert(slab, size);
	}
}

void
slab_free(addr, type)
	void *addr;
	int	type;
{
	slab_t slab;

	slab = slab_lookup(addr);
	if(slab) {
		slab_remove(addr);
		free(addr, type);
	}
}

/* slab metadata information */
void
slabmeta(meta, size)
	slab_metadata_t meta;
	u_long   		size;
{
	u_long indx = BUCKETINDX(size);
	u_long bsize = BUCKETSIZE(indx);

    meta->sm_bslots = BUCKET_SLOTS(bsize);
    meta->sm_aslots = ALLOCATED_SLOTS(size);
    meta->sm_freeslots = SLOTSFREE(meta->sm_bslots, meta->sm_aslots);
    if(meta->sm_freeslots < 0) {
    	meta->sm_freeslots = 0;
    }
    if(index < 10) {
        meta->sm_type = SLAB_SMALL;
    } else {
    	if(index >= 10) {
    		meta->sm_type = SLAB_LARGE;
    	}
    }
}

void
slab_insert(slab, size)
    slab_t  slab;
    long    size;
{
    register struct slablist  *slabs;

    if(slab == NULL) {
        return;
    }

    slab->s_size = size;
    slab->s_flags = flags;
    slabmeta(slab->s_meta, size);

    slabs = &slab_buckets[BUCKETINDX(size)];
    simple_lock(&slab_bucket_lock);
    if(slab->s_meta->sm_type == SLAB_SMALL) {
        CIRCLEQ_INSERT_HEAD(slabs, slab, s_list);
    } else {
        CIRCLEQ_INSERT_TAIL(slabs, slab, s_list);
    }
    simple_unlock(&slab_bucket_lock);
    slab_count++;
}

void
slab_remove(size)
	long    size;
{
	struct 	 slab_head  *slabs;
    register slab_t     slab;

    slabs = &slab_buckets[BUCKETINDX(size)];
    simple_lock(&slab_bucket_lock);
	CIRCLEQ_REMOVE(slabs, slab, s_list);
	simple_unlock(&slab_bucket_lock);
	slab_count--;
}

slab_t
slab_lookup(size)
	long    size;
{
	register struct slab_head   *slabs;
    register slab_t         	slab;

    slabs = &slab_buckets[BUCKETINDX(size)];
    simple_lock(&slab_bucket_lock);
    if(slab->s_meta == SLAB_SMALL) {
    	for(slab = CIRCLEQ_FIRST(slabs); slab != NULL; slab = CIRCLEQ_NEXT(slab, s_list)) {
    		if(slab->s_size == size) {
    			simple_unlock(&slab_bucket_lock);
    			return (slab);
    		}
    	}
    }
    if(slab->s_meta == SLAB_LARGE) {
    	for(slab = CIRCLEQ_LAST(slabs); slab != NULL; slab = CIRCLEQ_NEXT(slab, s_list)) {
    		if(slab->s_size == size) {
    			simple_unlock(&slab_bucket_lock);
    			return (slab);
    		}
    	}
    }
    simple_unlock(&slab_bucket_lock);
    return (NULL);
}
