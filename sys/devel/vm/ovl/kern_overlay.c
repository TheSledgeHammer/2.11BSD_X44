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
 * kern_tbmalloc.c:
 * Splits kern_malloc into two layers.
 * Providing a course grain allocator(segregated fit) on top of a finer grain alloctor (tertiary buddy).
 *
 * Like malloc memory is allocated using a segregated fit algorithm, where a block of memory is allocated to
 * a bucket and then split further into 3 using a tertiary buddy algorithm (power of 2).
 * The freelist is maintained across both allocators to keep things more uniform.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>

#include <devel/sys/tbtree.h>

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
	if (((unsigned long) type) > M_LAST)
		panic("malloc - bogus type");
#endif
	indx = BUCKETINDX(size);
	kbp = &bucket[indx];
	tbtree_allocate(kbp, kbp->kb_trbtree);
	s = splimp();
#ifdef KMEMSTATS
	while (ksp->ks_memuse >= ksp->ks_limit) {
		if (flags & M_NOWAIT) {
			splx(s);
			return ((void*) NULL);
		}
		if (ksp->ks_limblocks < 65535)
			ksp->ks_limblocks++;
		tsleep((caddr_t) ksp, PSWP + 2, memname[type], 0);
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

		va = (caddr_t) tbtree_malloc(kbp->kb_trbtree, (vm_size_t) ctob(npg),
				type, !(flags & (M_NOWAIT | M_CANFAIL)));

		if (va == NULL) {
			splx(s);
#ifdef DEBUG
        	if (flags & (M_NOWAIT|M_CANFAIL))
        		simplelockrecurse--;
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
			freep = (struct asl*) cp;
#ifdef DIAGNOSTIC
			end = (long *)&cp[copysize];
			for (lp = (long *)cp; lp < end; lp++)
				*lp = WEIRD_ADDR;
			freep->asl_type = M_FREE;
#endif /* DIAGNOSTIC */
			if (cp <= va)
				break;
			cp -= allocsize;
			freep->asl_addr = cp;
		}
		freep->asl_addr = savedlist;
		if (kbp->kb_last == NULL)
			kbp->kb_last = (caddr_t) freep;
	}
	va = kbp->kb_next;
	kbp->kb_next = ((struct asl*) va)->asl_addr;
#ifdef DIAGNOSTIC
    freep = (struct asl *)va;
    savedtype = (unsigned)freep->asl_type < M_LAST ?
    		memname[freep->asl_type] : "???";
    if (kbp->kb_next && !kernacc(kbp->kb_next, sizeof(struct asl), 0)) {
    	printf("%s of object 0x%x size %d %s %s (invalid addr 0x%x)\n",
    				"Data modified on freelist: word 2.5", va, size,
    				"previous type", savedtype, kbp->kb_next);
    	kbp->kb_next = NULL;
    }
#if BYTE_ORDER == BIG_ENDIAN
	freep->asl_type = WEIRD_ADDR >> 16;
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
	out: kbp->kb_calls++;
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
	return ((void*) va);
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
	tbtree_allocate(kbp, kbp->kb_trbtree);
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
		tbtree_free(kbp->kb_trbtree, (vm_offset_t) addr, ctob(kup->ku_pagecnt),
				type);
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
		splx(s);
		return;
	}
	freep = (struct asl*) addr;
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
	if (ksp->ks_memuse + size >= ksp->ks_limit
			&& ksp->ks_memuse < ksp->ks_limit)
		wakeup((caddr_t) ksp);
	ksp->ks_inuse--;
#endif
	if (kbp->kb_next == NULL)
		kbp->kb_next = addr;
	else
		((struct asl*) kbp->kb_last)->asl_addr = addr;
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
	kmemusage = (struct kmemusage*) kmem_alloc(kernel_map,
			(vm_size_t) (npg * sizeof(struct kmemusage)));
	kmem_map = kmem_suballoc(kernel_map, (vm_offset_t*) &kmembase,
			(vm_offset_t*) &kmemlimit, (vm_size_t) (npg * NBPG), FALSE);
#ifdef KMEMSTATS
	for (indx = 0; indx < MINBUCKET + 16; indx++) {
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
