/*
 * Copyright (c) 1987, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_malloc.c	8.4 (Berkeley) 5/20/95
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <vm/include/vm_kern.h>
#include <vm/include/vm.h>

//#ifdef NEWSLAB

struct kmemcache *slabcache;
/* buckets */
struct kmembuckets bucket[MINBUCKET + 16];
struct kmemslabs slabbucket[MINBUCKET + 16];
struct kmemmeta metabucket[MINBUCKET + 16];
struct kmemmagazine magazinebucket[MINBUCKET + 16];

void
kmembucket_init(void)
{
	register long indx;

	/* Initialize buckets */
	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		/* slabs */
		slabbucket[indx].ksl_bucket = &bucket[indx];
		slabbucket[indx].ksl_meta = &metabucket[indx];
		/* meta */
		metabucket[indx].ksm_slab = &slabbucket[indx];
	}
}

struct kmembuckets *
kmembucket_alloc(cache, size, index, mtype)
	struct kmemcache *cache;
	unsigned long size, index;
	int mtype;
{
    register struct kmemslabs *slab;
    register struct kmembuckets *kbp;

    slab = kmemslab_create(cache, size, index, mtype);
    if (slab != NULL) {
        kbp = slab->ksl_bucket;
    }
    if (kbp != NULL) {
        kmemslab_insert(cache, slab, size, index, mtype);
        return (kbp);
    }
    return (NULL);
}

struct kmembuckets *
kmembucket_free(cache, size, index, mtype)
	struct kmemcache *cache;
	unsigned long size, index;
	int mtype;
{
	register struct kmemslabs *slab;
	register struct kmembuckets *kbp;

	slab = kmemslab_lookup(cache, size, index, mtype);
	if (slab != NULL) {
		kbp = slab->ksl_bucket;
	}
	if (kbp != NULL) {
		kmemslab_remove(cache, slab, size, index, mtype);
		return (kbp);
	}
	return (NULL);
}

void
kmembucket_destroy(cache, kbp, size, index, mtype)
	struct kmemcache *cache;
	struct kmembuckets *kbp;
	unsigned long size, index;
	int mtype;
{
    register struct kmemslabs *slab;
    register struct kmembuckets *skbp;

    slab = kmemslab_lookup(cache, size, index, mtype);
    if (slab != NULL) {
        skbp = slab->ksl_bucket;
    }
	if (skbp != NULL) {
		if (kbp != NULL) {
			if (skbp == kbp) {
				return;
			}
		} else {
			goto destroy;
		}
	}

destroy:
	skbp = kbp;
	slab->ksl_bucket = skbp;
	kmemslab_destroy(cache, slab, size, index, mtype);
}

/* allocate memory to vm/ovl */
vm_offset_t
vmembucket_malloc(size, flags)
	unsigned long size;
	int flags;
{
	vm_offset_t va;
#ifdef OVERLAY
	va = omem_malloc(omem_map, (vm_size_t) size,
			(M_OVERLAY & !(flags & (M_NOWAIT | M_CANFAIL))));
#else
	va = kmem_malloc(kmem_map, (vm_size_t) size,
			!(flags & (M_NOWAIT | M_CANFAIL)));
#endif
	return (va);
}

/* free memory from vm/ovl */
void
vmembucket_free(addr, size, type)
	void *addr;
	unsigned long size;
	int type;
{
#ifdef OVERLAY
	if (type & M_OVERLAY) {
		omem_free(omem_map, (vm_offset_t)addr, size);
	}
#else
	kmem_free(kmem_map, (vm_offset_t)addr, size);
#endif
}

/*
malloc(size, type, flags)
{
	register struct kmembuckets *kbp;
	caddr_t  va;

	kbp = kmembucket_alloc(&slabcache, size, BUCKETINDX(size), type);

	if (size > MAXALLOCSAVE) {
		allocsize = roundup(size, CLBYTES);
	} else {
		allocsize = 1 << indx;
	}
	npg = clrnd(btoc(allocsize));
	va = (caddr_t)vmembucket_malloc((vm_size_t)ctob(npg), flags);
}

free(addr, type)
{
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
	long size;

	kup = btokup(addr);
	size = 1 << kup->ku_indx;
	kbp = kmembucket_free(&slabcache, size, kup->ku_indx, type);

	if (size > MAXALLOCSAVE) {
		vmembucket_free(addr, ctob(kup->ku_pagecnt), type);
	}
}

void
kmeminit(void)
{
	int npg;

	npg = VM_KMEM_SIZE / NBPG;

	kmemslab_init(&slabcache, npg);
	kmembucket_init();
}
*/
#endif

