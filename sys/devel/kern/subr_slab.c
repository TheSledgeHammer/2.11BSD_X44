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

struct kmemslabs_cache *slabcache;
struct kmemslabs slabbucket[MINBUCKET + 16];
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

/* slab metadata */
void
kmemslab_meta(slab, size)
	struct kmemslabs *slab;
	u_long  size;
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
	meta->ksm_bslots = BUCKET_SLOTS(bsize);
	meta->ksm_aslots = ALLOCATED_SLOTS(size);
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

/* slab magazine functions */
struct kmemslabs_magazine *
kmemslab_magazine_create(cache, size)
	struct kmemslabs_cache *cache;
	vm_size_t size;
{
	struct kmemslabs_magazine *mag;
#ifdef OVERLAY
	mag = (struct kmemslabs_magazine *)omem_alloc(overlay_map, (vm_size_t)(size * sizeof(struct kmemslabs_magazine)));
#else
	mag = (struct kmemslabs_magazine *)kmem_alloc(kernel_map, (vm_size_t)(size * sizeof(struct kmemslabs_magazine)));
#endif
	CIRCLEQ_INIT(&cache->ksc_maglist);
	mag->ksm_refcount = 0;
	return (mag);
}

void
kmemslab_magazine_destroy(cache, mag)
	struct kmemslabs_cache *cache;
	struct kmemslabs_magazine *mag;
{
	if (mag != NULL) {
#ifdef OVERLAY
		omem_free(overlay_map, (vm_offset_t)mag, (vm_size_t)sizeof(struct kmemslabs_magazine));
#else
		kmem_free(kernel_map, (vm_offset_t)mag, (vm_size_t)sizeof(struct kmemslabs_magazine));
#endif
	}
}

void
kmemslab_magazine_insert(cache, mag, flags, stype)
	struct kmemslabs_cache *cache;
	struct kmemslabs_magazine *mag;
	int flags, stype;
{
	simple_lock(&malloc_slock);
	switch (flags) {
	case SLAB_FULL:
		if (stype == SLAB_SMALL) {
			CIRCLEQ_INSERT_HEAD(&cache->ksc_maglist, mag, ksm_full);
		} else {
			CIRCLEQ_INSERT_TAIL(&cache->ksc_maglist, mag, ksm_full);
		}
		break;
	case SLAB_PARTIAL:
		if (stype == SLAB_SMALL) {
			CIRCLEQ_INSERT_HEAD(&cache->ksc_maglist, mag, ksm_partial);
		} else {
			CIRCLEQ_INSERT_TAIL(&cache->ksc_maglist, mag, ksm_partial);
		}
		break;
	case SLAB_EMPTY:
		if (stype == SLAB_SMALL) {
			CIRCLEQ_INSERT_HEAD(&cache->ksc_maglist, mag, ksm_empty);
		} else {
			CIRCLEQ_INSERT_TAIL(&cache->ksc_maglist, mag, ksm_empty);
		}
		break;
	}
	simple_unlock(&malloc_slock);
    mag->ksm_refcount++;
}

void
kmemslab_magazine_remove(cache, mag, flags)
	struct kmemslabs_cache *cache;
	struct kmemslabs_magazine *mag;
	int flags;
{
	simple_lock(&malloc_slock);
	switch (flags) {
	case SLAB_FULL:
		CIRCLEQ_REMOVE(&cache->ksc_maglist, mag, ksm_full);
		break;
	case SLAB_PARTIAL:
		CIRCLEQ_REMOVE(&cache->ksc_maglist, mag, ksm_partial);
		break;
	case SLAB_EMPTY:
		CIRCLEQ_REMOVE(&cache->ksc_maglist, mag, ksm_empty);
		break;
	}
	simple_unlock(&malloc_slock);
	mag->ksm_refcount--;
}

