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
#include <sys/extent.h>
#include <sys/malloc.h>
#include <devel/sys/malloctypes.h>
#include <devel/vm/include/vm_slab.h>

struct slablist 	slab_list;
simple_lock_data_t	slab_list_lock;

/*
 * Start of Routines for Slab Allocator using extents
 */
void
slab_startup(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	slab_t 			slab;
	int				error;
	u_long 			indx;
	size_t 			size;
	u_long			bsize;

    CIRCLEQ_INIT(slab_list);

    simple_lock_init(&slab_list_lock, "slab_list_lock");

    slab_count = 0;

    size = end - start;

	for(indx = 0; indx < MINBUCKET + 16; indx++) {
		bsize = BUCKETSIZE(indx);
		slab_insert(slab, bsize, M_VMSLAB, M_WAITOK);
	}
	slab->s_extent = extent_create("slab extent", start, end, slab->s_mtype, NULL, NULL, EX_WAITOK | EX_MALLOCOK);
	error = extent_alloc_region(slab->s_extent, start, bsize, EX_WAITOK | EX_MALLOCOK);
	if (error) {
		printf("slab_startup: slab allocator initialized successful");
	} else {
		panic("slab_startup: slab allocator couldn't be initialized");
		slab_destroy(slab);
	}
}

void
slab_malloc(size, alignment, boundary, mtype, flags)
	u_long 	size, alignment, boundary;
	int		mtype, flags;
{
	slab_t 	slab;
	int 	error;
	u_long 	*result;

	slab = slab_create(CIRCLEQ_FIRST(&slab_list));
	slab = slab_lookup(size, mtype);

	error = extent_alloc(slab->s_extent, size, alignment, boundary, flags, &result);
	if(error) {
		printf("slab_malloc: successful");
	} else {
		printf("slab_malloc: unsuccessful");
	}
}

void
slab_free(addr, mtype)
	void 	*addr;
	int		mtype;
{
	slab_t slab;
	size_t start;
	int   error;

	slab = slab_lookup(addr, mtype);
	if (slab) {
		start = slab->s_extent->ex_start;
		error = extent_free(slab->s_extent, start, addr, flags);
		slab_remove(addr);
	}
}

void
slab_destroy(slab)
	slab_t slab;
{
	if(slab->s_extent != NULL) {
		extent_destroy(slab->s_extent);
	}
}
/* End of Routines for Slab Allocator using extents */

/* Start of Routines for Slab Allocator using Kmembuckets */
struct slab *
slab_create(slab)
	struct slab *slab;
{
    if(slab == NULL) {
        memset(slab, 0, sizeof(struct slab *));
    }
    return (slab);
}

void
slab_malloc1(size, mtype, flags)
	unsigned long size;
	int mtype, flags;
{
	register struct slab *slab;
	register struct kmembuckets *kbp;
	long indx;

	indx = BUCKETINDX(size);
	slab = slab_create(CIRCLEQ_FIRST(&slab_list));
	slab_insert(slab, size, type, flags);

	slab->s_size = size;
	slab->s_mtype = mtype;
	slab->s_flags = flags;

	slabmeta(slab, size, mtype, flags);

	if(slab->s_flags == SLAB_FULL) {
		CIRCLEQ_FOREACH(slab, &slab_list, s_list) {
			if(slab->s_flags == SLAB_PARTIAL) {
				slab->s_bucket = bucket_search(slab, slab->s_meta, SLAB_PARTIAL);
			} else {
				slab->s_bucket = bucket_search(slab, slab->s_meta, SLAB_EMPTY);
			}
		}
	}
}

/*
 * search array for an empty bucket or partially full bucket that can fit
 * block of memory to be allocated
 */
struct kmembuckets *
bucket_search(slab, meta, sflags)
	slab_t slab;
	slab_metadata_t meta;
	int sflags;
{
	register struct kmembuckets *kbp;
	long indx, bsize;
	int bslots, aslots, fslots;

	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		bsize = BUCKETSIZE(indx);
		bslots = BUCKET_SLOTS(bsize);
		aslots = ALLOCATED_SLOTS(slab->s_size);
		fslots = SLOTSFREE(bslots, aslots);
		switch(sflags) {
		case SLAB_PARTIAL:
			if (bsize > meta->sm_bsize && bslots > meta->sm_bslots && fslots > meta->sm_fslots) {
				kbp = slab->s_bucket[index];
				slabmeta(slab, slab->s_size, slab->s_mtype, slab->s_flags);
				return (kbp);
			}
			break;
		case SLAB_EMPTY:
			kbp = slab->s_bucket[index];
			slabmeta(slab, slab->s_size, slab->s_mtype, slab->s_flags);
			return (kbp);
		}
	}
	return (NULL);
}
/* End of Routines for Slab Allocator using Kmembuckets */

/* slab metadata information */
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
    simple_lock(&slab_list_lock);
	for(slab = slab_object(slabs, size); slab != NULL; slab = CIRCLEQ_NEXT(slab, s_list)) {
		if(slab->s_size == size && slab->s_mtype == mtype) {
			simple_unlock(&slab_list_lock);
			return (slab);
		}
	}
    simple_unlock(&slab_list_lock);
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
    simple_lock(&slab_list_lock);
	if (indx < 10) {
		slab->s_stype = SLAB_SMALL;
		  CIRCLEQ_INSERT_HEAD(slabs, slab, s_list);
	} else {
		slab->s_stype = SLAB_LARGE;
		CIRCLEQ_INSERT_TAIL(slabs, slab, s_list);
	}
    simple_unlock(&slab_list_lock);
    slab_count++;
}