#ifdef OLDSLAB
/* slab metadata */
void
slabmeta(slab, size)
	struct kmemslabs *slab;
	u_long size;
{
	register struct kmemmeta *meta;
	u_long indx;
	u_long bsize;

	/* bucket's index and size */
	indx = BUCKETINDX(size);
	bsize = BUCKETSIZE(indx);

	/* slab metadata */
	meta = &slab->ksl_meta;
	meta->ksm_bsize = bsize;
	meta->ksm_bindx = indx;
	meta->ksm_bslots = BUCKET_SLOTS(bsize, size);
	meta->ksm_aslots = ALLOCATED_SLOTS(bsize, size);
	meta->ksm_fslots = SLOTSFREE(bsize, size);
	if (meta->ksm_fslots < 0) {
		meta->ksm_fslots = 0;
	}

	meta->ksm_min = percent(meta->ksm_bslots, 0); 		/* lower bucket boundary */
	meta->ksm_max = percent(meta->ksm_bslots, 95);  	/* upper bucket boundary */

	/* test if free bucket slots is greater than 0 and less than 95% */
	if ((meta->ksm_fslots > meta->ksm_min) && (meta->ksm_fslots < meta->ksm_max)) {
		slab->ksl_flags |= SLAB_PARTIAL;
	/* test if free bucket slots is greater than or equal to 95% */
	} else if (meta->ksm_fslots >= meta->ksm_max) {
		slab->ksl_flags |= SLAB_FULL;
	} else {
		slab->ksl_flags |= SLAB_EMPTY;
	}
}

/* slabcache functions */
struct kmemcache *
slabcache_create(map, size)
	vm_map_t  map;
	vm_size_t size;
{
	struct kmemcache *cache;

	cache = (struct kmemcache *)kmem_alloc(map, (vm_size_t)(size * sizeof(struct kmemcache)));
	CIRCLEQ_INIT(&cache->ksc_slablist);
	cache->ksc_slabcount = 0;
	return (cache);
}

void
slabcache_alloc(cache, size, index, mtype)
	struct kmemcache *cache;
	long size;
	u_long index;
	int mtype;
{
	register struct kmemslabs *slab;

	slab = &slabbucket[index];
	slab->ksl_size = size;
	slab->ksl_mtype = mtype;

    simple_lock(&malloc_slock);
	if (index < 10) {
		slab->ksl_stype = SLAB_SMALL;
		CIRCLEQ_INSERT_HEAD(&cache->ksc_slablist, slab, ksl_list);
	} else {
		slab->ksl_stype = SLAB_LARGE;
		CIRCLEQ_INSERT_TAIL(&cache->ksc_slablist, slab, ksl_list);
	}
	simple_unlock(&malloc_slock);
	cache->ksc_slab = *slab;
	cache->ksc_slabcount++;

    /* update metadata */
	slabmeta(slab, size);
}

void
slabcache_free(cache, size, index)
	struct kmemcache *cache;
	long size;
	u_long index;
{
	register struct kmemslabs *slab;

	slab = &slabbucket[index];

	simple_lock(&malloc_slock);
	CIRCLEQ_REMOVE(&cache->ksc_slablist, slab, ksl_list);
	simple_unlock(&malloc_slock);
	cache->ksc_slab = *slab;
	cache->ksc_slabcount--;

    /* update metadata */
	slabmeta(slab, size);
}

struct kmemslabs *
slabcache_lookup(cache, size, index, mtype)
	struct kmemcache *cache;
	long size;
	u_long index;
	int mtype;
{
	register struct kmemslabs *slab;

    simple_lock(&malloc_slock);
    if (LARGE_OBJECT(size)) {
    	CIRCLEQ_FOREACH_REVERSE(slab, &cache->ksc_slablist, ksl_list) {
    		if (slab == &slabbucket[index]) {
    			if ((slab->ksl_size == size) && (slab->ksl_mtype == mtype)) {
    				simple_unlock(&malloc_slock);
    	    		return (slab);
    			}
    		}
    	}
    } else {
    	CIRCLEQ_FOREACH(slab, &cache->ksc_slablist, ksl_list) {
    		if (slab == &slabbucket[index]) {
    			if ((slab->ksl_size == size) && (slab->ksl_mtype == mtype)) {
    				simple_unlock(&malloc_slock);
    	    		return (slab);
    			}
    		}
    	}
    }
	simple_unlock(&malloc_slock);
    return (NULL);
}

/* slab functions */
struct kmemslabs *
slab_get_by_size(cache, size, mtype)
	struct kmemcache *cache;
	long size;
	int mtype;
{
    return (slabcache_lookup(cache, size, BUCKETINDX(size), mtype));
}

struct kmemslabs *
slab_get_by_index(cache, index, mtype)
	struct kmemcache *cache;
	u_long index;
	int mtype;
{
    return (slabcache_lookup(cache, BUCKETSIZE(index), index, mtype));
}

