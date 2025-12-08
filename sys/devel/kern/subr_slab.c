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
/*
 * Splitting of slab allocator from kern_malloc, with slab subroutines to
 * interface between kern_malloc and the vm memory management and vmem (future implementation).
 */
/*
 * TODO: Slab Magazines:
 * - pop/push routines
 * - slots counter on each magazine list
 * - rotating magazines when full
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
#ifdef OVERLAY
#include <ovl/include/ovl_overlay.h>
#include <ovl/include/ovl.h>
#endif


struct kmemcache *slabcache;
/* buckets */
struct kmembuckets bucket[MINBUCKET + 16];
struct kmemslabs slabbucket[MINBUCKET + 16];
struct kmemmeta metabucket[MINBUCKET + 16];
struct kmemmagazine magazinebucket[MINBUCKET + 16];

vm_map_t kmem_map;
#ifdef OVERLAY
ovl_map_t omem_map;
#endif

/* [internal use only] */
unsigned long kmem_vm_malloc(unsigned long, int);
void kmem_vm_free(void*, unsigned long);
#ifdef OVERLAY
unsigned long kmem_ovl_malloc(unsigned long, int);
void kmem_ovl_free(void*, unsigned long, int);
#endif

static int slab_check(struct kmemmeta *);
static int slots_check(int, int, u_long, u_long);
/* slabmeta */
static void kmemslab_meta_insert(struct kmemslabs *, u_long, u_long);
static struct kmemmeta *kmemslab_meta_lookup(struct kmemslabs *, u_long, u_long);
static void kmemslab_meta_remove(struct kmemslabs *, u_long, u_long);
/* slabcache */
static void slabcache_insert(struct kmemcache *, struct kmemslabs *, u_long, u_long, int, int);
static void slabcache_remove(struct kmemcache *, struct kmemslabs *, u_long, u_long, int, int);
static struct kmemslabs *slabcache_lookup(struct kmemcache *, u_long, u_long, int, int);
static struct kmemslabs *slabcache_lookup_normal(struct kmemcache *, u_long, u_long, int, int);
static struct kmemslabs *slabcache_lookup_reverse(struct kmemcache *, u_long, u_long, int, int);

static float bucket_percentage(float, float);

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

/* check and update slab allocation flags */
static int
slab_check(meta)
	struct kmemmeta *meta;
{
	int fslotchk, aslotchk, bslotchk;

	bslotchk = slots_check(meta->ksm_bslots, meta->ksm_tbslots, meta->ksm_min, meta->ksm_max);
	fslotchk = slots_check(meta->ksm_fslots, meta->ksm_tfslots, meta->ksm_min, meta->ksm_max);
	aslotchk = slots_check(meta->ksm_aslots, meta->ksm_taslots, meta->ksm_min, meta->ksm_max);
	return (bslotchk ? fslotchk : aslotchk);
}

/* slab metadata functions */
LIST_HEAD(, kmemmeta) metalist = LIST_HEAD_INITIALIZER(metalist);

/* calculate slab metadata */
static void
kmemslab_meta_insert(slab, size, index)
	struct kmemslabs *slab;
	u_long size, index;
{
	struct kmemmeta *meta;
	unsigned long bsize, bindex;

	if (slab == NULL) {
		return;
	}

	meta = &metabucket[index];

	/* bucket's index and size */
	bindex = BUCKETINDX(size);
	bsize = BUCKETSIZE(index);

	/* slab metadata */
	meta->ksm_slab = slab;
	meta->ksm_bsize = bsize;
	meta->ksm_bindx = bindex;
	meta->ksm_bslots = BUCKET_SLOTS(bsize, size);
	meta->ksm_aslots = ALLOCATED_SLOTS(bsize, size);
	meta->ksm_fslots = SLOTSFREE(bsize, size);
	if (meta->ksm_fslots < 0) {
		meta->ksm_fslots = 0;
	}

	/* running accumulative total */
	meta->ksm_tbslots += meta->ksm_bslots;
	meta->ksm_tfslots += meta->ksm_fslots;
	meta->ksm_taslots += meta->ksm_aslots;

	/* set the minimum and maximum bucket boundaries */
	meta->ksm_min = (u_long) bucket_percentage((u_long) meta->ksm_bslots,
			(u_long) 0); /* lower bucket boundary */
	meta->ksm_max = (u_long) bucket_percentage((u_long) meta->ksm_bslots,
			(u_long) 95); /* upper bucket boundary */

	/* set slab meta to meta */
	slab->ksl_meta = meta;
	/* check and update slab allocation flags */
	slab->ksl_flags = slab_check(meta);

	simple_lock(&malloc_slock);
	LIST_INSERT_HEAD(&metalist, meta, ksm_list);
	simple_unlock(&malloc_slock);
	meta->ksm_refcnt++;
}