/* slabcache functions */
struct kmemslabs_cache *
kmemslab_cache_create(size)
	vm_size_t size;
{
	struct kmemslabs_cache *cache;
#ifdef OVERLAY
	cache = (struct kmemslabs_cache *)omem_alloc(overlay_map, (vm_size_t)(size * sizeof(struct kmemslabs_cache)));
#else
	cache = (struct kmemslabs_cache *)kmem_alloc(kernel_map, (vm_size_t)(size * sizeof(struct kmemslabs_cache)));
#endif
	CIRCLEQ_INIT(&cache->ksc_slablist);
	cache->ksc_refcount = 0;
	return (cache);
}

void
kmemslab_cache_destroy(cache)
	struct kmemslabs_cache *cache;
{
	if (cache != NULL) {
#ifdef OVERLAY
		omem_free(overlay_map, (vm_offset_t)cache, (vm_size_t)sizeof(struct kmemslabs_cache));
#else
		kmem_free(kernel_map, (vm_offset_t)cache, (vm_size_t)sizeof(struct kmemslabs_cache));
#endif
	}
}

void
kmemslab_cache_alloc(cache, size, index, mtype)
	struct kmemslabs_cache 	*cache;
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
	cache->ksc_refcount++;

	/* update metadata */
	kmemslab_meta(slab, size);
}

void
kmemslab_cache_free(cache, size, index)
	struct kmemslabs_cache 	*cache;
	long size;
	u_long index;
{
	register struct kmemslabs *slab;

	slab = &slabbucket[index];

	simple_lock(&malloc_slock);
	CIRCLEQ_REMOVE(&cache->ksc_slablist, slab, ksl_list);
	simple_unlock(&malloc_slock);
	cache->ksc_slab = *slab;
	cache->ksc_refcount--;

	/* update metadata */
	kmemslab_meta(slab, size);
}

struct kmemslabs *
kmemslab_cache_lookup(cache, size, index, mtype)
	struct kmemslabs_cache 	*cache;
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
kmemslab_create(cache, size, mtype)
	struct kmemslabs_cache 	*cache;
	long size;
	int mtype;
{
	register struct kmemslabs *slab;

	kmemslab_cache_alloc(cache, size, BUCKETINDX(size), mtype);
	slab = &cache->ksc_slab;
	return (slab);
}

void
kmemslab_destroy(cache, size)
	struct kmemslabs_cache 	*cache;
	long  size;
{
	register struct kmemslabs *slab;

	slab = &cache->ksc_slab;
	if (slab != NULL) {
		slab = NULL;
		cache->ksc_slab = *slab;
	}
}

void
kmemslab_alloc(cache, size, mtype)
	struct kmemslabs_cache *cache;
	vm_size_t size;
	int	mtype;
{
	register struct kmemslabs *slab;

	slabcache_alloc(cache, size, BUCKETINDX(size), mtype);
	slab = &cache->ksc_slab;
}

void
kmemslab_free(cache, size)
	struct kmemslabs_cache 	*cache;
	long  size;
{
	register struct kmemslabs *slab;

	kmemslab_cache_free(cache, size, BUCKETINDX(size));
	slab = &cache->ksc_slab;
}

struct kmemslabs *
kmemslab_get_by_size(cache, size, mtype)
	struct kmemslabs_cache 	*cache;
	long    		size;
	int 			mtype;
{
	return (kmemslab_cache_lookup(cache, size, BUCKETINDX(size), mtype));
}

struct kmemslabs *
kmemslab_get_by_index(cache, index, mtype)
	struct kmemslabs_cache 	*cache;
	u_long index;
	int mtype;
{
	return (kmemslab_cache_lookup(cache, BUCKETSIZE(index), index, mtype));
}

struct kmembuckets *
kmemslab_bucket(slab)
	struct kmemslabs *slab;
{
    if (slab->ksl_bucket != NULL) {
        return (slab->ksl_bucket);
    }
    return (NULL);
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
