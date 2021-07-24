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
 * Slab_metadata is responsible for retaining all slot related data and some slab information
 * (i.e. slab state (partial, full or empty) and slab size (large or small))
 *
 * Slab allows for small and large objects by splitting kmembuckets into two.
 * Any objects with a bucket index less than 10 are flagged as small, while objects
 * greater than or equal to 10 are flagged as large.
 */

/*
 * TODO:
 * - allocate slab_cache
 * - setup slab routines to use kmembuckets instead of BUCKETINDX(size)
 */

#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/fnv_hash.h>
#include <devel/sys/malloctypes.h>
#include <devel/vm/include/vm_slab.h>

struct slab_cache       *slabCache;
struct slab 			slab_list[MINBUCKET + 16];
simple_lock_data_t		slab_list_lock;

void
slab_init()
{
	simple_lock_init(&slab_list_lock, "slab_list_lock");

	slabCache = (slab_cache_t) rmalloc(coremap, sizeof(slab_cache_t));

	CIRCLEQ_INIT(&slabCache->sc_head);

	slab_count = 0;
}

/* slab metadata */
void
slabmeta(slab, size)
	slab_t 	slab;
	u_long  size;
{
	register slab_metadata_t meta;
	u_long indx;
	u_long bsize;

	indx = BUCKETINDX(size);
	bsize = BUCKETSIZE(indx);

	/* slab metadata */
	meta = slab->s_meta;
	meta->sm_bsize = bsize;
	meta->sm_bindx = indx;
	meta->sm_bslots = BUCKET_SLOTS(meta->sm_bsize);
	meta->sm_aslots = ALLOCATED_SLOTS(size);
	meta->sm_fslots = SLOTSFREE(meta->sm_bslots, meta->sm_aslots);
	if (meta->sm_fslots < 0) {
		meta->sm_fslots = 0;
	}

	/* test if free bucket slots is between 5% to 95% */
	if((meta->sm_fslots >= (meta->sm_bslots * 0.05)) && (meta->sm_fslots <= (meta->sm_bslots * 0.95))) {
		slab->s_flags |= SLAB_PARTIAL;
	/* test if free bucket slots is greater than 95% */
	} else if(meta->sm_fslots > (meta->sm_bslots * 0.95)) {
		slab->s_flags |= SLAB_FULL;
	} else {
		slab->s_flags |= SLAB_EMPTY;
	}
}

slab_t
slab_object(cache, size)
    slab_cache_t cache;
    long    size;
{
    register slab_t   slab;
    if(LARGE_OBJECT(size)) {
        slab = CIRCLEQ_LAST(&cache->sc_head);
    } else {
        slab = CIRCLEQ_FIRST(&cache->sc_head);
    }
    return (slab);
}

slab_t
slab_lookup(cache, size, mtype)
	slab_cache_t 	cache;
	long    		size;
	int 			mtype;
{
    register slab_t   slab;

    slab = &slab_list[BUCKETINDX(size)];
    slab_lock(&slab_list_lock);
	for(slab = slab_object(cache, size); slab != NULL; slab = CIRCLEQ_NEXT(slab, s_list)) {
		if(slab->s_size == size && slab->s_mtype == mtype) {
			slab_unlock(&slab_list_lock);
			return (slab);
		}
	}
	slab_unlock(&slab_list_lock);
    return (NULL);
}

void
slab_insert(cache, size, mtype, flags)
	slab_cache_t cache;
    long  		size;
    int	    	mtype, flags;
{
    register slab_t slab;
    register u_long indx;

	indx = BUCKETINDX(size);
	slab->s_size = size;
	slab->s_mtype = mtype;
	slab->s_flags = flags;

    slab = &slab_list[BUCKETINDX(size)];

    slab_lock(&slab_list_lock);
	if (indx < 10) {
		slab->s_stype = SLAB_SMALL;
		  CIRCLEQ_INSERT_HEAD(cache->sc_head, slab, s_list);
	} else {
		slab->s_stype = SLAB_LARGE;
		CIRCLEQ_INSERT_TAIL(cache->sc_head, slab, s_list);
	}
	slab_unlock(&slab_list_lock);
    slab_count++;

    /* update metadata */
	slabmeta(slab, size);
}

void
slab_remove(cache, size)
	slab_cache_t cache;
	long  	size;
{
	slab_t slab;

	slab = &slab_list[BUCKETINDX(size)];
	slab_lock(&slab_list_lock);
	CIRCLEQ_REMOVE(cache->sc_head, slab, s_list);
	slab_unlock(&slab_list_lock);
	slab_count--;
}

struct kmembuckets *
slab_kmembucket(slab)
	slab_t slab;
{
    if(slab->s_bucket != NULL) {
        return (slab->s_bucket);
    }
    return (NULL);
}

/*
 * search array for an empty bucket or partially full bucket that can fit
 * block of memory to be allocated
 */
struct kmembuckets *
kmembucket_search(cache, meta, size, mtype, flags)
	slab_cache_t    cache;
	slab_metadata_t meta;
	long size;
	int mtype, flags;
{
	register slab_t slab, next;
	register struct kmembuckets *kbp;
	long indx, bsize;
	int bslots, aslots, fslots;

	slab = slab_lookup(cache, size, mtype);

	indx = BUCKETINDX(size);
	bsize = BUCKETSIZE(indx);
	bslots = BUCKET_SLOTS(bsize);
	aslots = ALLOCATED_SLOTS(slab->s_size);
	fslots = SLOTSFREE(bslots, aslots);

	switch (flags) {
	case SLAB_FULL:
		next = CIRCLEQ_NEXT(slab, s_list);
		CIRCLEQ_FOREACH(next, &cache->sc_head, s_list) {
			if(next != slab) {
				if(next->s_flags == SLAB_PARTIAL) {
					slab = next;
					if (bsize > meta->sm_bsize && bslots > meta->sm_bslots && fslots > meta->sm_fslots) {
						kbp = slab_kmembucket(slab);
						slabmeta(slab, slab->s_size);
						return (kbp);
					}
				}
				if(next->s_flags == SLAB_EMPTY) {
					slab = next;
					kbp = slab_kmembucket(slab);
					slabmeta(slab, slab->s_size);
					return (kbp);
				}
			}
			return (NULL);
		}
		break;

	case SLAB_PARTIAL:
		if (bsize > meta->sm_bsize && bslots > meta->sm_bslots && fslots > meta->sm_fslots) {
			kbp = slab_kmembucket(slab);
			slabmeta(slab, slab->s_size);
			return (kbp);
		}
		break;

	case SLAB_EMPTY:
		kbp = slab_kmembucket(slab);
		slabmeta(slab, slab->s_size);
		return (kbp);
	}
	return (NULL);
}