static struct kmemmeta *
kmemslab_meta_lookup(slab, size, index)
	struct kmemslabs *slab;
	u_long size, index;
{
	struct kmemmeta *meta;
	simple_lock(&malloc_slock);
	LIST_FOREACH(meta, &metalist, ksm_list) {
		if ((meta == &metabucket[index]) && (meta->ksm_slab == slab)
				&& (meta->ksm_bindx == BUCKETINDX(size))
				&& (meta->ksm_bsize == BUCKETSIZE(index))) {
			simple_unlock(&malloc_slock);
			return (meta);
		}
	}
	simple_unlock(&malloc_slock);
	return (NULL);
}

static void
kmemslab_meta_remove(slab, size, index)
	struct kmemslabs *slab;
	u_long size, index;
{
	struct kmemmeta *meta;

	meta = kmemslab_meta_lookup(slab, size, index);
	if (meta != NULL) {
		meta->ksm_tbslots -= meta->ksm_bslots;
		meta->ksm_tfslots -= meta->ksm_fslots;
		meta->ksm_taslots -= meta->ksm_aslots;
		LIST_REMOVE(meta, ksm_list);
		meta->ksm_refcnt--;
	}
}

/* slab magazine functions */
struct kmemmagazine *
kmemslab_magazine_create(cache, size)
	struct kmemcache *cache;
	vm_size_t size;
{
	struct kmemmagazine *mag;
	unsigned long index, bsize;

	index = BUCKETINDX(size);
	bsize = BUCKETSIZE(index);
#ifdef OVERLAY
	mag = (struct kmemslabs_magazine *)omem_alloc(overlay_map, (vm_size_t)(size * sizeof(struct kmemmagazine)));
#else
	mag = (struct kmemslabs_magazine *)kmem_alloc(kernel_map, (vm_size_t)(size * sizeof(struct kmemmagazine)));
#endif
	CIRCLEQ_INIT(&cache->ksc_maglist);
	cache->ksc_magcount = 0;
	mag->ksm_maxslots = SLOTSFREE(bsize, size);
	mag->ksm_freeslots = mag->ksm_maxslots;
	return (mag);
}

void
kmemslab_magazine_destroy(cache, mag)
	struct kmemcache *cache;
	struct kmemmagazine *mag;
{
	if (mag != NULL) {
#ifdef OVERLAY
		omem_free(overlay_map, (vm_offset_t)mag, (vm_size_t)sizeof(struct kmemmagazine));
#else
		kmem_free(kernel_map, (vm_offset_t)mag, (vm_size_t)sizeof(struct kmemmagazine));
#endif
	}
}

void
kmemslab_magazine_insert(cache, mag, flags, stype)
	struct kmemcache *cache;
	struct kmemmagazine *mag;
	int flags, stype;
{
	simple_lock(&malloc_slock);
	if (stype == SLAB_SMALL) {
		CIRCLEQ_INSERT_HEAD(&cache->ksc_maglist, mag, ksm_list);
	} else {
		CIRCLEQ_INSERT_TAIL(&cache->ksc_maglist, mag, ksm_list);
	}
	mag->ksm_freeslots--;
	simple_unlock(&malloc_slock);
	cache->ksc_magcount++;
}

void
kmemslab_magazine_remove(cache, mag, flags)
	struct kmemcache *cache;
	struct kmemmagazine *mag;
	int flags;
{
	simple_lock(&malloc_slock);
	CIRCLEQ_REMOVE(&cache->ksc_maglist, mag, ksm_list);
	mag->ksm_freeslots++;
	simple_unlock(&malloc_slock);
	cache->ksc_magcount--;
}

/* slabcache functions */
struct kmemcache *
kmemslab_cache_create(size)
	vm_size_t size;
{
	struct kmemcache *cache;
#ifdef OVERLAY
	cache = (struct kmemcache *)omem_alloc(overlay_map, (vm_size_t)(size * sizeof(struct kmemcache)));
#else
	cache = (struct kmemcache *)kmem_alloc(kernel_map, (vm_size_t)(size * sizeof(struct kmemcache)));
#endif
	CIRCLEQ_INIT(&cache->ksc_slablist_empty);
	CIRCLEQ_INIT(&cache->ksc_slablist_partial);
	CIRCLEQ_INIT(&cache->ksc_slablist_full);
	cache->ksc_slabcount = 0;
	return (cache);
}

