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
 * kern_malloc2.c:
 * Splits the original segregated fit allocator into two layers. The top layer is
 * the 4.4BSD segregated fit allocator; while the bottom layer is a tertiary tree buddy
 * allocator.
 * Transforming the segregated fit allocator into a bucket/ zone allocation system of sorts.
 *
 * Malloc:
 * The allocator works very similar to 4.4BSD's kern_malloc. Each block of memory instead is allocated by
 * kmemtree_trealloc (i.e. the tertiary buddy tree) before kmem_malloc.
 *
 * Free:
 * Freespace is all done by the available space list(asl) over both the tertiary tree and the buckets. Freeing up
 * space works in the same manor as allocating memory. Going through kmemtree_trealloc_free before being freed by
 * kmem_free.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kernel.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>

#include <kern/memory/malloc2.h>

struct kmembuckets bucket[MINBUCKET + 16];
struct kmemstats kmemstats[M_LAST];
struct kmemusage *kmemusage;
char *kmembase, *kmemlimit;
char *memname[] = INITKMEMNAMES;

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
#endif /* DIAGNOSTIC */

static int isPowerOfTwo(long n); 	/* 0 = true, 1 = false */

/* Allocate a block of memory */
void *
malloc(size, type, flags)
    unsigned long size;
    int type, flags;
{
    register struct kmembuckets *kbp;
    register struct kmemusage *kup;
    register struct asl *freep;
    long indx, npg, allocsize;
    int s;
    caddr_t  va, cp, savedlist;
#ifdef DIAGNOSTIC
    long *end, *lp;
    int copysize;
    char *savedtype;
#endif
#ifdef DEBUG
    extern int simplelockrecurse;
#endif
#ifdef KMEMSTATS
	register struct kmemstats *ksp = &kmemstats[type];
	if (((unsigned long)type) > M_LAST)
		panic("malloc - bogus type");
#endif
	indx = BUCKETINDX(size);
    kbp = &bucket[indx];
    kmemtree_allocate(kbp, kbp->kb_trbtree);
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
		simplelockrecurse++;
#endif
    if (kbp->kb_next == NULL) {
    	kbp->kb_last = NULL;

		if (size > MAXALLOCSAVE)
			allocsize = roundup(size, CLBYTES);
		else
			allocsize = 1 << indx;
		npg = clrnd(btoc(allocsize));

        va = (caddr_t) kmemtree_trealloc(kmem_map, (vm_size_t)ctob(npg), !(flags & (M_NOWAIT | M_CANFAIL)));

        if (va == NULL) {
        	splx(s);
#ifdef DEBUG
        	if (flags & (M_NOWAIT|M_CANFAIL))
        		simplelockrecurse--;
        		panic("malloc: out of space in kmem_map");
#endif
        	return ((void *) NULL);
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
			asl_set_addr(freep, cp);
#ifdef DIAGNOSTIC
			end = (long *)&cp[copysize];
			for (lp = (long *)cp; lp < end; lp++)
				*lp = WEIRD_ADDR;
			freep->asl_type = M_FREE;
#endif /* DIAGNOSTIC */
			if (cp <= va)
				break;
			cp -= allocsize;
			asl_set_addr(freep->asl_next,  cp);
		}

		asl_set_addr(freep->asl_next,  savedlist);
		if (kbp->kb_last == NULL)
			kbp->kb_last = (caddr_t)freep;
    }
    va = kbp->kb_next;
    kbp->kb_next = ((struct asl *)va)->asl_addr;
#ifdef DIAGNOSTIC
    freep = (struct asl *)va;
    savedtype = (unsigned)freep->asl_type < M_LAST ?
    		memname[freep->asl_type] : "???";
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
	freep->asl_type = (short)WEIRD_ADDR;
#endif
	if (((long)(&freep->asl_addr)) & 0x2)
		freep->asl_addr = (caddr_t)((WEIRD_ADDR >> 16)|(WEIRD_ADDR << 16));
	else
		freep->asl_addr = (caddr_t)WEIRD_ADDR;
	end = (long *)&va[copysize];
	for (lp = (long *)va; lp < end; lp++) {
		if (*lp == WEIRD_ADDR)
			continue;
		printf("%s %d of object 0x%x size %d %s %s (0x%x != 0x%x)\n",
				"Data modified on freelist: word", lp - (long *)va,
				va, size, "previous type", savedtype, *lp, WEIRD_ADDR);
		break;
	}
	freep->asl_spare0 = 0;
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
#ifdef DEBUG
	if (flags & M_NOWAIT)
		simplelockrecurse--;
#endif
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
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
	register struct asl *freep;
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
	kbp = &bucket[kup->ku_indx];
	kmemtree_allocate(kbp, kbp->kb_trbtree);
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
			kmemtree_trealloc_free(kmem_map, (vm_offset_t)addr, ctob(kup->ku_pagecnt));
#ifdef KMEMSTATS
			size = kup->ku_pagecnt << PGSHIFT;
			ksp->ks_memuse -= size;
			kup->ku_indx = 0;
			kup->ku_pagecnt = 0;
			if (ksp->ks_memuse + size >= ksp->ks_limit && ksp->ks_memuse < ksp->ks_limit)
				wakeup((caddr_t)ksp);
			ksp->ks_inuse--;
			kbp->kb_total -= 1;
#endif
			splx(s);
			return;
	}
	freep = (struct asl *)addr;
