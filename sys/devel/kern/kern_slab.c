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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <vm/include/vm_kern.h>
#include <vm/include/vm.h>

#include <devel/sys/malloctypes.h>
#include <devel/sys/slab.h>

struct slab_cache       *slabCache;
struct slab 			kmemslab[MINBUCKET + 16];
simple_lock_data_t		slab_lock;
struct kmemstats 		kmemstats[M_LAST];
struct kmemusage 		*kmemusage;
char *kmembase, 		*kmemlimit;
char *memname[] = 		INITKMEMNAMES;

/* [internal use only] */
caddr_t	kmalloc(unsigned long, int);
caddr_t	omalloc(unsigned long, int);
void	kfree(void *, short);
void	ofree(void *, short, int);

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

/* slab allocation routines */

/* slab metadata */
void
slabmeta(slab, size)
	slab_t 	slab;
	u_long  size;
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
	meta->sm_fslots = SLOTSFREE(bsize, size);
	if (meta->sm_fslots < 0) {
		meta->sm_fslots = 0;
	}

	meta->sm_min = percent(meta->sm_bslots, 0); 	/* lower bucket boundary */
	meta->sm_max = percent(meta->sm_bslots, 95);  	/* upper bucket boundary */

	/* test if free bucket slots is greater than 0 and less or equal to 95% */
	if((meta->sm_fslots > meta->sm_min) && (meta->sm_fslots <= meta->sm_max)) {
		slab->s_flags |= SLAB_PARTIAL;
	/* test if free bucket slots is greater than 95% */
	} else if(meta->sm_fslots > meta->sm_max) {
		slab->s_flags |= SLAB_FULL;
	} else {
		slab->s_flags |= SLAB_EMPTY;
	}
}

slab_t
slab_object(cache, size)
    slab_cache_t 	cache;
    long    		size;
{
    register slab_t   slab;
    if(LARGE_OBJECT(size)) {
        slab = CIRCLEQ_LAST(&cache->sc_head);
    } else {
        slab = CIRCLEQ_FIRST(&cache->sc_head);
    }
    return (slab);
}

slab_t
slab_get(cache, size, mtype)
	slab_cache_t 	cache;
	long    		size;
	int 			mtype;
{
    register slab_t   slab;

    slab = &kmemslab[BUCKETINDX(size)];
    slab_lock(&slab_lock);
	for(slab = slab_object(cache, size); slab != NULL; slab = CIRCLEQ_NEXT(slab, s_list)) {
		if(slab->s_size == size && slab->s_mtype == mtype) {
			slab_unlock(&slab_lock);
			return (slab);
		}
	}
	slab_unlock(&slab_lock);
    return (NULL);
}

void
slab_create(cache, size, mtype)
	slab_cache_t cache;
    long  		size;
    int	    	mtype;
{
    register slab_t slab;
    register u_long indx;

	indx = BUCKETINDX(size);
	slab->s_size = size;
	slab->s_mtype = mtype;

    slab = &kmemslab[indx];
    cache->sc_link = slab;

    slab_lock(&slab_lock);
	if (indx < 10) {
		slab->s_stype = SLAB_SMALL;
		  CIRCLEQ_INSERT_HEAD(cache->sc_head, slab, s_list);
	} else {
		slab->s_stype = SLAB_LARGE;
		CIRCLEQ_INSERT_TAIL(cache->sc_head, slab, s_list);
	}
	slab_unlock(&slab_lock);
    slab_count++;

    /* update metadata */
	slabmeta(slab, size);
}

void
slab_destroy(cache, size)
	slab_cache_t cache;
	long  	size;
{
	slab_t slab;

	slab = &kmemslab[BUCKETINDX(size)];
	slab_lock(&slab_lock);
	CIRCLEQ_REMOVE(cache->sc_head, slab, s_list);
	slab_unlock(&slab_lock);
	slab_count--;

    /* update metadata */
	slabmeta(slab, size);
}