void
kmemslab_cache_destroy(cache)
	struct kmemcache *cache;
{
	if (cache != NULL) {
#ifdef OVERLAY
		omem_free(overlay_map, (vm_offset_t)cache, (vm_size_t)sizeof(struct kmemcache));
#else
		kmem_free(kernel_map, (vm_offset_t)cache, (vm_size_t)sizeof(struct kmemcache));
#endif
	}
}

/* slab cache functions: original */
void
kmemslab_cache_alloc(cache, size, index, mtype)
	struct kmemcache *cache;
	u_long size, index;
	int mtype;
{
	register struct kmemslabs *slab;

	slab = &slabbucket[index];
	slabcache_insert(cache, slab, size, index, mtype, slab->ksl_flags);
}

void
kmemslab_cache_free(cache, size, index, mtype)
	struct kmemcache *cache;
	u_long size, index;
	int mtype;
{
	register struct kmemslabs *slab;

	slab = &slabbucket[index];
	slabcache_remove(cache, slab, size, index, mtype, slab->ksl_flags);
}

/* slab functions: original */
void
kmemslab_alloc(cache, size, mtype)
	struct kmemcache *cache;
	vm_size_t size;
	int	mtype;
{
	register struct kmemslabs *slab;

	slabcache_alloc(cache, size, BUCKETINDX(size), mtype);
	slab = &cache->ksc_slab;
}

void
kmemslab_free(cache, size)
	struct kmemcache *cache;
	long  size;
{
	register struct kmemslabs *slab;

	kmemslab_cache_free(cache, size, BUCKETINDX(size));
	slab = &cache->ksc_slab;
}

/* slab functions: new */
struct kmemslabs *
kmemslab_create(cache, size, index, mtype)
	struct kmemcache *cache;
	u_long size, index;
	int mtype;
{
	register struct kmemslabs *slab;

	slab = &slabbucket[index];
	slab->ksl_size = size;
	slab->ksl_mtype = mtype;
	if (index < 10) {
		slab->ksl_stype = SLAB_SMALL;
	} else {
		slab->ksl_stype = SLAB_LARGE;
	}
	/* update metadata */
	kmemslab_meta_insert(slab, size, index);
	cache->ksc_slab = *slab;
	return (slab);
}

void
kmemslab_destroy(cache, size, index)
	struct kmemcache *cache;
	u_long size, index;
{
	register struct kmemslabs *slab, *destroy;

	slab = &cache->ksc_slab;
	destroy = NULL;
	if (slab != NULL) {
		destroy = kmemslab_lookup(cache, &slab->ksl_meta, size, index,
				slab->ksl_mtype);
	}
	if (destroy != NULL) {
		kmemslab_remove(cache, destroy, size, index, destroy->ksl_mtype);
	}
	slab = NULL;
	cache->ksc_slab = *slab;
}

void
kmemslab_insert(cache, slab, size, index, mtype)
	struct kmemcache *cache;
	struct kmemslabs *slab;
	u_long size, index;
	int mtype;
{
	struct kmemmeta *meta;
	int rval;

	meta = kmemslab_meta_lookup(slab, size, index);
	rval = slab_check(meta);
	switch (rval) {
	case SLAB_FULL:
		if ((meta->ksm_tfslots > meta->ksm_min)
				&& (meta->ksm_tfslots < meta->ksm_max)) {
			/* full -> partial */
			slabcache_insert(cache, slab, size, index, mtype, SLAB_PARTIAL);
			slabcache_remove(cache, slab, size, index, mtype, SLAB_FULL);
		} else {
			slabcache_insert(cache, slab, size, index, mtype, SLAB_FULL);
		}
		break;
	case SLAB_PARTIAL:
		if (meta->ksm_tfslots >= meta->ksm_max) {
			/* partial -> full */
			slabcache_insert(cache, slab, size, index, mtype, SLAB_FULL);
			slabcache_remove(cache, slab, size, index, mtype, SLAB_PARTIAL);
		} else if (meta->ksm_tfslots == meta->ksm_min) {
			/* partial -> empty */
			slabcache_insert(cache, slab, size, index, mtype, SLAB_EMPTY);
			slabcache_remove(cache, slab, size, index, mtype, SLAB_PARTIAL);
		} else {
			slabcache_insert(cache, slab, size, index, mtype, SLAB_PARTIAL);
		}
		break;
	case SLAB_EMPTY:
		if ((meta->ksm_tfslots > meta->ksm_min)
				&& (meta->ksm_tfslots < meta->ksm_max)) {
			/* empty -> partial */
			slabcache_insert(cache, slab, size, index, mtype, SLAB_PARTIAL);
			slabcache_remove(cache, slab, size, index, mtype, SLAB_EMPTY);
		} else {
			slabcache_insert(cache, slab, size, index, mtype, SLAB_EMPTY);
		}
		break;
	}
}

