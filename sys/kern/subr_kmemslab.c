/*
 * The 3-Clause BSD License:
 * Copyright (c) 2025 Martin Kelly
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
 *
 * @(#)subr_kmemslab.c	1.0 (Martin Kelly) 12/8/25
 */
/*
 * Splitting of slab allocator from kern_malloc, with slab subroutines to
 * interface between kern_malloc and the vm memory management and vmem (future implementation).
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

#include <machine/param.h>

struct kmemmagazine_depot magazinedepot[MAGAZINE_DEPOT_MAX];
struct kmemcpu_cache cpucache[NCPUS];
LIST_HEAD(, kmemmeta) metalist = LIST_HEAD_INITIALIZER(metalist);

/* slabmeta */
static void slabmeta_insert(struct kmemslabs *, u_long, u_long);
static struct kmemmeta *slabmeta_lookup(u_long, u_long);
static void slabmeta_remove(struct kmemslabs *, u_long, u_long);

/* slab */
static struct kmemslabs *slab_create(struct kmemcache *, u_long, u_long, int);
static void slab_destroy(struct kmemcache *, struct kmemslabs *, u_long, u_long, int);
static void slab_insert(struct kmemcache *, struct kmemslabs *, u_long, u_long, int);
static void slab_remove(struct kmemcache *, struct kmemslabs *, u_long, u_long, int);
static struct kmemslabs *slab_lookup(struct kmemcache *, u_long, u_long, int);
static int slab_state(int, int, u_long, u_long);
static int slab_check(struct kmemmeta *);

/* slabcache */
static struct kmemcache *slabcache_create(vm_size_t);
static void slabcache_destroy(struct kmemcache *);
static void slabcache_insert(struct kmemcache *, struct kmemslabs *, u_long, u_long, int, int);
static void slabcache_remove(struct kmemcache *, struct kmemslabs *, u_long, u_long, int, int);
static struct kmemslabs *slabcache_lookup(struct kmemcache *, u_long, u_long, int, int);
static struct kmemslabs *slabcache_lookup_normal(struct kmemcache *, u_long, u_long, int, int);
static struct kmemslabs *slabcache_lookup_reverse(struct kmemcache *, u_long, u_long, int, int);

/* depot */
static struct kmemmagazine_depot *depot_create(struct kmemcache *, int, int);
static void depot_insert(struct kmemmagazine_depot *, struct kmemmagazine *, void *, u_long, u_long, int *, int);
static struct kmemmagazine *depot_lookup(struct kmemmagazine_depot *, void *, u_long, u_long, int);
static void depot_remove(struct kmemmagazine_depot *, struct kmemmagazine *, void *, u_long, u_long, int *, int);

/* magazine */
static struct kmemmagazine *magazine_create(struct kmemmagazine_depot *, void *, u_long, u_long, int);
static void magazine_insert(struct kmemmagazine_depot *, struct kmemmagazine *, void *, u_long, u_long, int *);
static void magazine_remove(struct kmemmagazine_depot *, struct kmemmagazine *, void *, u_long, u_long, int *);
static struct kmemmagazine *magazine_lookup(struct kmemmagazine_depot *, void *, u_long, u_long, int *);
static int magazine_state(int);
static struct kmemmeta *magazine_check(struct kmemslabs *, struct kmemmagazine *);

/* cpucache */
static struct kmemcpu_cache *cpucache_create(struct kmemmagazine *, int, int);
static void cpucache_magazine_alloc(struct kmemcache *, struct kmemmagazine_depot **, void *, u_long, u_long);
static void *cpucache_alloc(struct kmemcache *, struct kmemmagazine_depot *, void *, u_long, u_long, int);
static void *cpucache_free(struct kmemcache *, struct kmemmagazine_depot *, void *, u_long, u_long, int);

/* kmemslabs: interface */
void kmemslab_init(struct kmemcache *, vm_size_t);
void *kmemslab_alloc(struct kmemcache *, void *, u_long, u_long, int);
void *kmemslab_free(struct kmemcache *, void *, u_long, u_long, int);
void kmemslab_destroy(struct kmemcache *, void *, u_long, u_long, int);

