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
 * A Simple Slab Allocator.
 * The Slab allocator treats each bucket in kmembuckets as a pool with "x" slots available.
 *
 * Slots are determined by the bucket index and the object size being allocated.
 * Slab metadata is responsible for retaining all slot related data and some slab information
 * (i.e. slab state (partial, full or empty) and slab size (large or small))
 *
 * Slab allows for small and large objects by splitting kmembuckets into two ranges,
 * as determined by the bucket index.
 * Any objects with a bucket index less than 10 are flagged as small, while objects
 * greater than or equal to 10 are flagged as large.
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

struct kmemslabs_cache  *slabcache;
struct kmemslabs		slabbucket[MINBUCKET + 16];

struct kmembucket 		kmembucket[MINBUCKET + 16];
struct kmemstats 		kmemstats[M_LAST];
struct kmemusage 		*kmemusage;
struct lock_object 		malloc_slock;
vm_map_t				kmem_map;
char 					*kmembase;
char 					*kmemlimit;
char *memname[] = INITKMEMNAMES;

#ifdef OVERLAY
ovl_map_t				omem_map;
#endif

/* [internal use only] */
caddr_t	kmalloc(unsigned long, int);
void	kfree(void *, short);

#ifdef OVERLAY
caddr_t	omalloc(unsigned long, int);
void	ofree(void *, short, int);
#endif

#ifdef DIAGNOSTIC

/* This structure provides a set of masks to catch unaligned frees. */
long addrmask[] = { 0,
	0x00000001, 0x00000003, 0x00000007, 0x0000000f,
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
	0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
	0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
};

#define WEIRD_ADDR	0xdeadbeef
#define MAX_COPY	32

struct freelist {
	long	spare0;
	short	type;
	long	spare1;
	caddr_t	next;
};
#else /* !DIAGNOSTIC */
struct freelist {
	caddr_t	next;
};
#endif /* DIAGNOSTIC */

/* slab metadata */
void
slabmeta(slab, size)
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

/* slabcache functions */
struct kmemslabs_cache *
slabcache_create(map, size)
	vm_map_t  map;
	vm_size_t size;
{
	struct kmemslabs_cache *cache;

	cache = (struct kmemslabs_cache *)kmem_alloc(map, (vm_size_t)(size * sizeof(struct kmemslabs_cache)));
	CIRCLEQ_INIT(&cache->ksc_slablist);
	cache->ksc_slabcount = 0;
	return (cache);
}

void
slabcache_alloc(cache, size, index, mtype)
	struct kmemslabs_cache 	*cache;
	long    		size;
	u_long 			index;
	int 			mtype;
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
	struct kmemslabs_cache 	*cache;
	long    		size;
	u_long 			index;
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
	struct kmemslabs_cache 	*cache;
	long    		size;
	u_long    		index;
	int 			mtype;
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
	struct kmemslabs_cache 	*cache;
	long    		size;
	int 			mtype;
{
    return (slabcache_lookup(cache, size, BUCKETINDX(size), mtype));
}

struct kmemslabs *
slab_get_by_index(cache, index, mtype)
	struct kmemslabs_cache 	*cache;
	u_long    		index;
	int 			mtype;
{
    return (slabcache_lookup(cache, BUCKETSIZE(index), index, mtype));
}

struct kmemslabs *
slab_create(cache, size, mtype)
	struct kmemslabs_cache 	*cache;
    long  size;
    int	  mtype;
{
    register struct kmemslabs *slab;

	slabcache_alloc(cache, size, BUCKETINDX(size), mtype);
	slab = &cache->ksc_slab;
	return (slab);
}