void
kmemslab_remove(cache, slab, size, index, mtype)
	struct kmemcache *cache;
	struct kmemslabs *slab;
	u_long size, index;
	int mtype;
{
	struct kmemmeta *meta;
	int rval;

	meta = kmemslab_meta_lookup(slab, size, index);
	rval = slab_check(meta);
	switch (rval) {
	case SLAB_FULL:
		if ((meta->ksm_tfslots > meta->ksm_min)
				&& (meta->ksm_tfslots < meta->ksm_max)) {
			/* full -> partial */
			slabcache_insert(cache, slab, size, index, mtype, SLAB_PARTIAL);
			slabcache_remove(cache, slab, size, index, mtype, SLAB_FULL);
		} else {
			slabcache_remove(cache, slab, size, index, mtype, SLAB_FULL);
		}
		break;
	case SLAB_PARTIAL:
		if (meta->ksm_tfslots >= meta->ksm_max) {
			/* partial -> full */
			slabcache_insert(cache, slab, size, index, mtype, SLAB_FULL);
			slabcache_remove(cache, slab, size, index, mtype, SLAB_PARTIAL);
		} else if (meta->ksm_tfslots == meta->ksm_min) {
			/* partial -> empty */
			slabcache_insert(cache, slab, size, index, mtype, SLAB_EMPTY);
			slabcache_remove(cache, slab, size, index, mtype, SLAB_PARTIAL);
		} else {
			slabcache_remove(cache, slab, size, index, mtype, SLAB_PARTIAL);
		}
		break;
	case SLAB_EMPTY:
		if ((meta->ksm_tfslots > meta->ksm_min)
				&& (meta->ksm_tfslots < meta->ksm_max)) {
			/* empty -> partial */
			slabcache_insert(cache, slab, size, index, mtype, SLAB_PARTIAL);
			slabcache_remove(cache, slab, size, index, mtype, SLAB_EMPTY);
		} else {
			slabcache_remove(cache, slab, size, index, mtype, SLAB_EMPTY);
		}
		break;
	}
}

struct kmemslabs *
kmemslab_lookup(cache, meta, size, index, mtype)
	struct kmemcache *cache;
	struct kmemmeta *meta;
	u_long size, index;
	int mtype;
{
	struct kmemslabs *slab;
	int rval;

	rval = slab_check(meta);
	switch (rval) {
	case SLAB_FULL:
		slab = slabcache_lookup(cache, size, index, mtype, SLAB_FULL);
		break;
	case SLAB_PARTIAL:
		slab = slabcache_lookup(cache, size, index, mtype, SLAB_PARTIAL);
		break;
	case SLAB_EMPTY:
		slab = slabcache_lookup(cache, size, index, mtype, SLAB_EMPTY);
		break;
	}
	return (slab);
}

struct kmemslabs *
kmemslab_get_by_size(cache, size, mtype)
	struct kmemcache *cache;
	u_long size;
	int	mtype;
{
	return (kmemslab_lookup(cache, size, BUCKETINDX(size), mtype));
}

struct kmemslabs *
kmemslab_get_by_index(cache, index, mtype)
	struct kmemcache *cache;
	u_long index;
	int mtype;
{
	return (kmemslab_lookup(cache, BUCKETSIZE(index), index, mtype));
}

/* kmembucket functions */
struct kmembuckets *
kmemslab_bucket(slab)
	struct kmemslabs *slab;
{
    if (slab->ksl_bucket != NULL) {
        return (slab->ksl_bucket);
    }
    return (NULL);
}

