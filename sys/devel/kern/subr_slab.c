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
 * @(#)subr_slab.c	1.0 (Martin Kelly) 12/8/25
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
#include <sys/queue.h>
#include <sys/malloc.h>

#include <arch/i386/include/param.h>

void kmemslab_init(struct kmemcache *, vm_size_t);
void *kmemslab_alloc(struct kmemcache *, void *, u_long, u_long, int);
void kmemslab_free(struct kmemcache *, void *, u_long, u_long, int);
void kmemslab_destroy(struct kmemcache *, void *, u_long, u_long, int);

struct kmem_ops kmemslab_ops = {
		.kmops_init = kmemslab_init,
		.kmops_alloc = kmemslab_alloc,
		.kmops_free = kmemslab_free,
		.kmops_destroy = kmemslab_destroy,
};

struct kmemmagazine_depot magazinedepot[MAGAZINE_DEPOT_MAX];
struct kmemcpu_cache slabcachemp[NCPUS];

static int magazine_state(int);

struct kmemcpu_cache *
slabcache_mp_create(mag, ncpus, capacity)
	struct kmemmagazine *mag;
	int ncpus, capacity;
{
	struct kmemcpu_cache *cachemp;

	cachemp = &slabcachemp[ncpus];
	cachemp->kscp_rounds_current = capacity;
	cachemp->kscp_magazine_previous = capacity;
	cachemp->kscp_magazine_current = mag;
	cachemp->kscp_magazine_previous = mag;
	return (cachemp);
}

struct kmemcache *
kmemslab_cache_create(size)
	vm_size_t size;
{
	register struct kmemcache *cache;
	int cpuid;
#ifdef OVERLAY
	cache = (struct kmemcache *)omem_alloc(overlay_map, (vm_size_t)(size * sizeof(struct kmemcache)));
#else
	cache = (struct kmemcache *)kmem_alloc(kernel_map, (vm_size_t)(size * sizeof(struct kmemcache)));
#endif

    for (cpuid = 0; cpuid < NCPUS; cpuid++) {
        cache->ksc_percpu = slabcache_mp_create(NULL, cpuid, MAGAZINE_CAPACITY);
    }
	CIRCLEQ_INIT(&cache->ksc_slablist_empty);
	CIRCLEQ_INIT(&cache->ksc_slablist_partial);
	CIRCLEQ_INIT(&cache->ksc_slablist_full);
	cache->ksc_ops = &kmemslab_ops;
	cache->ksc_slabcount = 0;
	cache->ksc_depotcount = 0;
	return (cache);
}

/* magazine functions */
struct kmemmagazine *
magazine_create(depot, object, size, index, flags)
	struct kmemmagazine_depot *depot;
	void *object;
	unsigned long size, index;
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

void
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

void
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

struct kmemmagazine *
magazine_lookup(depot, object, size, index, nrounds)
	struct kmemmagazine_depot *depot;
	void *object;
	unsigned long size, index;
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
	return (depot);
}

/* depot functions */
struct kmemmagazine_depot *
depot_create(cache, num, capacity)
	struct kmemcache *cache;
	int num, capacity;
{
	struct kmemmagazine_depot *depot = NULL;

	depot = &magazinedepot[num];
	LIST_INIT(&depot->ksmd_magazinelist_empty);
	LIST_INIT(&depot->ksmd_magazinelist_full);
	depot->ksmd_capacity = capacity;
	depot->ksmd_magcount = 0;
	cache->ksc_depot = *depot;
	cache->ksc_depotcount++;
	return (depot);
}

void
depot_insert(depot, mag, object, size, index, nrounds, flags)
	struct kmemmagazine_depot *depot;
	struct kmemmagazine *mag;
	void *object;
	unsigned long size, index;
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