struct kmem_ops kmemslab_ops = {
		.kmops_init = kmemslab_init,
		.kmops_alloc = kmemslab_alloc,
		.kmops_free = kmemslab_free,
		.kmops_destroy = kmemslab_destroy,
};

/* slab metadata functions */

/* calculate slab metadata */
static void
slabmeta_insert(slab, size, index)
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
	meta->ksm_aslots = BUCKET_SLOTS_ALLOCATED(bsize, size);
	meta->ksm_fslots = BUCKET_SLOTS_FREE(bsize, size);
	if (meta->ksm_fslots < 0) {
		meta->ksm_fslots = 0;
	}

	/* running accumulative total */
	meta->ksm_tbslots += meta->ksm_bslots;
	meta->ksm_tfslots += meta->ksm_fslots;
	meta->ksm_taslots += meta->ksm_aslots;

	/* set the minimum and maximum bucket boundaries */
	meta->ksm_min = (u_long)percent((u_long)meta->ksm_bslots,
			(u_long)0); /* lower bucket boundary */
	meta->ksm_max = (u_long)percent((u_long)meta->ksm_bslots,
			(u_long)95); /* upper bucket boundary */

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
slabmeta_lookup(size, index)
	u_long size, index;
{
	struct kmemmeta *meta;

	simple_lock(&malloc_slock);
	LIST_FOREACH(meta, &metalist, ksm_list) {
		if (meta == &metabucket[index]) {
			if ((meta->ksm_bindx == BUCKETINDX(size))
					&& (meta->ksm_bsize == BUCKETSIZE(index))) {
				simple_unlock(&malloc_slock);
				return (meta);
			}
		}
	}
	simple_unlock(&malloc_slock);
	return (NULL);
}

static void
slabmeta_remove(slab, size, index)
	struct kmemslabs *slab;
	u_long size, index;
{
	struct kmemmeta *meta;

	meta = slabmeta_lookup(size, index);
	if (meta != NULL) {
		meta->ksm_tbslots -= meta->ksm_bslots;
		meta->ksm_tfslots -= meta->ksm_fslots;
		meta->ksm_taslots -= meta->ksm_aslots;
		/* remove meta from slab */
		if (meta->ksm_slab == slab) {
			slab->ksl_meta = meta;
		}
		LIST_REMOVE(meta, ksm_list);
		meta->ksm_refcnt--;
	}
}

/* slabcache functions */
static struct kmemcache *
slabcache_create(size)
	vm_size_t size;
{
	struct kmemcache *cache;
	int cpuid;
#ifdef OVERLAY
	cache = (struct kmemcache *)omem_alloc(overlay_map, (vm_size_t)(size * sizeof(struct kmemcache)));
#else
	cache = (struct kmemcache *)kmem_alloc(kernel_map, (vm_size_t)(size * sizeof(struct kmemcache)));
#endif
	for (cpuid = 0; cpuid < NCPUS; cpuid++) {
		cache->ksc_percpu = cpucache_create(NULL, cpuid, MAGAZINE_CAPACITY);
	}
	CIRCLEQ_INIT(&cache->ksc_slablist_empty);
	CIRCLEQ_INIT(&cache->ksc_slablist_partial);
	CIRCLEQ_INIT(&cache->ksc_slablist_full);
	cache->ksc_ops = &kmemslab_ops;
	cache->ksc_slabcount = 0;
	cache->ksc_depotcount = 0;
	return (cache);
}

/* UNUSED */
/*
 * Caution:
 * Using slabcache_destroy could have run on effects in kern_malloc.c, that could lead to
 * memory starvation or a kernel panic, without changing or addressing slabcache initialization.
 * 
 * Potential Changes:
 * 1) Allocate slabcache dynamically within malloc. Instead of once during kmeminit.
 * 2) Allow slabcache to be reinitalized after destruction.
 */
static void
slabcache_destroy(cache)
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

/* slabcache functions */
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
	slabmeta_insert(slab, size, index);
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
	slabmeta_remove(slab, size, index);
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

/* slab functions: */
static struct kmemslabs *
slab_create(cache, size, index, mtype)
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
	slabmeta_insert(slab, size, index);
	cache->ksc_slab = *slab;
	return (slab);
}