struct kmembuckets *
kmemslab_bucket_search(cache, meta, size, index, mtype)
{
	register struct kmemslabs *slab;
	register struct kmembuckets *kbp;

	slab = kmemslab_lookup(cache, size, index, mtype);
	switch (slab->ksl_flags) {
	case SLAB_FULL:
		kbp = kmemslab_bucket(slab);
		if (kbp != NULL) {
			kmemslab_meta(slab, size, index);
			return (kbp);
		}
		break;

	case SLAB_PARTIAL:
		kbp = kmemslab_bucket(slab);
		if (kbp != NULL) {
			kmemslab_meta(slab, size, index);
			return (kbp);
		}
		break;

	case SLAB_EMPTY:
		kbp = kmemslab_bucket(slab);
		if (kbp != NULL) {
			kmemslab_meta(slab, size, index);
			return (kbp);
		}
		break;
	}
	return (NULL);
}

/* internal slabcache functions */
static void
slabcache_insert(cache, slab, size, index, mtype, flags)
	struct kmemcache *cache;
	struct kmemslabs *slab;
	u_long size, index;
	int mtype, flags;
{
	slab->ksl_size = size;
	slab->ksl_mtype = mtype;
	slab->ksl_flags = flags;

	simple_lock(&malloc_slock);
	if (index < 10) {
		slab->ksl_stype = SLAB_SMALL;
		switch (flags) {
		case SLAB_FULL:
			CIRCLEQ_INSERT_HEAD(&cache->ksc_slablist_full, slab, ksl_full);
			break;
		case SLAB_PARTIAL:
			CIRCLEQ_INSERT_HEAD(&cache->ksc_slablist_partial, slab, ksl_partial);
			break;
		case SLAB_EMPTY:
			CIRCLEQ_INSERT_HEAD(&cache->ksc_slablist_empty, slab, ksl_empty);
			break;
		}
	} else {
		slab->ksl_stype = SLAB_LARGE;
		switch (flags) {
		case SLAB_FULL:
			CIRCLEQ_INSERT_TAIL(&cache->ksc_slablist_full, slab, ksl_full);
			break;
		case SLAB_PARTIAL:
			CIRCLEQ_INSERT_TAIL(&cache->ksc_slablist_partial, slab, ksl_partial);
			break;
		case SLAB_EMPTY:
			CIRCLEQ_INSERT_TAIL(&cache->ksc_slablist_empty, slab, ksl_empty);
			break;
		}
	}
	simple_unlock(&malloc_slock);
	cache->ksc_slab = *slab;
	cache->ksc_slabcount++;

	/* update metadata */
	kmemslab_meta_insert(slab, size, index);
}

static void
slabcache_remove(cache, slab, size, index, mtype, flags)
	struct kmemcache *cache;
	struct kmemslabs *slab;
	u_long size, index;
	int mtype, flags;
{
	simple_lock(&malloc_slock);
	if (((slab->ksl_size == size) || (slab->ksl_size == BUCKETSIZE(index))) && (slab->ksl_mtype == mtype)) {
		switch (flags) {
		case SLAB_FULL:
			CIRCLEQ_REMOVE(&cache->ksc_slablist_full, slab, ksl_full);
			break;
		case SLAB_PARTIAL:
			CIRCLEQ_REMOVE(&cache->ksc_slablist_partial, slab, ksl_partial);
			break;
		case SLAB_EMPTY:
			CIRCLEQ_REMOVE(&cache->ksc_slablist_empty, slab, ksl_empty);
			break;
		}
	}
	simple_unlock(&malloc_slock);
	cache->ksc_slab = *slab;
	cache->ksc_slabcount--;

	/* update metadata */
	kmemslab_meta_remove(slab, size, index);
}

static struct kmemslabs *
slabcache_lookup(cache, size, index, mtype, flags)
	struct kmemcache *cache;
	u_long size, index;
	int mtype, flags;
{
	register struct kmemslabs *slab;

	if (LARGE_OBJECT(size)) {
		slab = slabcache_lookup_reverse(cache, size, index, mtype, flags);
	} else {
		slab = slabcache_lookup_normal(cache, size, index, mtype, flags);
	}
	return (slab);
}