#ifdef DIAGNOSTIC
	if (freep->asl_spare0 == WEIRD_ADDR) {
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
	freep->asl_type = type;
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
	if (ksp->ks_memuse + size >= ksp->ks_limit && ksp->ks_memuse < ksp->ks_limit)
		wakeup((caddr_t)ksp);
	ksp->ks_inuse--;
#endif
	if (kbp->kb_next == NULL)
		kbp->kb_next = addr;
	else
		((struct asl *)kbp->kb_last)->asl_addr = addr;
	freep->asl_addr = NULL;
	kbp->kb_last = addr;
	splx(s);
}

/* Initialize the kernel memory allocator */
void
kmeminit()
{
	register long indx;
	int npg;
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
		kmemusage = (struct kmemusage *) kmem_alloc(kernel_map, (vm_size_t)(npg * sizeof(struct kmemusage)));
		kmem_map = kmem_suballoc(kernel_map, (vm_offset_t *)&kmembase, (vm_offset_t *)&kmemlimit, (vm_size_t)(npg * NBPG), FALSE);
#ifdef KMEMSTATS
		for(indx = 0; indx < MINBUCKET + 16; indx++) {
			if (1 << indx >= CLBYTES) {
				bucket[indx].kb_elmpercl = 1;
			} else {
				bucket[indx].kb_elmpercl = CLBYTES / (1 << indx);
			}
			bucket[indx].kb_highwat = 5 * bucket[indx].kb_elmpercl;
		}
		for (indx = 0; indx < M_LAST; indx++) {
			kmemstats[indx].ks_limit = npg * NBPG * 6 / 10;
		}
#endif
}

/* Tertiary Buddy Tree Allocator */

/* Allocate kmemtree */
void
kmemtree_allocate(kbp, ktp)
	struct kmembuckets *kbp;
	struct kmemtree *ktp;
{
	memset(ktp, 0, sizeof(struct kmemtree *));

    ktp->kt_left = NULL;
    ktp->kt_middle = NULL;
    ktp->kt_right = NULL;

    ktp->kt_freelist1 = (struct asl *)ktp;
    ktp->kt_freelist2 = (struct asl *)ktp;
    ktp->kt_freelist1->asl_next = NULL;
    ktp->kt_freelist1->asl_prev = NULL;
    ktp->kt_freelist2->asl_next = NULL;
    ktp->kt_freelist2->asl_prev = NULL;
}

/* Tertiary Tree (kmemtree) */
struct kmemtree *
kmemtree_insert(size, type, ktp)
    register struct kmemtree *ktp;
	int type;
    u_long size;
{
    ktp->kt_size = size;
    ktp->kt_type = type;
    ktp->kt_entries++;
    return (ktp);
}

struct kmemtree *
kmemtree_push_left(size, dsize, ktp)
	u_long size, dsize;  /* dsize = size difference (if any) */
	struct kmemtree *ktp;
{
	struct asl* free = ktp->kt_freelist1;
	if(dsize > 0) {
		free->asl_next = asl_insert(free, dsize);
	} else {
		free->asl_next = asl_insert(free, size);
	}
	ktp->kt_left = kmemtree_insert(size, TYPE_11, ktp);
	return(ktp);
}

struct kmemtree *
kmemtree_push_middle(size, dsize, ktp)
	u_long size, dsize;   /* dsize = size difference (if any) */
	struct kmemtree *ktp;
{
	struct asl* free = ktp->kt_freelist2;
	if(dsize > 0) {
		free->asl_next = asl_insert(free, dsize);
	} else {
		free->asl_next = asl_insert(free, size);
	}
	ktp->kt_middle = kmemtree_insert(size, TYPE_01, ktp);
	return(ktp);
}

