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
 * - Implement suitable hash algorithms
 */

#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/fnv_hash.h>
#include <devel/sys/malloctypes.h>
#include <devel/vm/include/vm_slab.h>

#define SLAB_BUCKET_HASH_COUNT (MINBUCKET + 16)

struct slablist 					slab_list[MINBUCKET + 16];
struct slab_cache_list 				slab_cache_list;

simple_lock_data_t					slab_list_lock;
simple_lock_data_t					slab_cache_list_lock;

/* slab metadata */
void
slabmeta(slab, size, mtype, flags)
	slab_t 	slab;
	u_long  size;
	int		mtype, flags;
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

/* slab cache routines */

long
slab_cache_hash(slab)
	slab_t slab;
{
	Fnv32_t hash1 = fnv_32_buf(&slab, sizeof(&slab), FNV1_32_INIT) % SLAB_BUCKET_HASH_COUNT;
	Fnv32_t hash2 = (((unsigned long)slab)%SLAB_BUCKET_HASH_COUNT);
	return (hash1^hash2);
}

void
slab_cache_insert(slab)
	slab_t slab;
{
	struct slab_cache_list *cache;
	slab_cache_t			entry;

	if(slab == NULL) {
		return;
	}

	cache = &slab_cache_list[cache_hash(slab)];
	entry = malloc(sizeof(slab_cache_t));			/* TODO allocate */
	entry->sc_slab = slab;

	slab_cache_lock(&slab_cache_list_lock);
	TAILQ_INSERT_HEAD(cache, entry, sc_cache_links);
	slab_cache_unlock(&slab_cache_list_lock);
	slab_cache_count++;
}

void
slab_cache_remove(slab)
	slab_t slab;
{
	struct slab_cache_list *cache;
	slab_cache_t			entry;

	cache = &slab_cache_list[cache_hash(slab)];
	slab_cache_lock(&slab_cache_list_lock);
	for(entry = TAILQ_FIRST(cache); entry != NULL; entry = TAILQ_NEXT(entry, sc_cache_links)) {
		slab = entry->sc_slab;
		if(slab) {
			TAILQ_REMOVE(cache, entry, sc_cache_links);
			slab_cache_count--;
			slab_cache_unlock(&slab_cache_list_lock);
		}
	}
}

slab_cache_t
slab_cache_lookup(slab)
	slab_t slab;
{
	struct slab_cache_list *cache;
	slab_cache_t			entry;

	cache = &slab_cache_list[cache_hash(slab)];
	slab_cache_lock(&slab_cache_list_lock);
	for(entry = TAILQ_FIRST(cache); entry != NULL; entry = TAILQ_NEXT(entry, sc_cache_links)) {
		slab = entry->sc_slab;
		if(slab) {
			TAILQ_REMOVE(cache, entry, sc_cache_links);
			slab_cache_count--;
			slab_cache_unlock(&slab_cache_list_lock);
			return (cache);
		}
	}
	slab_cache_unlock(&slab_cache_list_lock);
	return (NULL);
}

/* slab routines */

long
slab_bucket_hash(bucket)
	struct kmembuckets *bucket;
{
	Fnv32_t hash1 = fnv_32_buf(&bucket, sizeof(&bucket), FNV1_32_INIT) % SLAB_BUCKET_HASH_COUNT;
	Fnv32_t hash2 = (((unsigned long)bucket)%SLAB_BUCKET_HASH_COUNT);
	return (hash1^hash2);
}

slab_t
slab_object(slabs, size)
	struct slablist   *slabs;
	long    size;
{
	register slab_t   slab;
	if(LARGE_OBJECT(size)) {
		slab = CIRCLEQ_LAST(slabs);
	} else {
		slab = CIRCLEQ_FIRST(slabs);
	}
	return (slab);
}

slab_t
slab_lookup(size, mtype)
	long    size;
	int 	mtype;
{
    struct slablist   *slabs;
    register slab_t   slab;

    slabs = &slab_list[BUCKETINDX(size)];
    slab_lock(&slab_list_lock);
	for(slab = slab_object(slabs, size); slab != NULL; slab = CIRCLEQ_NEXT(slab, s_list)) {
		if(slab->s_size == size && slab->s_mtype == mtype) {
			slab_unlock(&slab_list_lock);
			return (slab);
		}
	}
	slab_unlock(&slab_list_lock);
    return (NULL);
}

void
slab_insert(slab, size, mtype, flags)
    slab_t  slab;
    u_long  size;
    int	    mtype, flags;
{
    register struct slablist  *slabs;
    register u_long indx;

    if(slab == NULL) {
        return;
    }

	indx = BUCKETINDX(size);
	slab->s_size = size;
	slab->s_mtype = mtype;
	slab->s_flags = flags;

    slabs = &slab_list[BUCKETINDX(size)];
    slab_lock(&slab_list_lock);
	if (indx < 10) {
		slab->s_stype = SLAB_SMALL;
		  CIRCLEQ_INSERT_HEAD(slabs, slab, s_list);
	} else {
		slab->s_stype = SLAB_LARGE;
		CIRCLEQ_INSERT_TAIL(slabs, slab, s_list);
	}
	slab_unlock(&slab_list_lock);
    slab_count++;
}

void
slab_remove(slab)
	slab_t  slab;
{
	struct slablist *slabs;

	slabs = &slab_list[BUCKETINDX(slab->s_size)];
	slab_lock(&slab_list_lock);
	CIRCLEQ_REMOVE(slabs, slab, s_list);
	slab_unlock(&slab_list_lock);
	slab_count--;
}