struct kmembuckets *
slab_kmembucket(slab)
	slab_t slab;
{
    if(slab->s_bucket != NULL) {
        return (slab->s_bucket);
    }
    return (NULL);
}

/*
 * search array for an empty bucket or partially full bucket that can fit
 * block of memory to be allocated
 */
struct kmembuckets *
kmembucket_search(cache, meta, size, mtype)
	slab_cache_t    cache;
	slab_metadata_t meta;
	long size;
	int mtype;
{
	register slab_t slab, next;
	register struct kmembuckets *kbp;
	long indx, bsize;
	int bslots, aslots, fslots;

	slab = slab_get(cache, size, mtype);

	indx = BUCKETINDX(size);
	bsize = BUCKETSIZE(indx);
	bslots = BUCKET_SLOTS(bsize);
	aslots = ALLOCATED_SLOTS(slab->s_size);
	fslots = SLOTSFREE(bsize, slab->s_size);

	switch (slab->s_flags) {
	case SLAB_FULL:
		next = CIRCLEQ_NEXT(slab, s_list);
		CIRCLEQ_FOREACH(next, &cache->sc_head, s_list) {
			if(next != slab) {
				if(next->s_flags == SLAB_PARTIAL) {
					slab = next;
					if (bsize > meta->sm_bsize && bslots > meta->sm_bslots && fslots > meta->sm_fslots) {
						kbp = slab_kmembucket(slab);
						slabmeta(slab, slab->s_size);
						return (kbp);
					}
				}
				if(next->s_flags == SLAB_EMPTY) {
					slab = next;
					kbp = slab_kmembucket(slab);
					slabmeta(slab, slab->s_size);
					return (kbp);
				}
			}
			return (NULL);
		}
		break;

	case SLAB_PARTIAL:
		if (bsize > meta->sm_bsize && bslots > meta->sm_bslots && fslots > meta->sm_fslots) {
			kbp = slab_kmembucket(slab);
			slabmeta(slab, slab->s_size);
			return (kbp);
		}
		break;

	case SLAB_EMPTY:
		kbp = slab_kmembucket(slab);
		slabmeta(slab, slab->s_size);
		return (kbp);
	}
	return (NULL);
}

/* kernel malloc routines */
void *
malloc(size, type, flags)
	long size;
	int type, flags;
{
	register struct slab *slab;
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
	if (((unsigned long)type) > M_LAST) {
		panic("malloc - bogus type");
	}
#endif
	slab_create(&slabCache, size, type);
	slab = &slabCache->sc_link;
	kbp = kmembucket_search(&slabCache, slab->s_meta, size, type);
	s = splimp();
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

		if (size > MAXALLOCSAVE) {
			allocsize = roundup(size, CLBYTES);
		} else {
			allocsize = 1 << indx;
		}

		npg = clrnd(btoc(allocsize));
		if(flags & M_OVERLAY) {
			va = omalloc((vm_size_t)ctob(npg), flags);
		} else {
			va = kmalloc((vm_size_t)ctob(npg), flags);
		}
		if (va == NULL) {
			splx(s);
#ifdef DEBUG
			if (flags & (M_NOWAIT | M_CANFAIL))
				panic("malloc: out of space in kmem_map");
#endif
			return ((void*) NULL);
		}
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
		if (kbp->kb_last == NULL) {
			kbp->kb_last = (caddr_t)freep;
		}
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
	splx(s);
	if ((flags & M_ZERO) && va != NULL)
		memset(va, 0, size);
	return ((void *) va);
}