void
slab_destroy(cache, size)
	struct kmemslabs_cache 	*cache;
	long  size;
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
	struct kmemslabs_cache **cache;
	vm_map_t  map;
	vm_size_t size;
{
	struct kmemslabs_cache *ksc;
	long indx;

	/* Initialize slabs kmembuckets */
	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		slabbucket[indx].ksl_bucket = &kmembucket[indx];
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
	struct kmemslabs_cache 	*cache;
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
	bslots = BUCKET_SLOTS(bsize);
	aslots = ALLOCATED_SLOTS(slab->ksl_size);
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

/* Allocate a block of memory */
void *
malloc(size, type, flags)
    unsigned long size;
    int type, flags;
{
    register struct kmemslabs *slab;
    register struct kmembuckets *kbp;
    register struct kmemusage *kup;
    register struct freelist *freep;
    long indx, npg, allocsize;
    int s;
    caddr_t  va, cp, savedlist;
#ifdef DIAGNOSTIC
    long *end, *lp;
    int copysize;
    char *savedtype;
#endif
#ifdef KMEMSTATS
	register struct kmemstats *ksp = &kmemstats[type];
	if (((unsigned long)type) > M_LAST)
		panic("malloc - bogus type");
#endif
	slab = slab_create(&slabcache, size, type);
    kbp = kmembucket_search(&slabcache, slab->ksl_meta, size, type);
    s = splimp();
    simple_lock(&malloc_slock);
#ifdef KMEMSTATS
    while (ksp->ks_memuse >= ksp->ks_limit) {
    	if (flags & M_NOWAIT) {
    		splx(s);
    		return ((void *) NULL);
    	}
    	if (ksp->ks_limblocks < 65535)
    		ksp->ks_limblocks++;
    	tsleep((caddr_t)ksp, PSWP+2, memname[type], 0);
    }
    ksp->ks_size |= 1 << indx;
#endif
#ifdef DIAGNOSTIC
    copysize = 1 << indx < MAX_COPY ? 1 << indx : MAX_COPY;
#endif
#ifdef DEBUG
	if (flags & M_NOWAIT)
		/* do nothing */
#endif
    if (kbp->kb_next == NULL) {
    	kbp->kb_last = NULL;
    	simple_unlock(&malloc_slock);
#ifdef OVERLAY
		if (flags & M_OVERLAY) {
			va = omalloc(size, flags);
		} else {
			va = kmalloc(size, flags);
		}
#else
    	va = kmalloc(size, flags);
#endif
        if (va == NULL) {
        	splx(s);
#ifdef DEBUG
        if (flags & (M_NOWAIT | M_CANFAIL))
        	panic("malloc: out of space in kmem_map");
#endif
        	return ((void *) NULL);
        }
        simple_lock(&malloc_slock);
#ifdef KMEMSTATS
		kbp->kb_total += kbp->kb_elmpercl;
#endif
		kup = btokup(va);
		kup->ku_indx = indx;
		if (allocsize > MAXALLOCSAVE) {
			if (npg > 65535)
				panic("malloc: allocation too large");
			kup->ku_pagecnt = npg;
#ifdef KMEMSTATS
			ksp->ks_memuse += allocsize;
#endif
			goto out;
		}
#ifdef KMEMSTATS
		kup->ku_freecnt = kbp->kb_elmpercl;
		kbp->kb_totalfree += kbp->kb_elmpercl;
#endif
		savedlist = kbp->kb_next;
		kbp->kb_next = cp = va + (npg * NBPG) - allocsize;
		for (;;) {
			freep = (struct freelist *)cp;
#ifdef DIAGNOSTIC
			end = (long *)&cp[copysize];
			for (lp = (long *)cp; lp < end; lp++)
				*lp = WEIRD_ADDR;
			freep->type = M_FREE;
#endif /* DIAGNOSTIC */
			if (cp <= va) {
				break;
			}
			cp -= allocsize;
			freep->next = cp;
		}
		freep->next = savedlist;
		if (kbp->kb_last == NULL)
			kbp->kb_last = (caddr_t)freep;
    }
    va = kbp->kb_next;
    kbp->kb_next = ((struct freelist *)va)->next;
#ifdef DIAGNOSTIC
    freep = (struct freelist *)va;
    savedtype = (unsigned)freep->type < M_LAST ? memname[freep->type] : "???";
    if (kbp->kb_next && !kernacc(kbp->kb_next, sizeof(struct freelist), 0)) {
    	printf("%s of object 0x%x size %d %s %s (invalid addr 0x%x)\n",
    				"Data modified on freelist: word 2.5", va, size,
    				"previous type", savedtype, kbp->kb_next);
    	kbp->kb_next = NULL;
    }
#if BYTE_ORDER == BIG_ENDIAN
	freep->type = WEIRD_ADDR >> 16;
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
	freep->type = (short)WEIRD_ADDR;
#endif
	if (((long)(&freep->next)) & 0x2)
		freep->next = (caddr_t)((WEIRD_ADDR >> 16)|(WEIRD_ADDR << 16));
	else
		freep->next = (caddr_t)WEIRD_ADDR;
	end = (long *)&va[copysize];
	for (lp = (long *)va; lp < end; lp++) {
		if (*lp == WEIRD_ADDR)
			continue;
		printf("%s %d of object 0x%x size %d %s %s (0x%x != 0x%x)\n",
				"Data modified on freelist: word", lp - (long *)va,
				va, size, "previous type", savedtype, *lp, WEIRD_ADDR);
		break;
	}
	freep->spare0 = 0;
#endif /* DIAGNOSTIC */
#ifdef KMEMSTATS
	kup = btokup(va);
	if (kup->ku_indx != indx)
		panic("malloc: wrong bucket");
	if (kup->ku_freecnt == 0)
		panic("malloc: lost data");
	kup->ku_freecnt--;
	kbp->kb_totalfree--;
	ksp->ks_memuse += 1 << indx;
out:
	kbp->kb_calls++;
	ksp->ks_inuse++;
	ksp->ks_calls++;
	if (ksp->ks_memuse > ksp->ks_maxused)
		ksp->ks_maxused = ksp->ks_memuse;
#else
out:
#endif
	simple_unlock(&malloc_slock);
	splx(s);
	if ((flags & M_ZERO) && va != NULL)
		bzero(va, size);
	return ((void *) va);
}

/* Free a block of memory allocated by malloc */
void
free(addr, type)
	void *addr;
	int type;
{
	register struct kmemslabs 	*slab;
	register struct kmembuckets *kbp;
	register struct kmemusage 	*kup;
	register struct freelist 	*freep;
	long size;
	int s;
#ifdef DIAGNOSTIC
	caddr_t cp;
	long *end, *lp, alloc, copysize;
#endif
#ifdef KMEMSTATS
	register struct kmemstats *ksp = &kmemstats[type];
#endif
	kup = btokup(addr);
	size = 1 << kup->ku_indx;
	slab = slab_get_by_index(&slabcache, kup->ku_indx, type);
	if (BUCKETINDX(size) != kup->ku_indx) {
		kbp = kmembucket_search(&slabcache, slab->ksl_meta, BUCKETSIZE(kup->ku_indx), type);
	} else {
		kbp = kmembucket_search(&slabcache, slab->ksl_meta, size, type);
	}
	s = splimp();
	simple_lock(&malloc_slock);
#ifdef DIAGNOSTIC
	if (size > NBPG * CLSIZE)
		alloc = addrmask[BUCKETINDX(NBPG * CLSIZE)];
	else
		alloc = addrmask[kup->ku_indx];
	if (((u_long)addr & alloc) != 0)
		panic("free: unaligned addr 0x%x, size %d, type %s, mask %d\n", addr, size, memname[type], alloc);
#endif /* DIAGNOSTIC */
	if (size > MAXALLOCSAVE) {
#ifdef OVERLAY
		if(type & M_OVERLAY) {
			ofree(addr, ctob(kup->ku_pagecnt), (type & M_OVERLAY));
		} else {
			kfree(addr, ctob(kup->ku_pagecnt));
		}
#else
		kfree(addr, ctob(kup->ku_pagecnt));
#endif
#ifdef KMEMSTATS
		size = kup->ku_pagecnt << PGSHIFT;
		ksp->ks_memuse -= size;
		kup->ku_indx = 0;
		kup->ku_pagecnt = 0;
		if (ksp->ks_memuse + size >= ksp->ks_limit
				&& ksp->ks_memuse < ksp->ks_limit)
			wakeup((caddr_t) ksp);
		ksp->ks_inuse--;
		kbp->kb_total -= 1;
#endif
		simple_unlock(&malloc_slock);
		splx(s);
		return;
	}
	freep = (struct freelist*) addr;
#ifdef DIAGNOSTIC
	if (freep->spare0 == WEIRD_ADDR) {
		for (cp = kbp->kb_next; cp; cp = *(caddr_t *)cp) {
			if (addr != cp) {
				continue;
			}
			printf("multiply freed item 0x%x\n", addr);
			panic("free: duplicated free");
		}
	}
	copysize = size < MAX_COPY ? size : MAX_COPY;
	end = (long *)&((caddr_t)addr)[copysize];
	for (lp = (long *)addr; lp < end; lp++)
		*lp = WEIRD_ADDR;
	freep->type = type;
#endif /* DIAGNOSTIC */
#ifdef KMEMSTATS
	kup->ku_freecnt++;
	if (kup->ku_freecnt >= kbp->kb_elmpercl)
		if (kup->ku_freecnt > kbp->kb_elmpercl)
			panic("free: multiple frees");
		else if (kbp->kb_totalfree > kbp->kb_highwat)
			kbp->kb_couldfree++;
	kbp->kb_totalfree++;
	ksp->ks_memuse -= size;
	if (ksp->ks_memuse + size >= ksp->ks_limit
			&& ksp->ks_memuse < ksp->ks_limit)
		wakeup((caddr_t) ksp);
	ksp->ks_inuse--;
#endif
	if (kbp->kb_next == NULL)
		kbp->kb_next = addr;
	else
		((struct freelist*) kbp->kb_last)->next = addr;
	freep->next = NULL;
	kbp->kb_last = addr;
	if(slab->ksl_flags == SLAB_EMPTY && kbp == NULL) {
		slab->ksl_bucket = kbp;
		slab_destroy(&slabcache, size);
	}
	simple_unlock(&malloc_slock);
	splx(s);
}

/*
 * Change the size of a block of memory.
 */
void *
realloc(curaddr, newsize, type, flags)
	void *curaddr;
	unsigned long newsize;
	int type, flags;
{
	struct kmemusage *kup;
	unsigned long cursize;
	void *newaddr;
#ifdef DIAGNOSTIC
	long alloc;
#endif /* DIAGNOSTIC */

	/*
	 * realloc() with a NULL pointer is the same as malloc().
	 */
	if (curaddr == NULL)
		return (malloc(newsize, type, flags));

	/*
	 * realloc() with zero size is the same as free().
	 */
	if (newsize == 0) {
		free(curaddr, type);
		return (NULL);
	}

	/*
	 * Find out how large the old allocation was (and do some
	 * sanity checking).
	 */
	kup = btokup(curaddr);
	cursize = 1 << kup->ku_indx;

#ifdef DIAGNOSTIC
	/*
	 * Check for returns of data that do not point to the
	 * beginning of the allocation.
	 */
	if (cursize > roundup(newsize, CLBYTES))
		alloc = addrmask[BUCKETINDX( roundup(newsize, CLBYTES))];
	else
		alloc = addrmask[kup->ku_indx];
	if (((u_long) curaddr & alloc) != 0)
		panic("realloc: "
				"unaligned addr %p, size %ld, type %s, mask %ld\n", curaddr,
				cursize, type, alloc);
#endif /* DIAGNOSTIC */

	if (cursize > MAXALLOCSAVE)
		cursize = ctob(kup->ku_pagecnt);

	/*
	 * If we already actually have as much as they want, we're done.
	 */
	if (newsize <= cursize)
		return (curaddr);

	/*
	 * Can't satisfy the allocation with the existing block.
	 * Allocate a new one and copy the data.
	 */
	newaddr = malloc(newsize, type, flags);
	if (__predict_false(newaddr == NULL)) {
		/*
		 * malloc() failed, because flags included M_NOWAIT.
		 * Return NULL to indicate that failure.  The old
		 * pointer is still valid.
		 */
		return (NULL);
	}
	bcopy(curaddr, newaddr, cursize);

	/*
	 * We were successful: free the old allocation and return
	 * the new one.
	 */
	free(curaddr, type);
	return (newaddr);
}

/* Allocate a block of contiguous memory */
void *
calloc(nitems, size, type, flags)
	unsigned long size;
	int nitems, type, flags;
{
	void 			*addr;
	unsigned long 	newsize;
	int 		  	i;

	/*
	 * calloc() with zero nitems is the same as malloc()
	 */
	if (nitems == 0) {
		return (malloc(size, type, flags));
	}

	/*
	 * Find out how much contiguous space is needed
	 */
	for (i = 1; i <= nitems; i++) {
		newsize = (size * i);
	}

	/*
	 * allocate newsize of contiguous space
	 */
	addr = malloc(newsize, type, flags);

	/*
	 * clear contents
	 */
	bzero(addr, newsize);
	return (addr);
}

/* Initialize the kernel memory allocator */
void
kmeminit(void)
{
	register long indx;
	int npg;

	simple_lock_init(&malloc_slock, "malloc_slock");

#if	((MAXALLOCSAVE & (MAXALLOCSAVE - 1)) != 0)
		ERROR!_kmeminit:_MAXALLOCSAVE_not_power_of_2
#endif
#if	(MAXALLOCSAVE > MINALLOCSIZE * 32768)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_big
#endif
#if	(MAXALLOCSAVE < CLBYTES)
		ERROR!_kmeminit:_MAXALLOCSAVE_too_small
#endif
		npg = VM_KMEM_SIZE / NBPG;

		slabinit(&slabcache, kernel_map, npg);
		kmemusage = (struct kmemusage *)kmem_alloc(kernel_map, (vm_size_t)(npg * sizeof(struct kmemusage)));
		kmem_map = kmem_suballoc(kernel_map, (vm_offset_t *)&kmembase, (vm_offset_t *)&kmemlimit, (vm_size_t)(npg * NBPG), FALSE);

#ifdef OVERLAY
		omem_map = omem_suballoc(overlay_map, (vm_offset_t *)&kmembase, (vm_offset_t *)&kmemlimit, (vm_size_t)(npg * NBPG));
#endif

#ifdef KMEMSTATS
		for(indx = 0; indx < MINBUCKET + 16; indx++) {
			if (1 << indx >= CLBYTES) {
				slabbucket[indx].ksl_bucket->kb_elmpercl = 1;
			} else {
				slabbucket[indx].ksl_bucket->kb_elmpercl = CLBYTES / (1 << indx);
			}
			slabbucket[indx].ksl_bucket->kb_highwat = 5 * slabbucket[indx].ksl_bucket->kb_elmpercl;
		}
		for (indx = 0; indx < M_LAST; indx++) {
			kmemstats[indx].ks_limit = npg * NBPG * 6 / 10;
		}
#endif
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

void *
overlay_malloc(size, type, flags)
	unsigned long size;
	int type, flags;
{
	return (malloc(size, type, flags | M_OVERLAY));
}

void
overlay_free(addr, type)
	void *addr;
	int type;
{
	free(addr, type | M_OVERLAY);
}

void *
overlay_realloc(curaddr, newsize, type, flags)
	void *curaddr;
	unsigned long newsize;
	int type, flags;
{
	return (realloc(curaddr, newsize, type, flags | M_OVERLAY));
}

void *
overlay_calloc(nitems, size, type, flags)
	unsigned long size;
	int nitems, type, flags;
{
	return (calloc(nitems, size, type, flags | M_OVERLAY));
}

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
