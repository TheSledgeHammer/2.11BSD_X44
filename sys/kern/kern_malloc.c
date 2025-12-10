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
 * The Slab allocator is in split into two parts between this file and subr_kmemslab.c.
 * subr_kmemslab.c is the slab allocator back-end, which is closer to your typical slab
 * allocator (As described by Bonwick et al.) and kern_malloc.c which is the front-end
 * to the slab-allocator.
 *
 * subr_kmemslab.c:
 * The slab allocator consists of 3 circular lists full, partial and empty.
 * Each list will insert and/or remove from the head or tail of that list depending on the
 * object's size. Transitions between each list also occur during insertion and/or removal,
 * these transitions are dictated by the slab's meta-data.
 *
 * The Metadata keeps track of the slab's current allocation state, the bucket slot's
 * (i.e. available, allocated and free), running totals of those bucket slots, bucket size,
 * bucket index, and the slab it corresponds too.
 *
 * kern_malloc.c:
 * Uses the Segregated-fit allocator from 4.4BSD-Lite2. Which has been lightly modified
 * to work with the above slab allocator. Where kmembuckets is directed by the
 * slab allocator.
 *
 * Each slab treats each bucket in kmembuckets as a pool with "x" slots available.
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

struct kmemcache slabcache;
struct kmembuckets bucket[MINBUCKET + 16];
struct kmemslabs slabbucket[MINBUCKET + 16];
struct kmemmeta metabucket[MINBUCKET + 16];
struct kmemmagazine magazinebucket[MINBUCKET + 16];
struct kmemstats kmemstats[M_LAST];
struct kmemusage *kmemusage;
struct lock_object malloc_slock;
vm_map_t kmem_map;
char *kmembase;
char *kmemlimit;
char *memname[] = INITKMEMNAMES;

#ifdef OVERLAY
ovl_map_t omem_map;
#endif

/* [internal use only] */
vm_offset_t vmembucket_malloc(unsigned long, int);
void vmembucket_free(void *, unsigned long, int);

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

/* Allocate a block of memory */
void *
malloc(size, type, flags)
    unsigned long size;
    int type, flags;
{
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

	kbp = kmembucket_alloc(&slabcache, size, BUCKETINDX(size), type);
	s = splimp();
	simple_lock(&malloc_slock);
#ifdef KMEMSTATS
	while (ksp->ks_memuse >= ksp->ks_limit) {
		if (flags & M_NOWAIT) {
			splx(s);
			return ((void *)NULL);
		}
		if (ksp->ks_limblocks < 65535)
			ksp->ks_limblocks++;
		tsleep((caddr_t)ksp, PSWP + 2, memname[type], 0);
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
		if (size > MAXALLOCSAVE) {
			allocsize = roundup(size, CLBYTES);
		} else {
			allocsize = 1 << indx;
		}
		npg = clrnd(btoc(allocsize));
		va = (caddr_t)vmembucket_malloc(size, flags);
		if (va == NULL) {
        	splx(s);
#ifdef DEBUG
        if (flags & (M_NOWAIT | M_CANFAIL))
        	panic("malloc: out of space in kmem_map");
#endif
        	return ((void *)NULL);
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
	return ((void *)va);
}

/* Free a block of memory allocated by malloc */
void
free(addr, type)
	void *addr;
	int type;
{
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
	kbp = kmembucket_free(&slabcache, size, kup->ku_indx, type);
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
		vmembucket_free(addr, ctob(kup->ku_pagecnt), type);
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
		wakeup((caddr_t)ksp);
	ksp->ks_inuse--;
#endif
	if (kbp->kb_next == NULL)
		kbp->kb_next = addr;
	else
		((struct freelist *)kbp->kb_last)->next = addr;
	freep->next = NULL;
	kbp->kb_last = addr;
	if (kbp == NULL) {
		kmembucket_destroy(&slabcache, kbp, size, kup->ku_indx, type);
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

/* Initialize buckets */
void
kmembucket_init(void)
{
	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		/* slabs */
		slabbucket[indx].ksl_bucket = &bucket[indx];
		slabbucket[indx].ksl_meta = &metabucket[indx];
		/* meta */
		metabucket[indx].ksm_slab = &slabbucket[indx];
	}
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

	kmemslab_init(&slabcache, npg);
	kmembucket_init();
	kmemusage = (struct kmemusage *)kmem_alloc(kernel_map, (vm_size_t)(npg * sizeof(struct kmemusage)));
	kmem_map = kmem_suballoc(kernel_map, (vm_offset_t *)&kmembase, (vm_offset_t *)&kmemlimit, (vm_size_t)(npg * NBPG), FALSE);
#ifdef OVERLAY
	omem_map = omem_suballoc(overlay_map, (vm_offset_t *)&kmembase, (vm_offset_t *)&kmemlimit, (vm_size_t)(npg * NBPG));
#endif

#ifdef KMEMSTATS
	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		if (1 << indx >= CLBYTES) {
			bucket[indx].kb_elmpercl = 1;
		} else {
			bucket[indx].kb_elmpercl = CLBYTES / (1 << indx);
		}
		bucket[indx].kb_highwat = 5	* bucket[indx].kb_elmpercl;
		slabbucket[indx].ksl_bucket = &bucket[indx];
	}
	for (indx = 0; indx < M_LAST; indx++) {
		kmemstats[indx].ks_limit = npg * NBPG * 6 / 10;
	}
#endif
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

#endif