struct kmemmagazine *
depot_lookup(depot, object, size, index, flags)
	struct kmemmagazine_depot *depot;
	void *object;
	unsigned long size, index;
	int flags;
{
	struct kmemmagazine *mag;

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

void
depot_remove(depot, mag, object, size, index, nrounds, flags)
	struct kmemmagazine_depot *depot;
	struct kmemmagazine *mag;
	void *object;
	unsigned long size, index;
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

struct kmemmeta *
magazine_check(size, index)
	unsigned long size, index;
{
	struct kmemmeta *meta;

	meta = kmemslab_meta_lookup(size, index);
	if (meta != NULL) {
		return (meta);
	}
	return (NULL);
}

void
object_alloc(cache, depot, object, size, index, ndepots)
	struct kmemcache *cache;
	struct kmemmagazine_depot **depot;
	void *object;
	unsigned long size, index;
	int ndepots;
{
	register struct kmemmagazine_depot *dep;
	register struct kmemcpu_cache *mp;
	int cpuid;

	dep = depot_create(cache, ndepots, MAGAZINE_CAPACITY);
	for (cpuid = 0; cpuid < NCPUS; cpuid++) {
		mp = &cache->ksc_percpu[cpuid];
		mp->kscp_magazine_current = magazine_create(dep, object, size, index,
				MAGAZINE_EMPTY);
		mp->kscp_magazine_previous = magazine_create(dep, object, size, index,
				MAGAZINE_EMPTY);
	}
	*depot = dep;
}

void *
object_put(cache, depot, object, size, index, cpuid)
	struct kmemcache *cache;
	struct kmemmagazine_depot *depot;
	void *object;
	unsigned long size, index;
	int cpuid;
{
	register struct kmemmagazine *mag;
	register struct kmemcpu_cache *mp;
	int retries;

	retries = 0;
	mp = &cache->ksc_percpu[cpuid];
retry:
	if (mp->kscp_rounds_current > 0) {
		mag = mp->kscp_magazine_current;
		magazine_insert(depot, mag, object, size, index,
				&mp->kscp_rounds_current);
		return (mag->ksm_object);
	}
	if (mp->kscp_rounds_previous > 0) {
		mag = mp->kscp_magazine_previous;
		magazine_insert(depot, mag, object, size, index,
				&mp->kscp_rounds_previous);
		return (mag->ksm_object);
	}
	if (retries < 2) {
		if (mag != mp->kscp_magazine_current
				|| mag != mp->kscp_magazine_previous) {
			if (mp->kscp_rounds_current == 0 || mp->kscp_rounds_previous == 0) {
				retries++;
				goto retry;
			}
		}
	}
	return (NULL);
}

void *
object_get(cache, depot, object, size, index, cpuid)
	struct kmemcache *cache;
	struct kmemmagazine_depot *depot;
	void *object;
	unsigned long size, index;
	int cpuid;
{
	register struct kmemmagazine *mag;
	register struct kmemcpu_cache *mp;
	int retries;

	retries = 0;
	mp = &cache->ksc_percpu[cpuid];
retry:
	if (mp->kscp_magazine_current != NULL) {
		mag = magazine_lookup(depot, object, size, index,
				&mp->kscp_rounds_current);
		if (mag == mp->kscp_magazine_current) {
			magazine_remove(depot, mag, object, size, index,
					&mp->kscp_rounds_current);
			return (mag->ksm_object);
		}
	}
	if (mp->kscp_magazine_previous != NULL) {
		mag = magazine_lookup(depot, object, size, index,
				&mp->kscp_rounds_previous);
		if (mag == mp->kscp_magazine_previous) {
			magazine_remove(depot, mag, object, size, index,
					&mp->kscp_rounds_previous);
			return (mag->ksm_object);
		}
	}
	if (retries < 2) {
		if (mag != mp->kscp_magazine_current
				|| mag != mp->kscp_magazine_previous) {
			if (mp->kscp_rounds_current > 0 || mp->kscp_rounds_previous > 0) {
				retries++;
				goto retry;
			}
		}
	}
	return (NULL);
}

void
slab_alloc(cache, object, size, index, mtype)
	struct kmemcache *cache;
	void *object;
	unsigned long size, index;
	int mtype;
{
	register struct kmemslabs *slab;
	register struct kmemmagazine *mag;
	register struct kmembuckets *kbp;

	struct kmemmeta *meta;

	/* create & allocate slab */
	slab = kmemslab_create(cache, size, index, mtype);
	if (slab != NULL) {
		kbp = slab->ksl_bucket;
		/* insert slab */
		/* allocate magazine */
		/* check magazine size and index against meta */
		meta = magazine_check(mag->ksm_size, mag->ksm_index);
		if (meta != NULL) {
			/* check slab against meta or the magazine size will fit slab's size */
			if (slab->ksl_size >= mag->ksm_size || slab->ksl_meta == meta) {
				/* insert magazine */
				/* return kmem_ops alloc */
			} else {
				/* free magazine */
				/* remove inserted slab */
				/* return null */
			}
		}
	}


	if (kbp != NULL) {
		kmemslab_insert(cache, slab, size, index, mtype);
	}
}

void
kmemslab_init(cache, size)
	struct kmemcache *cache;
	vm_size_t size;
{
	struct kmem_ops *ops;

	op = &cache->ksc_ops;

	(*ops->kmops_init)(cache, size);
}

void *
kmemslab_alloc(cache, object, size, index, mtype)
	struct kmemcache *cache;
	void *object;
	unsigned long size, index;
	int mtype;
{
	struct kmem_ops *ops;

	op = &cache->ksc_ops;

	return ((*ops->kmops_alloc)(cache, object, size, index, mtype));
}

void
kmemslab_free(cache, object, size, index, mtype)
	struct kmemcache *cache;
	void *object;
	unsigned long size, index;
	int mtype;
{
	struct kmem_ops *ops;

	op = &cache->ksc_ops;

	(*ops->kmops_free)(cache, object, size, index, mtype);
}

void
kmemslab_destroy(cache, object, size, index, mtype)
	struct kmemcache *cache;
	void *object;
	unsigned long size, index;
	int mtype;
{
	struct kmem_ops *ops;

	op = &cache->ksc_ops;

	(*ops->kmops_destroy)(cache, object, size, index, mtype);
}

void *
slab_get()
{
	register struct kmemslabs *slab;

	slab = kmemslab_lookup(cache, size, index, mtype);
	if (slab != NULL) {
		return (slab);
	}
}