static struct kmemslabs *
slabcache_lookup_normal(cache, size, index, mtype, flags)
	struct kmemcache *cache;
	u_long size, index;
	int mtype, flags;
{
	register struct kmemslabs *slab;

	simple_lock(&malloc_slock);
	switch (flags) {
	case SLAB_FULL:
		CIRCLEQ_FOREACH(slab, &cache->ksc_slablist_full, ksl_full) {
			if (slab == &slabbucket[index]) {
				if (((slab->ksl_size == size)) && (slab->ksl_mtype == mtype)) {
					simple_unlock(&malloc_slock);
					return (slab);
				}
			}
		}
		break;
	case SLAB_PARTIAL:
		CIRCLEQ_FOREACH(slab, &cache->ksc_slablist_partial, ksl_partial) {
			if (slab == &slabbucket[index]) {
				if (((slab->ksl_size == size)) && (slab->ksl_mtype == mtype)) {
					simple_unlock(&malloc_slock);
					return (slab);
				}
			}
		}
		break;
	case SLAB_EMPTY:
		CIRCLEQ_FOREACH(slab, &cache->ksc_slablist_empty, ksl_empty) {
			if (slab == &slabbucket[index]) {
				if (((slab->ksl_size == size)) && (slab->ksl_mtype == mtype)) {
					simple_unlock(&malloc_slock);
					return (slab);
				}
			}
		}
		break;
	}
	simple_unlock(&malloc_slock);
	return (NULL);
}

static struct kmemslabs *
slabcache_lookup_reverse(cache, size, index, mtype, flags)
	struct kmemcache *cache;
	u_long size, index;
	int mtype, flags;
{
	register struct kmemslabs *slab;

	simple_lock(&malloc_slock);
	switch (flags) {
	case SLAB_FULL:
		CIRCLEQ_FOREACH_REVERSE(slab, &cache->ksc_slablist_full, ksl_full) {
			if (slab == &slabbucket[index]) {
				if (((slab->ksl_size == size)) && (slab->ksl_mtype == mtype)) {
					simple_unlock(&malloc_slock);
					return (slab);
				}
			}
		}
		break;
	case SLAB_PARTIAL:
		CIRCLEQ_FOREACH_REVERSE(slab, &cache->ksc_slablist_partial, ksl_partial) {
			if (slab == &slabbucket[index]) {
				if (((slab->ksl_size == size)) && (slab->ksl_mtype == mtype)) {
					simple_unlock(&malloc_slock);
					return (slab);
				}
			}
		}
		break;
	case SLAB_EMPTY:
		CIRCLEQ_FOREACH_REVERSE(slab, &cache->ksc_slablist_empty, ksl_empty) {
			if (slab == &slabbucket[index]) {
				if (((slab->ksl_size == size)) && (slab->ksl_mtype == mtype)) {
					simple_unlock(&malloc_slock);
					return (slab);
				}
			}
		}
		break;
	}
	simple_unlock(&malloc_slock);
	return (NULL);
}

/* slab's kmembucket percentage */
static float
bucket_percentage(float x, float y)
{
    float p = ((x / 100) * y);
    return (p);
}

/* check slots and return matching slab allocation flags */
static int
slots_check(slots, tslots, min, max)
	int slots, tslots;
	u_long min, max;
{
	int rval;

	rval = 0;
	if (tslots > min && tslots < max) {
		if (slots > min && slots < tslots) {
			rval = SLAB_PARTIAL;
		} else if (slots >= tslots) {
			rval = SLAB_FULL;
		} else {
			rval = SLAB_EMPTY;
		}
	} else if (tslots >= max) {
		rval = SLAB_FULL;
	} else {
		rval = SLAB_EMPTY;
	}
	return (rval);
}

/* vm kmem */
/* allocate memory to vm [internal use only] */
unsigned long
kmem_vm_malloc(size, flags)
	unsigned long size;
	int flags;
{
	return (kmem_malloc(kmem_map, (vm_size_t)size, !(flags & (M_NOWAIT | M_CANFAIL))));
}

/* free memory from vm [internal use only] */
void
kmem_vm_free(addr, size)
	void *addr;
	unsigned long size;
{
	kmem_free(kmem_map, (vm_offset_t)addr, size);
}

#ifdef OVERLAY

/* ovl omem */
/* allocate memory to ovl [internal use only] */
unsigned long
kmem_ovl_malloc(size, flags)
	unsigned long size;
	int flags;
{
	return (omem_malloc(omem_map, (vm_size_t)size, (M_OVERLAY & !(flags & (M_NOWAIT | M_CANFAIL)))));
}

/* free memory from ovl [internal use only] */
void
kmem_ovl_free(addr, size, type)
	void *addr;
	unsigned long size;
	int type;
{
	if (type & M_OVERLAY) {
		omem_free(omem_map, (vm_offset_t)addr, size);
	}
}

#endif