struct kmemtree *
kmemtree_push_right(size, dsize, ktp)
	u_long size, dsize;  /* dsize = size difference (if any) */
	struct kmemtree *ktp;
{
	struct asl* free = ktp->kt_freelist2;
	if(dsize > 0) {
		free->asl_next = asl_insert(free, dsize);
	} else {
		free->asl_next = asl_insert(free, size);
	}
	ktp->kt_right = kmemtree_insert(size, TYPE_10, ktp);
	return(ktp);
}

struct kmemtree *
trealloc_left(ktp, size)
    struct kmemtree *ktp;
    u_long size;
{
    long indx = BUCKETINDX(size);
    u_long bsize = BUCKETSIZE(indx);
    ktp->kt_bindx = indx;
    ktp->kt_bsize = bsize;

    u_long left;
    u_long diff = 0;

    left = SplitLeft(bsize);
    if(size < left) {
        ktp->kt_left = kmemtree_push_left(left, diff, ktp);
        while(size <= left && left > 0) {
            left = SplitLeft(left);
            ktp->kt_left = kmemtree_push_left(left, diff, ktp);
            if(size <= left) {
                if(size < left) {
                    diff = left - size;
                    ktp->kt_left = kmemtree_push_left(left, diff, ktp);
                } else {
                    ktp->kt_left = kmemtree_push_left(left, diff, ktp);
                }
                break;
            }
        }
    } else {
        if(size < bsize && size >= left) {
            diff = bsize - size;
            ktp->kt_left = kmemtree_push_left(size, diff, ktp);
        } else {
            ktp->kt_left = kmemtree_push_left(size, diff, ktp);
        }
    }
    return (ktp);
}

struct kmemtree *
trealloc_middle(ktp, size)
    struct kmemtree *ktp;
    u_long size;
{
    long indx = BUCKETINDX(size);
    u_long bsize = BUCKETSIZE(indx);
    ktp->kt_bindx = indx;
    ktp->kt_bsize = bsize;

    u_long middle;
    u_long diff = 0;

    middle = SplitMiddle(bsize);
    if(size < middle) {
        ktp->kt_middle = kmemtree_push_middle(middle, diff, ktp);
        while(size <= middle && middle > 0) {
        	middle = SplitMiddle(middle);
            ktp->kt_middle = kmemtree_push_middle(middle, diff, ktp);
            if(size <= middle) {
                if(size < middle) {
                    diff = middle - size;
                    ktp->kt_middle = kmemtree_push_middle(middle, diff, ktp);
                } else {
                    ktp->kt_middle = kmemtree_push_middle(middle, diff, ktp);
                }
                break;
            }
        }
    } else {
        if(size < bsize && size >= middle) {
            diff = bsize - size;
            ktp->kt_middle = kmemtree_push_middle(size, diff, ktp);
        } else {
            ktp->kt_middle = kmemtree_push_middle(size, diff, ktp);
        }
    }
    return (ktp);
}

struct kmemtree *
trealloc_right(ktp, size)
    struct kmemtree *ktp;
    u_long size;
{
    long indx = BUCKETINDX(size);
    u_long bsize = BUCKETSIZE(indx);
    ktp->kt_bindx = indx;
    ktp->kt_bsize = bsize;

    u_long right;
    u_long diff = 0;

    right = SplitRight(bsize);
    if(size < right) {
        ktp->kt_right = kmemtree_push_right(right, diff, ktp);
        while(size <= right && right > 0) {
        	right = SplitRight(right);
            ktp->kt_right = kmemtree_push_right(right, diff, ktp);
            if(size <= right) {
                if(size < right) {
                    diff = right - size;
                    ktp->kt_right = kmemtree_push_right(right, diff, ktp);
                } else {
                    ktp->kt_right = kmemtree_push_right(right, diff, ktp);
                }
                break;
            }
        }
    } else {
        if(size < bsize && size >= right) {
            diff = bsize - size;
            ktp->kt_right = kmemtree_push_right(size, diff, ktp);
        } else {
            ktp->kt_right = kmemtree_push_right(size, diff, ktp);
        }
    }
    return (ktp);
}

struct kmemtree *
kmemtree_find(ktp, size)
    struct kmemtree *ktp;
	u_long size;
{
    if(ktp == trealloc_left(ktp, size)) {
        return ktp;
    } else if(ktp == trealloc_middle(ktp, size)) {
        return ktp;
    } else if(ktp == trealloc_right(ktp, size)) {
        return ktp;
    } else {
    	panic("Couldn't find block of memory in tree");
        return (NULL);
    }
}

/*
 * Assumes that the current address of kmembucket is null
 * If size is not a power of 2 else will check is size log base 2.
 */
