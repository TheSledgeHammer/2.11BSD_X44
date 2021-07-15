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
#include <sys/malloc.h>
#include <sys/fnv_hash.h>
#include <devel/sys/malloctypes.h>
#include <devel/vm/include/vm_slab.h>


/*
 * Modify malloc:
 * - use slab_list with pointer to kmembucket.
 * - provide a sanity-check that the bucket matches the slabs designated bucket.
 * - if non-matching search for correct or closest matching
 *
 * Question Slab's placement?
 * - Modifying kern_malloc to utilize slabs, means that the slab allocator is not required in the vm
 * and can be placed in the kernel. As no part of the slab allocator is running before kernel_malloc
 * is initialized.
 */
struct kbucketlist 	slab_kbuckets[MINBUCKET + 16];

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

    simple_lock_init(&slab_list_lock, "slab_list_lock");

    CIRCLEQ_INIT(slab_list);

    slab_count = 0;

    size = end - start;

	for(indx = 0; indx < MINBUCKET + 16; indx++) {
		bsize = BUCKETSIZE(indx);
		slab_insert(slab, bsize, M_VMSLAB, M_WAITOK);
	}
}

#define SLAB_BUCKET_HASH_COUNT (MINBUCKET + 16)

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

/*
 * search array for an empty bucket or partially full bucket that can fit
 * block of memory to be allocated
 */
struct kmembuckets *
bucket_search(slab, meta, size, flags)
	slab_t slab;
	slab_metadata_t meta;
	unsigned long size;
	int flags;
{
	register slab_t next;
	register struct kmembuckets *kbp;
	long indx, bsize;
	int bslots, aslots, fslots;

	indx = BUCKETINDX(size);
	bsize = BUCKETSIZE(indx);
	bslots = BUCKET_SLOTS(bsize);
	aslots = ALLOCATED_SLOTS(slab->s_size);
	fslots = SLOTSFREE(bslots, aslots);

	slab = CIRCLEQ_FIRST(&slab_list[indx]);
	slab_insert(slab, size, type, flags);

	switch (flags) {
	case SLAB_FULL:
		next = CIRCLEQ_NEXT(slab, s_list);
		CIRCLEQ_FOREACH(next, &slab_list[indx], s_list) {
			if(next != slab) {
				if(next->s_flags == SLAB_PARTIAL) {
					slab = next;
					if (bsize > meta->sm_bsize && bslots > meta->sm_bslots && fslots > meta->sm_fslots) {
						kbp = slab->s_bucket[index];
						slabmeta(slab, slab->s_size);
					}
				}
				if(next->s_flags == SLAB_EMPTY) {
					slab = next;
					kbp = slab->s_bucket[index];
					slabmeta(slab, slab->s_size);
				}
				return (kbp);
			} else {
				return (NULL);
			}
		}
		break;

	case SLAB_PARTIAL:
		if (bsize > meta->sm_bsize && bslots > meta->sm_bslots && fslots > meta->sm_fslots) {
			kbp = slab->s_bucket[index];
			slabmeta(slab, slab->s_size);
			return (kbp);
		}
		break;

	case SLAB_EMPTY:
		kbp = slab->s_bucket[index];
		slabmeta(slab, slab->s_size);
		return (kbp);
	}
	return (NULL);
}

