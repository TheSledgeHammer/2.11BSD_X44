
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/queue.h>
//#include <sys/malloc.h>
#include <devel/memory/malloc2.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>

struct kmembuckets p[MINBUCKET + 16];
static int isPowerOfTwo(long n); 	/* 0 = true, 1 = false */

void
kmembucket_init(kbp)
	struct kmembuckets *kbp;
{
    CIRCLEQ_INIT(&kbp->kb_header);
}

void
kmembucket_setup(indx)
    long indx;
{
    register struct kmembuckets *kbp = &p[indx];
}

/* Allocate a bucket at the head of the Table */
struct kmembucket_entry *
kmembucket_allocate_head(kbp, size)
    struct kmembuckets *kbp;
    u_long size;
{
    long indx = BUCKETINDX(size);
    register struct kmembucket_entry *kbe = CIRCLEQ_FIRST(&kbp->kb_header);
    kbe->kbe_bsize = size;
    kbe->kbe_bindx = indx;
    CIRCLEQ_INSERT_HEAD(&kbp->kb_header, kbe, kbe_entry);
    return (kbe);
}

/* Allocate a bucket at the Tail of the Table */
struct kmembucket_entry *
kmembucket_allocate_tail(kbp, size)
    struct kmembuckets *kbp;
    u_long size;
{
    long indx = BUCKETINDX(size);
    register struct kmembucket_entry *kbe = CIRCLEQ_LAST(&kbp->kb_header);
    kbe->kbe_bsize = size;
    kbe->kbe_bindx = indx;
    CIRCLEQ_INSERT_TAIL(&kbp->kb_header, kbe, kbe_entry);
    return (kbe);
}

/* Allocate kmemtrees from Table */
struct kmemtree *
kmemtree_allocate(kbe)
	struct kmembucket_entry *kbe;
{
    register struct kmemtree *ktp;
    ktp = kbe->kbe_ztree;
    ktp->kt_left = NULL;
    ktp->kt_middle = NULL;
    ktp->kt_right = NULL;
    ktp->kt_space = kbe->kbe_bspace;
    if(ktp->kt_space == FALSE) {
        ktp->kt_bindx = kbe->kbe_bindx;
        ktp->kt_bsize = kbe->kbe_bsize;
        ktp->kt_space = TRUE;
        kbe->kbe_bspace = ktp->kt_space;
    }
    ktp->kt_freelist1 = (struct asl *)ktp;
    ktp->kt_freelist2 = (struct asl *)ktp;
    ktp->kt_freelist1->asl_next = NULL;
    ktp->kt_freelist1->asl_prev = NULL;
    ktp->kt_freelist2->asl_next = NULL;
    ktp->kt_freelist2->asl_prev = NULL;
    return (ktp);
}


/* Bucket List Search (kmembuckets) */
struct kmembucket_entry *
kmembucket_search_next(kbp, kbe, next)
    struct kmembuckets *kbp;
	struct kmembucket_entry *kbe;
    caddr_t next;
{
    CIRCLEQ_FOREACH(kbe, &kbp->kb_header, kbe_entry) {
        if(CIRCLEQ_FIRST(&kbp->kb_header)->kbe_entry == kbe) {
            if(kbe->kbe_next == next) {
                return kbe;
            }
        }
    }
    return (NULL);
}

struct kmembucket_entry *
kmembucket_search_last(kbp, kbe, last)
	struct kmembuckets *kbp;
	struct kmembucket_entry *kbe;
	caddr_t last;
{
    CIRCLEQ_FOREACH(kbe, &kbp->kb_header, kbe_entry) {
        if(CIRCLEQ_LAST(&kbp->kb_header)->kbe_entry == kbe) {
        	if(kbe->kbe_last == last) {
        		return kbe;
        	}
        }
    }
    return (NULL);
}

/* Tertiary Tree: Available Space List (asl) */
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
	u_long size, dsize;  /*dsize = difference (if any) */
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
	u_long size, dsize;  /*dsize = difference (if any) */
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
	u_long size, dsize; /*dsize = difference (if any) */
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

/* Assumes that the current address of kmembucket is null */
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