caddr_t
kmemtree_trealloc(ktp, size, flags)
	struct kmemtree *ktp;
	u_long size;
    int flags;
{
    struct kmemtree *left, *middle, *right = NULL;

	/* determines if npg has a log base of 2 */
	u_long tmp = LOG2((long) size);

    if(isPowerOfTwo(size)) {
        left = trealloc_left(ktp, size);
        ktp->kt_addr = (caddr_t) kmem_malloc(kmem_map, right->kt_size, flags);

	} else if(isPowerOfTwo(size - 2)) {
		middle = trealloc_middle(ktp, size);
        ktp->kt_addr = (caddr_t) kmem_malloc(kmem_map, middle->kt_size, flags);

	} else if (isPowerOfTwo(size - 3)) {
		right = trealloc_right(ktp, size);
        ktp->kt_addr = (caddr_t) kmem_malloc(kmem_map, right->kt_size, flags);

	} else {
		/* allocates size (tmp) if it has a log base of 2 */
		if(isPowerOfTwo(tmp)) {
			left = trealloc_left(ktp, size);
            ktp->kt_addr = (caddr_t) kmem_malloc(kmem_map, left->kt_size, flags);

		} else if(isPowerOfTwo(tmp - 2)) {
			middle = trealloc_middle(ktp, size);
            ktp->kt_addr = (caddr_t) kmem_malloc(kmem_map, middle->kt_size, flags);

		} else if (isPowerOfTwo(tmp - 3)) {
			right = trealloc_right(ktp, size);
            ktp->kt_addr = (caddr_t) kmem_malloc(kmem_map, right->kt_size, flags);
		}
    }
    return (ktp->kt_addr);
}

/* free block of memory from kmem_malloc: update asl, bucket freelist
 * Todo: Must match addr: No way of knowing what block of memory is being freed otherwise!
 * Sol 1 = Add addr ptr to asl &/ or kmemtree to log/ track addr.
 *  - Add to asl: Add addr ptr to functions (insert, search, remove, etc), then trealloc_free can run a check on addr
 */
void
kmemtree_trealloc_free(ktp, size)
	struct kmemtree *ktp;
	u_long size;
{
	struct kmemtree *toFind = NULL;
	struct asl *free = NULL;

	if(kmemtree_find(ktp, size)->kt_size == size) {
		toFind = kmemtree_find(ktp, size);
	}
	if(toFind->kt_type == TYPE_11) {
		free = ktp->kt_freelist1;
		if(free->asl_size == toFind->kt_size) {
			free = asl_remove(ktp->kt_freelist1, size);
		}
	}
	if(toFind->kt_type == TYPE_01 || toFind->kt_type == TYPE_10) {
		free = ktp->kt_freelist2;
		if(free->asl_size == toFind->kt_size) {
			free = asl_remove(ktp->kt_freelist2, size);
		}
	}
	ktp->kt_entries--;

	/*
	register struct kmemusage *kup;
	kmem_free(kmem_map, (vm_offset_t)toFind, ctob(kup->ku_pagecnt));
	*/
}

/* Available Space List (ASL) in TBTree */
struct asl *
asl_list(free, size)
	struct asl *free;
	u_long size;
{
    free->asl_size = size;
    return (free);
}

struct asl *
asl_insert(free, size)
	struct asl *free;
	u_long size;
{
    free->asl_prev = free->asl_next;
    free->asl_next = asl_list(free, size);
    return (free);
}

struct asl *
asl_remove(free, size)
	struct asl *free;
	u_long size;
{
    if(size == free->asl_size) {
        int empty = 0;
        free = asl_list(free, empty);
    }
    return (free);
}

struct asl *
asl_search(free, size)
	struct asl *free;
	u_long size;
{
    if(free != NULL) {
        if(size == free->asl_size && size != 0) {
            return free;
        }
    }
    printf("empty");
    return (NULL);
}

void
asl_set_addr(free, addr)
	struct asl *free;
	caddr_t addr;
{
	free->asl_addr = addr;
}

struct asl *
asl_search_addr(free, addr)
	struct asl *free;
	caddr_t addr;
{
	if(free != NULL) {
		if(addr == free->asl_addr && addr != NULL) {
			return free;
		}
	}
    printf("empty");
    return (NULL);
}

/* Function to check if x is a power of 2 (Internal use only) */
static int
isPowerOfTwo(n)
	long n;
{
	if (n == 0)
        return 0;
    while (n != 1) {
    	if (n%2 != 0)
            return 0;
        n = n/2;
    }
    return (1);
}