/* Free a block of memory allocated by malloc */
void
free(addr, type)
	void *addr;
	int type;
{
	register struct slab *slab;
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
	register struct freelist *freep;
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
	slab = slab_get(&slabCache, size, type);
	kbp = kmembucket_search(&slabCache, slab->s_meta, size, type);
	s = splimp();
#ifdef DIAGNOSTIC
	if (size > NBPG * CLSIZE)
		alloc = addrmask[BUCKETINDX(NBPG * CLSIZE)];
	else
		alloc = addrmask[kup->ku_indx];
	if (((u_long)addr & alloc) != 0)
		panic("free: unaligned addr 0x%x, size %d, type %s, mask %d\n", addr, size, memname[type], alloc);
#endif /* DIAGNOSTIC */
	if (size > MAXALLOCSAVE) {
		if(type & M_OVERLAY) {
			ofree(addr, ctob(kup->ku_pagecnt), (type & M_OVERLAY));
		} else {
			kfree(addr, ctob(kup->ku_pagecnt));
		}
#ifdef KMEMSTATS
		size = kup->ku_pagecnt << PGSHIFT;
		ksp->ks_memuse -= size;
		kup->ku_indx = 0;
		kup->ku_pagecnt = 0;
		if (ksp->ks_memuse + size >= ksp->ks_limit && ksp->ks_memuse < ksp->ks_limit)
			wakeup((caddr_t) ksp);
		ksp->ks_inuse--;
		kbp->kb_total -= 1;
#endif
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
	if(slab->s_flags == SLAB_EMPTY && kbp == NULL) {
		slab->s_bucket = kbp;
		slab_destroy(&slabCache, size);
	}
	splx(s);
}

/* Initialize the kernel memory allocator */
void
kmeminit()
{
	register long indx;
	int npg;

	simple_lock_init(&slab_lock, "slab_lock");

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
	slabCache = (struct slab_cache*) kmem_alloc(kernel_map, (vm_size_t)(npg * sizeof(struct slab_cache)));
	kmemusage = (struct kmemusage*) kmem_alloc(kernel_map,
			(vm_size_t)(npg * sizeof(struct kmemusage)));
	kmem_map = kmem_suballoc(kernel_map, (vm_offset_t*) &kmembase,
			(vm_offset_t*) &kmemlimit, (vm_size_t)(npg * NBPG), FALSE);

	CIRCLEQ_INIT(&slabCache->sc_head);
	slab_count = 0;

#ifdef KMEMSTATS
	for (indx = 0; indx < MINBUCKET + 16; indx++) {
		if (1 << indx >= CLBYTES) {
			&kmemslab[indx].s_bucket->kb_elmpercl = 1;
		} else {
			&kmemslab[indx].s_bucket->kb_elmpercl = CLBYTES / (1 << indx);
		}
		&kmemslab[indx].s_bucket->kb_highwat = 5
				* &slab_list[indx].s_bucket->kb_elmpercl;
	}
	for (indx = 0; indx < M_LAST; indx++) {
		kmemstats[indx].ks_limit = npg * NBPG * 6 / 10;
	}
#endif
}

void
slabcache_create(cache, size)
	struct slab_cache *cache;
	long 				size;
{
	cache = (struct slab_cache *) kmem_alloc(kernel_map, (vm_size_t)(size * sizeof(struct slab_cache)));
}

/* allocate memory to vm [internal use only] */
caddr_t
kmalloc(size, flags)
	unsigned long size;
	int flags;
{
	caddr_t  va;

	va = (caddr_t) kmem_malloc(kmem_map, size, !(flags & (M_NOWAIT | M_CANFAIL)));

	return (va);
}

/* allocate memory to ovl [internal use only] */
caddr_t
omalloc(size, flags)
	unsigned long size;
	int flags;
{
	caddr_t  va;

	if(!(flags & (M_NOWAIT | M_CANFAIL))) {
		va = (caddr_t)omem_malloc(omem_map, size, (flags & M_OVERLAY));
	}
	return (va);
}


/* free memory from vm [internal use only] */
void
kfree(addr, size)
	void *addr;
	short size;
{
	kmem_free(kmem_map, (vm_offset_t) addr, size);
}

/* free memory from ovl [internal use only] */
void
ofree(addr, size, type)
	void *addr;
	short size;
	int type;
{
	if(type & M_OVERLAY) {
		omem_free(omem_map, (vm_offset_t) addr, size);
	}
}