static void
slab_destroy(cache, slab, size, index, mtype)
	struct kmemcache *cache;
	struct kmemslabs *slab;
	u_long size, index;
	int mtype;
{
	slab_remove(cache, slab, size, index, mtype);
	slab = NULL;
	cache->ksc_slab = *slab;
}

static void
slab_insert(cache, slab, size, index, mtype)
	struct kmemcache *cache;
	struct kmemslabs *slab;
	u_long size, index;
	int mtype;
{
	register struct kmemmeta *meta;
	int rval;

	meta = slabmeta_lookup(size, index);
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

static void
slab_remove(cache, slab, size, index, mtype)
	struct kmemcache *cache;
	struct kmemslabs *slab;
	u_long size, index;
	int mtype;
{
	register struct kmemmeta *meta;
	int rval;

	meta = slabmeta_lookup(size, index);
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

static struct kmemslabs *
slab_lookup(cache, size, index, mtype)
	struct kmemcache *cache;
	u_long size, index;
	int mtype;
{
	register struct kmemslabs *slab;
	register struct kmemmeta *meta;
	int rval;

	meta = slabmeta_lookup(size, index);
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
	if (slab->ksl_meta != meta) {
		return (NULL);
	}
	return (slab);
}

/* check slots and return matching slab allocation flags */
static int
slab_state(slots, tslots, min, max)
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

/* check and update slab allocation flags */
static int
slab_check(meta)
	struct kmemmeta *meta;
{
	int fslotchk, aslotchk, bslotchk;

	bslotchk = slab_state(meta->ksm_bslots, meta->ksm_tbslots, meta->ksm_min, meta->ksm_max);
	fslotchk = slab_state(meta->ksm_fslots, meta->ksm_tfslots, meta->ksm_min, meta->ksm_max);
	aslotchk = slab_state(meta->ksm_aslots, meta->ksm_taslots, meta->ksm_min, meta->ksm_max);
	return (bslotchk ? fslotchk : aslotchk);
}

/* magazine depot functions */
static struct kmemmagazine_depot *
depot_create(cache, num, capacity)
	struct kmemcache *cache;
	int num, capacity;
{
	register struct kmemmagazine_depot *depot;

	depot = &magazinedepot[num];
	LIST_INIT(&depot->ksmd_magazinelist_empty);
	LIST_INIT(&depot->ksmd_magazinelist_full);
	depot->ksmd_capacity = capacity;
	depot->ksmd_magcount = 0;
	cache->ksc_depot = depot;
	cache->ksc_depotcount++;
	return (depot);
}

static void
depot_insert(depot, mag, object, size, index, nrounds, flags)
	struct kmemmagazine_depot *depot;
	struct kmemmagazine *mag;
	void *object;
	u_long size, index;
	int *nrounds, flags;
{
	mag->ksm_object = object;
	mag->ksm_size = size;
	mag->ksm_index = index;
	mag->ksm_flags = flags;
	simple_lock(&malloc_slock);
	switch (flags) {
	case MAGAZINE_FULL:
		LIST_INSERT_HEAD(&depot->ksmd_magazinelist_full, mag, ksm_list);
		break;
	case MAGAZINE_EMPTY:
		LIST_INSERT_HEAD(&depot->ksmd_magazinelist_empty, mag, ksm_list);
		break;
	}
	simple_unlock(&malloc_slock);
	mag->ksm_rounds--;
	*nrounds = mag->ksm_rounds;
	/* depot update */
	depot->ksmd_magazine = *mag;
	depot->ksmd_magcount++;
}

static struct kmemmagazine *
depot_lookup(depot, object, size, index, flags)
	struct kmemmagazine_depot *depot;
	void *object;
	u_long size, index;
	int flags;
{
	register struct kmemmagazine *mag;

	simple_lock(&malloc_slock);
	switch (flags) {
	case MAGAZINE_FULL:
		LIST_FOREACH(mag, &depot->ksmd_magazinelist_full, ksm_list) {
			if (mag == &magazinebucket[index]) {
				if ((mag->ksm_object == object) && (mag->ksm_size == size)
						&& (mag->ksm_index == index)) {
					simple_unlock(&malloc_slock);
					return (mag);
				}
			}
		}
		break;
	case MAGAZINE_EMPTY:
		LIST_FOREACH(mag, &depot->ksmd_magazinelist_empty, ksm_list) {
			if (mag == &magazinebucket[index]) {
				if ((mag->ksm_object == object) && (mag->ksm_size == size)
						&& (mag->ksm_index == index)) {
					simple_unlock(&malloc_slock);
					return (mag);
				}
			}
		}
		break;
	}
	simple_unlock(&malloc_slock);
	return (NULL);
}

static void
depot_remove(depot, mag, object, size, index, nrounds, flags)
	struct kmemmagazine_depot *depot;
	struct kmemmagazine *mag;
	void *object;
	u_long size, index;
	int *nrounds, flags;
{
	simple_lock(&malloc_slock);
	if ((mag->ksm_object == object) && (mag->ksm_size == size)
			&& (mag->ksm_index == index)) {
		if (flags == (MAGAZINE_FULL | MAGAZINE_EMPTY)) {
			LIST_REMOVE(mag, ksm_list);
		}
	}
	simple_unlock(&malloc_slock);
	mag->ksm_rounds++;
	*nrounds = mag->ksm_rounds;
	/* depot update */
	depot->ksmd_magazine = *mag;
	depot->ksmd_magcount--;
}

/* magazine functions */
static struct kmemmagazine *
magazine_create(depot, object, size, index, flags)
	struct kmemmagazine_depot *depot;
	void *object;
	u_long size, index;
	int flags;
{
	struct kmemmagazine *mag;

	mag = &magazinebucket[index];
	mag->ksm_object = object;
	mag->ksm_size = size;
	mag->ksm_index = index;
	mag->ksm_flags = flags;
	mag->ksm_rounds = depot->ksmd_capacity;
	depot->ksmd_magazine = *mag;
	return (mag);
}

static void
magazine_insert(depot, mag, object, size, index, nrounds)
	struct kmemmagazine_depot *depot;
	struct kmemmagazine *mag;
	void *object;
	u_long size, index;
	int *nrounds;
{
	int rval;

	rval = magazine_state(*nrounds);
	switch (rval) {
	case MAGAZINE_FULL:
		if (mag->ksm_rounds == 0) {
			/* full -> empty */
			depot_insert(depot, mag, object, size, index, nrounds, MAGAZINE_EMPTY);
		} else {
			depot_insert(depot, mag, object, size, index, nrounds, MAGAZINE_FULL);
		}
		break;
	case MAGAZINE_EMPTY:
		if (mag->ksm_rounds > 0) {
			/* empty -> full */
			depot_insert(depot, mag, object, size, index, nrounds, MAGAZINE_FULL);
		} else {
			depot_insert(depot, mag, object, size, index, nrounds, MAGAZINE_EMPTY);
		}
		break;
	}
}

static void
magazine_remove(depot, mag, object, size, index, nrounds)
	struct kmemmagazine_depot *depot;
	struct kmemmagazine *mag;
	void *object;
	u_long size, index;
	int *nrounds;
{
	int rval;

	rval = magazine_state(*nrounds);
	switch (rval) {
	case MAGAZINE_FULL:
		if (mag->ksm_rounds == 0) {
			/* full -> empty */
			depot_remove(depot, mag, object, size, index, nrounds, MAGAZINE_EMPTY);
		} else {
			depot_remove(depot, mag, object, size, index, nrounds, MAGAZINE_FULL);
		}
		break;
	case MAGAZINE_EMPTY:
		if (mag->ksm_rounds > 0) {
			/* empty -> full */
			depot_remove(depot, mag, object, size, index, nrounds, MAGAZINE_FULL);
		} else {
			depot_remove(depot, mag, object, size, index, nrounds, MAGAZINE_EMPTY);
		}
		break;
	}
}

static struct kmemmagazine *
magazine_lookup(depot, object, size, index, nrounds)
	struct kmemmagazine_depot *depot;
	void *object;
	u_long size, index;
	int *nrounds;
{
	struct kmemmagazine *mag;
	int rval;

	rval = magazine_state(*nrounds);
	switch (rval) {
	case MAGAZINE_FULL:
		mag = depot_lookup(depot, object, size, index, MAGAZINE_FULL);
		break;
	case MAGAZINE_EMPTY:
		mag = depot_lookup(depot, object, size, index, MAGAZINE_EMPTY);
		break;
	}
	return (mag);
}

static int
magazine_state(nrounds)
	int nrounds;
{
	int rval;

	rval = 0;
	if (nrounds > 0) {
		rval = MAGAZINE_FULL;
	} else {
		rval = MAGAZINE_EMPTY;
	}
	return (rval);
}

static struct kmemmeta *
magazine_check(slab, mag)
	struct kmemslabs *slab;
	struct kmemmagazine *mag;
{
	struct kmemmeta *meta;

	meta = slabmeta_lookup(mag->ksm_size, mag->ksm_index);
	if (meta != NULL) {
		if (meta == slab->ksl_meta) {
			return (meta);
		}
	}
	return (NULL);
}

/* cpucache functions */
static struct kmemcpu_cache *
cpucache_create(mag, ncpus, capacity)
	struct kmemmagazine *mag;
	int ncpus, capacity;
{
	register struct kmemcpu_cache *mp;

	mp = &cpucache[ncpus];
	mp->kscp_rounds_current = capacity;
	mp->kscp_rounds_previous = capacity;
	mp->kscp_magazine_current = mag;
	mp->kscp_magazine_previous = mag;
	return (mp);
}

static void
cpucache_magazine_alloc(cache, depot, object, size, index)
	struct kmemcache *cache;
	struct kmemmagazine_depot **depot;
	void *object;
	u_long size, index;
{
	register struct kmemcpu_cache *mp;
	int cpuid;

	for (cpuid = 0; cpuid < NCPUS; cpuid++) {
		mp = &cache->ksc_percpu[cpuid];
		mp->kscp_magazine_current = magazine_create(*depot, object, size, index,
				MAGAZINE_EMPTY);
		mp->kscp_magazine_previous = magazine_create(*depot, object, size, index,
				MAGAZINE_EMPTY);
	}
}

static void *
cpucache_alloc(cache, depot, object, size, index, cpuid)
	struct kmemcache *cache;
	struct kmemmagazine_depot *depot;
	void *object;
	u_long size, index;
	int cpuid;
{
	register struct kmemmagazine *mag;
	register struct kmemcpu_cache *mp;

	mp = &cache->ksc_percpu[cpuid];
retry:
	if (mp->kscp_magazine_current != NULL) {
		mag = mp->kscp_magazine_current;
		magazine_insert(depot, mag, object, size, index,
				&mp->kscp_rounds_current);
		cache->ksc_depot = depot;
		return (mag->ksm_object);
	}
	if (mp->kscp_magazine_previous != NULL) {
		mag = mp->kscp_magazine_previous;
		magazine_insert(depot, mag, object, size, index,
				&mp->kscp_rounds_previous);
		cache->ksc_depot = depot;
		return (mag->ksm_object);
	}
	if (mag != mp->kscp_magazine_current || mag != mp->kscp_magazine_previous) {
		if (mp->kscp_rounds_current > 0 || mp->kscp_rounds_previous > 0) {
			goto retry;
		}
	}
	return (NULL);
}

static void *
cpucache_free(cache, depot, object, size, index, cpuid)
	struct kmemcache *cache;
	struct kmemmagazine_depot *depot;
	void *object;
	u_long size, index;
	int cpuid;
{
	register struct kmemmagazine *mag;
	register struct kmemcpu_cache *mp;

	mp = &cache->ksc_percpu[cpuid];
retry:
	if (mp->kscp_magazine_current != NULL) {
		mag = magazine_lookup(depot, object, size, index,
				&mp->kscp_rounds_current);
		if (mag == mp->kscp_magazine_current) {
			magazine_remove(depot, mag, object, size, index,
					&mp->kscp_rounds_current);
			cache->ksc_depot = depot;
			return (mag->ksm_object);
		}
	}
	if (mp->kscp_magazine_previous != NULL) {
		mag = magazine_lookup(depot, object, size, index,
				&mp->kscp_rounds_previous);
		if (mag == mp->kscp_magazine_previous) {
			magazine_remove(depot, mag, object, size, index,
					&mp->kscp_rounds_previous);
			cache->ksc_depot = depot;
			return (mag->ksm_object);
		}
	}
	if (mag != mp->kscp_magazine_current || mag != mp->kscp_magazine_previous) {
		if (mp->kscp_rounds_current == 0 || mp->kscp_rounds_previous == 0) {
			goto retry;
		}
	}
	return (NULL);
}

/* Run's once only during startup within kmeminit (see kern_malloc.c) */
void
kmemslab_init(cache, size)
	struct kmemcache *cache;
	vm_size_t size;
{
	register struct kmemcache *ksc;

	ksc = slabcache_create(size);
	if (ksc != NULL) {
		cache = ksc;
	}
}

void *
kmemslab_alloc(cache, object, size, index, mtype)
	struct kmemcache *cache;
	void *object;
	u_long size, index;
	int mtype;
{
	register struct kmemslabs *slab;
	struct kmemmagazine_depot *depot;
	struct kmemmagazine *mag;
	struct kmemmeta *meta;
	void *obj;
	int ndepot;

	ndepot = 0;
	/* create & allocate slab */
	slab = slab_create(cache, size, index, mtype);
	if (slab != NULL) {
		slab_insert(cache, slab, size, index, mtype);
retry:
		if (ndepot < MAGAZINE_DEPOT_MAX) {
			/* create magazine depot */
			depot = depot_create(cache, ndepot, MAGAZINE_CAPACITY);
			if (depot != NULL) {
				/* allocated cpucache magazines */
				cpucache_magazine_alloc(cache, &depot, object, size, index);
				/* allocated magazine object */
				obj = cpucache_alloc(cache, depot, object, size, index, cpu_number());
				if (obj != NULL) {
					/* retrieve magazine from depot */
					mag = &depot->ksmd_magazine;
					/* check magazine size and index against meta */
					meta = magazine_check(slab, mag);
					if (meta != NULL) {
						return (obj);
					}
					/* retry in alternative depot */
					ndepot += 1;
					goto retry;
				}
			}
		}
	}
	return (NULL);
}

void *
kmemslab_free(cache, object, size, index, mtype)
	struct kmemcache *cache;
	void *object;
	u_long size, index;
	int mtype;
{
	register struct kmemslabs *slab;
	struct kmemmagazine_depot *depot;
	struct kmemmagazine *mag;
	struct kmemmeta *meta;
	void *obj;
	int ndepot;

	ndepot = 0;
	/* lookup & free slab */
	slab = slab_lookup(cache, size, index, mtype);
	if (slab != NULL) {
		slab_remove(cache, slab, size, index, mtype);
retry:
		if (ndepot < MAGAZINE_DEPOT_MAX) {
			/* retrieve magazine depot */
			depot = &cache->ksc_depot[ndepot];
			if (depot != NULL) {
				/* free magazine object */
				obj = cpucache_free(cache, depot, object, size, index, cpu_number());
				if (obj != NULL) {
					/* retrieve magazine from depot */
					mag = &depot->ksmd_magazine;
					/* check magazine size and index against meta */
					meta = magazine_check(slab, mag);
					if (meta != NULL) {
						return (obj);
					}
					/* retry in alternative depot */
					ndepot += 1;
					goto retry;
				}
			}
		}
	}
	return (NULL);
}

void
kmemslab_destroy(cache, object, size, index, mtype)
	struct kmemcache *cache;
	void *object;
	u_long size, index;
	int mtype;
{
	register struct kmemslabs *slab;
	void *obj;

	/* lookup slab */
	slab = slab_lookup(cache, size, index, mtype);
	if (slab != NULL) {
		/* free slab */
		obj = kmemslab_free(cache, object, size, index, mtype);
		if (obj == NULL) {
			/* destroy slab */
			slab_destroy(cache, slab, size, index, mtype);
		} else {
			/* slab object was not freed */
			panic("kmemslab_destroy: failed to free object");
		}
	}
}