struct kmemslabs *
slab_create(cache, size, mtype)
	struct kmemcache *cache;
    long size;
    int mtype;
{
    register struct kmemslabs *slab;

	slabcache_alloc(cache, size, BUCKETINDX(size), mtype);
	slab = &cache->ksc_slab;
	return (slab);
}

void
slab_destroy(cache, size)
	struct kmemcache *cache;
	long size;
{
	register struct kmemslabs *slab;

	slabcache_free(cache, size, BUCKETINDX(size));
	slab = &cache->ksc_slab;
}

struct kmembuckets *
slab_kmembucket(slab)
	struct kmemslabs *slab;
{
    if (slab->ksl_bucket != NULL) {
        return (slab->ksl_bucket);
    }
    return (NULL);
}

void
slabinit(cache, map, size)
	struct kmemcache **cache;
	vm_map_t map;
	vm_size_t size;
{
	struct kmemcache *ksc;
	long indx;

	/* Initialize slabs kmembuckets */
	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		slabbucket[indx].ksl_bucket = &bucket[indx];
	}
	/* Initialize slab cache */
	ksc = slabcache_create(map, size);
	*cache = ksc;
}

/* kmembucket functions */

/*
 * search array for an empty bucket or partially full bucket that can fit
 * block of memory to be allocated
 */
struct kmembuckets *
kmembucket_search(cache, meta, size, mtype)
	struct kmemcache *cache;
	struct kmemmeta *meta;
	long size;
	int mtype;
{
	register struct kmemslabs *slab, *next;
	register struct kmembuckets *kbp;
	long indx, bsize;
	int bslots, aslots, fslots;

	slab = slab_get_by_size(cache, size, mtype);

	indx = BUCKETINDX(size);
	bsize = BUCKETSIZE(indx);
	bslots = BUCKET_SLOTS(bsize, size);
	aslots = ALLOCATED_SLOTS(bsize, slab->ksl_size);
	fslots = SLOTSFREE(bsize, slab->ksl_size);

	switch (slab->ksl_flags) {
	case SLAB_FULL:
		next = CIRCLEQ_NEXT(slab, ksl_list);
		CIRCLEQ_FOREACH(next, &cache->ksc_slablist, ksl_list) {
			if ((next != slab) && (next->ksl_flags != SLAB_FULL)) {
				switch (next->ksl_flags) {
				case SLAB_PARTIAL:
					slab = next;
					goto partial;

				case SLAB_EMPTY:
					slab = next;
					goto empty;
				}
			}
			return (NULL);
		}
		break;

	case SLAB_PARTIAL:
partial:
		if ((bsize > meta->ksm_bsize) && (bslots > meta->ksm_bslots) && (fslots > meta->ksm_fslots)) {
			kbp = slab_kmembucket(slab);
			slabmeta(slab, slab->ksl_size);
			return (kbp);
		}
		break;

	case SLAB_EMPTY:
empty:
		kbp = slab_kmembucket(slab);
		slabmeta(slab, slab->ksl_size);
		return (kbp);
	}
	return (NULL);
}


/* allocate memory to vm [internal use only] */
caddr_t
kmalloc(size, flags)
	unsigned long size;
	int flags;
{
	long indx, npg, allocsize;
	caddr_t  va;

	if (size > MAXALLOCSAVE) {
		allocsize = roundup(size, CLBYTES);
	} else {
		allocsize = 1 << indx;
	}
	npg = clrnd(btoc(allocsize));
	va = (caddr_t)kmem_malloc(kmem_map, (vm_size_t)ctob(npg), !(flags & (M_NOWAIT | M_CANFAIL)));

	return (va);
}

/* free memory from vm [internal use only] */
void
kfree(addr, size)
	void *addr;
	short size;
{
	kmem_free(kmem_map, (vm_offset_t)addr, size);
}

#ifdef OVERLAY
/* allocate memory to ovl [internal use only] */
caddr_t
omalloc(size, flags)
	unsigned long size;
	int flags;
{
	long indx, npg, allocsize;
	caddr_t  va;

	if (size > MAXALLOCSAVE) {
		allocsize = roundup(size, CLBYTES);
	} else {
		allocsize = 1 << indx;
	}
	npg = clrnd(btoc(allocsize));
	va = (caddr_t)omem_malloc(omem_map, (vm_size_t)ctob(npg), (M_OVERLAY & !(flags & (M_NOWAIT | M_CANFAIL))));
	return (va);
}

/* free memory from ovl [internal use only] */
void
ofree(addr, size, type)
	void *addr;
	short size;
	int type;
{
	if (type & M_OVERLAY) {
		omem_free(omem_map, (vm_offset_t) addr, size);
	}
}
#endif
#endif

