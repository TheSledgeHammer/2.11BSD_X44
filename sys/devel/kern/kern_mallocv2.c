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

#ifdef NEWSLAB

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
	kmemslab_remove(cache, slab, size, index, mtype);
	skbp = kbp;
	slab->ksl_bucket = skbp;
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

/*
malloc(size, type, flags)
{
	register struct kmembuckets *kbp;
	caddr_t  va;

	kbp = kmembucket_alloc(&slabcache, size, BUCKETINDX(size), type);

	if (size > MAXALLOCSAVE) {
		allocsize = roundup(size, CLBYTES);
	} else {
		allocsize = 1 << indx;
	}
	npg = clrnd(btoc(allocsize));
	va = (caddr_t)vmembucket_malloc((vm_size_t)ctob(npg), flags);
}

free(addr, type)
{
	register struct kmembuckets *kbp;
	register struct kmemusage *kup;
	long size;

	kup = btokup(addr);
	size = 1 << kup->ku_indx;
	kbp = kmembucket_free(&slabcache, size, kup->ku_indx, type);

	if (size > MAXALLOCSAVE) {
		vmembucket_free(addr, ctob(kup->ku_pagecnt), type);
	}
}

void
kmeminit(void)
{
	int npg;

	npg = VM_KMEM_SIZE / NBPG;

	kmemslab_init(&slabcache, npg);
	kmembucket_init();
}
*/
#endif
