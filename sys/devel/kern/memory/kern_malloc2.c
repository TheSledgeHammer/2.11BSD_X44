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

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/queue.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include "kern/memory/malloc2.h"
#include <sys/fnv_hash.h>

struct kmembuckets 				kmembuckets[MINBUCKET + 16];

static int isPowerOfTwo(long n); 	/* 0 = true, 1 = false */

unsigned long
kmembucket_hash(buckets)
	struct kmembuckets *buckets;
{
	Fnv32_t hash1 = fnv_32_buf(&buckets, sizeof(&buckets), FNV1_32_INIT)%(MINBUCKET + 16);
	return (hash1);
}

void
kmembucket_setup(indx)
    long indx;
{
    register struct kmembuckets *kbp = &kmembuckets[kmembucket_hash()];
    CIRCLEQ_INIT(&kbp->kb_header);
    kmembucket_allocate_head(kbp);
    kmembucket_allocate_tail(kbp);
}

/* Allocate a bucket at the head of the Table */
struct kmembucket_entry *
kmembucket_allocate_head(kbp, size)
    struct kmembuckets *kbp;
    u_long size;
{
    long indx = BUCKETINDX(size);
    register struct kmembuckets *kbe = CIRCLEQ_FIRST(&kbp->kb_header);
    kbe->kb_bsize = size;
    kbe->kb_bindx = indx;
    CIRCLEQ_INSERT_HEAD(&kbp->kb_header, kbe, kb_entry);
    return (kbe);
}

/* Allocate a bucket at the Tail of the Table */
struct kmembucket_entry *
kmembucket_allocate_tail(kbp, size)
    struct kmembuckets *kbp;
    u_long size;
{
    long indx = BUCKETINDX(size);
    register struct kmembuckets *kbe = CIRCLEQ_LAST(&kbp->kb_header);
    kbe->kb_bsize = size;
    kbe->kb_bindx = indx;
    CIRCLEQ_INSERT_TAIL(&kbp->kb_header, kbe, kb_entry);
    return (kbe);
}

/* Allocate kmemtrees from Table */
struct kmemtree *
kmemtree_allocate(kbp)
	struct kmembuckets *kbp;
{
    register struct kmemtree *ktp;// = kmemtree_hash(kbp);
    ktp = kbp->kb_ztree;
    ktp->kt_left = NULL;
    ktp->kt_middle = NULL;
    ktp->kt_right = NULL;
    ktp->kt_space = kbp->kb_bspace;
    if(ktp->kt_space == FALSE) {
        ktp->kt_bindx = kbp->kb_bindx;
        ktp->kt_bsize = kbp->kb_bsize;
        ktp->kt_space = TRUE;
        kbp->kb_bspace = ktp->kt_space;
    }
    ktp->kt_freelist1 = (struct asl *)ktp;
    ktp->kt_freelist2 = (struct asl *)ktp;
    ktp->kt_freelist1->asl_next = NULL;
    ktp->kt_freelist1->asl_prev = NULL;
    ktp->kt_freelist2->asl_next = NULL;
    ktp->kt_freelist2->asl_prev = NULL;
    return (ktp);
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
        ktp->kt_va = (caddr_t) kmem_malloc(kmem_map, right->kt_size, flags);

	} else if(isPowerOfTwo(size - 2)) {
		middle = trealloc_middle(ktp, size);
        ktp->kt_va = (caddr_t) kmem_malloc(kmem_map, middle->kt_size, flags);

	} else if (isPowerOfTwo(size - 3)) {
		right = trealloc_right(ktp, size);
        ktp->kt_va = (caddr_t) kmem_malloc(kmem_map, right->kt_size, flags);

	} else {
		/* allocates size (tmp) if it has a log base of 2 */
		if(isPowerOfTwo(tmp)) {
			left = trealloc_left(ktp, size);
            ktp->kt_va = (caddr_t) kmem_malloc(kmem_map, left->kt_size, flags);

		} else if(isPowerOfTwo(tmp - 2)) {
			middle = trealloc_middle(ktp, size);
            ktp->kt_va = (caddr_t) kmem_malloc(kmem_map, middle->kt_size, flags);

		} else if (isPowerOfTwo(tmp - 3)) {
			right = trealloc_right(ktp, size);
            ktp->kt_va = (caddr_t) kmem_malloc(kmem_map, right->kt_size, flags);
		}
    }
    return (ktp->kt_va);
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
