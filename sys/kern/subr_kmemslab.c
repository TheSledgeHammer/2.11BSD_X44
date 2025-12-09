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

LIST_HEAD(, kmemmeta) metalist = LIST_HEAD_INITIALIZER(metalist);
CIRCLEQ_HEAD(, kmemmagazine) maglist;// = CIRCLEQ_HEAD_INITIALIZER(maglist);

static int slab_check(struct kmemmeta *);
/* slabmeta */
static void kmemslab_meta_insert(struct kmemslabs *, u_long, u_long);
static struct kmemmeta *kmemslab_meta_lookup(u_long, u_long);
static void kmemslab_meta_remove(struct kmemslabs *, u_long, u_long);
/* slabcache */
static void slabcache_insert(struct kmemcache *, struct kmemslabs *, u_long, u_long, int, int);
static void slabcache_remove(struct kmemcache *, struct kmemslabs *, u_long, u_long, int, int);
static struct kmemslabs *slabcache_lookup(struct kmemcache *, u_long, u_long, int, int);
static struct kmemslabs *slabcache_lookup_normal(struct kmemcache *, u_long, u_long, int, int);
static struct kmemslabs *slabcache_lookup_reverse(struct kmemcache *, u_long, u_long, int, int);
static int slots_check(int, int, u_long, u_long);

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
kmemslab_meta_lookup(size, index)
	u_long size, index;
{
	struct kmemmeta *meta;
	simple_lock(&malloc_slock);
	LIST_FOREACH(meta, &metalist, ksm_list) {
		if ((meta == &metabucket[index])
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

	meta = kmemslab_meta_lookup(size, index);
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

/* slab magazine functions */
struct kmemmagazine *
kmemslab_magazine_create(cache, size, index)
	struct kmemcache *cache;
	u_long size, index;
{
	register struct kmemmagazine *mag;

	mag = &magazinebucket[index];
	CIRCLEQ_INIT(&maglist);
	//mag->ksm_maxslots = SLOTSFREE(bsize, size);
	mag->ksm_freeslots = mag->ksm_maxslots;
	cache->ksc_magazine = *mag;
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
		CIRCLEQ_INSERT_HEAD(&maglist, mag, ksm_list);
	} else {
		CIRCLEQ_INSERT_TAIL(&maglist, mag, ksm_list);
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
	CIRCLEQ_REMOVE(&maglist, mag, ksm_list);
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
	cache->ksc_magcount = 0;
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

/* slab functions: new */
void
kmemslab_init(cache, size)
	struct kmemcache *cache;
	vm_size_t size;
{
	struct kmemcache *ksc;

	ksc = kmemslab_cache_create(size);
	if (ksc != NULL) {
		cache = ksc;
	}
}

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
kmemslab_destroy(cache, slab, size, index, mtype)
	struct kmemcache *cache;
	struct kmemslabs *slab;
	u_long size, index;
	int mtype;
{
	kmemslab_remove(cache, slab, size, index, mtype);
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
	register struct kmemmeta *meta;
	int rval;

	meta = kmemslab_meta_lookup(size, index);
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
	register struct kmemmeta *meta;
	int rval;

	meta = kmemslab_meta_lookup(size, index);
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
kmemslab_lookup(cache, size, index, mtype)
	struct kmemcache *cache;
	u_long size, index;
	int mtype;
{
	register struct kmemslabs *slab;
	register struct kmemmeta *meta;
	int rval;

	meta = kmemslab_meta_lookup(size, index);
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

/*
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
*/

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
