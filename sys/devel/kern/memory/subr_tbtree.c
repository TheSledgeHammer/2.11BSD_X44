/*
 * subr_tbtree.c
 *
 *  Created on: 3 Oct 2020
 *      Author: marti
 */

/* Tertiary Buddy Tree (TBTree) */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/stdbool.h>

#include <devel/kern/memory/tbtree.h>

struct tbtree 	treebucket[MINBUCKET + 16];

static int isPowerOfTwo(long n); 	/* 0 = true, 1 = false */


/* Allocate Tertiary Buddy Tree */
struct tbtree *
tbtree_allocate(ktp)
	struct tbtree *ktp;
{
    ktp->tb_left = NULL;
    ktp->tb_middle = NULL;
    ktp->tb_right = NULL;

    ktp->tb_freelist1 = (struct asl *)ktp;
    ktp->tb_freelist2 = (struct asl *)ktp;
    ktp->tb_freelist1->asl_next = NULL;
    ktp->tb_freelist1->asl_prev = NULL;
    ktp->tb_freelist2->asl_next = NULL;
    ktp->tb_freelist2->asl_prev = NULL;

    return (ktp);
}

struct tbtree *
tbtree_insert(size, type, ktp)
    register struct tbtree *ktp;
	int type;
    u_long size;
{
    ktp->tb_size = size;
    ktp->tb_type = type;
    ktp->tb_entries++;
    return (ktp);
}

struct tbtree *
tbtree_push_left(size, dsize, ktp)
	u_long size, dsize;  /* dsize = size difference (if any) */
	struct tbtree *ktp;
{
	struct asl* free = ktp->tb_freelist1;
	if(dsize > 0) {
		free->asl_next = asl_insert(free, dsize);
	} else {
		free->asl_next = asl_insert(free, size);
	}
	ktp->tb_left = tbtree_insert(size, TYPE_11, ktp);
	return(ktp);
}

struct tbtree *
tbtree_push_middle(size, dsize, ktp)
	u_long size, dsize;   /* dsize = size difference (if any) */
	struct tbtree *ktp;
{
	struct asl* free = ktp->tb_freelist2;
	if(dsize > 0) {
		free->asl_next = asl_insert(free, dsize);
	} else {
		free->asl_next = asl_insert(free, size);
	}
	ktp->tb_middle = tbtree_insert(size, TYPE_01, ktp);
	return(ktp);
}

struct tbtree *
tbtree_push_right(size, dsize, ktp)
	u_long size, dsize;  /* dsize = size difference (if any) */
	struct tbtree *ktp;
{
	struct asl* free = ktp->tb_freelist2;
	if(dsize > 0) {
		free->asl_next = asl_insert(free, dsize);
	} else {
		free->asl_next = asl_insert(free, size);
	}
	ktp->tb_right = tbtree_insert(size, TYPE_10, ktp);
	return(ktp);
}

struct tbtree *
tbtree_left(ktp, size)
    struct tbtree *ktp;
    u_long size;
{
    long indx = BUCKETINDX(size);
    u_long bsize = BUCKETSIZE(indx);
    ktp->tb_bindx = indx;
    ktp->tb_bindx = bsize;

    u_long left;
    u_long diff = 0;

    left = SplitLeft(bsize);
    if(size < left) {
        ktp->tb_left = tbtree_push_left(left, diff, ktp);
        while(size <= left && left > 0) {
            left = SplitLeft(left);
            ktp->tb_left = tbtree_push_left(left, diff, ktp);
            if(size <= left) {
                if(size < left) {
                    diff = left - size;
                    ktp->tb_left = tbtree_push_left(left, diff, ktp);
                } else {
                    ktp->tb_left = tbtree_push_left(left, diff, ktp);
                }
                break;
            }
        }
    } else {
        if(size < bsize && size >= left) {
            diff = bsize - size;
            ktp->tb_left = tbtree_push_left(size, diff, ktp);
        } else {
            ktp->tb_left = tbtree_push_left(size, diff, ktp);
        }
    }
    return (ktp);
}

struct tbtree *
tbtree_middle(ktp, size)
    struct tbtree *ktp;
    u_long size;
{
    long indx = BUCKETINDX(size);
    u_long bsize = BUCKETSIZE(indx);
    ktp->tb_bindx = indx;
    ktp->tb_bsize = bsize;

    u_long middle;
    u_long diff = 0;

    middle = SplitMiddle(bsize);
    if(size < middle) {
        ktp->tb_middle = tbtree_push_middle(middle, diff, ktp);
        while(size <= middle && middle > 0) {
        	middle = SplitMiddle(middle);
            ktp->tb_middle = tbtree_push_middle(middle, diff, ktp);
            if(size <= middle) {
                if(size < middle) {
                    diff = middle - size;
                    ktp->tb_middle = tbtree_push_middle(middle, diff, ktp);
                } else {
                    ktp->tb_middle = tbtree_push_middle(middle, diff, ktp);
                }
                break;
            }
        }
    } else {
        if(size < bsize && size >= middle) {
            diff = bsize - size;
            ktp->tb_middle = tbtree_push_middle(size, diff, ktp);
        } else {
            ktp->tb_middle = tbtree_push_middle(size, diff, ktp);
        }
    }
    return (ktp);
}

struct tbtree *
tbtree_right(ktp, size)
    struct tbtree *ktp;
    u_long size;
{
    long indx = BUCKETINDX(size);
    u_long bsize = BUCKETSIZE(indx);
    ktp->tb_bindx = indx;
    ktp->tb_bsize = bsize;

    u_long right;
    u_long diff = 0;

    right = SplitRight(bsize);
    if(size < right) {
        ktp->tb_right = tbtree_push_right(right, diff, ktp);
        while(size <= right && right > 0) {
        	right = SplitRight(right);
            ktp->tb_right = tbtree_push_right(right, diff, ktp);
            if(size <= right) {
                if(size < right) {
                    diff = right - size;
                    ktp->tb_right = tbtree_push_right(right, diff, ktp);
                } else {
                    ktp->tb_right = tbtree_push_right(right, diff, ktp);
                }
                break;
            }
        }
    } else {
        if(size < bsize && size >= right) {
            diff = bsize - size;
            ktp->tb_right = tbtree_push_right(size, diff, ktp);
        } else {
            ktp->tb_right = tbtree_push_right(size, diff, ktp);
        }
    }
    return (ktp);
}

struct tbtree *
tbtree_find(ktp, size)
    struct tbtree *ktp;
	u_long size;
{
    if(ktp == tbtree_left(ktp, size)) {
        return ktp;
    } else if(ktp == tbtree_middle(ktp, size)) {
        return ktp;
    } else if(ktp == tbtree_right(ktp, size)) {
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
tbtree_malloc(ktp, size, objtype, flags)
	struct tbtree *ktp;
	u_long size;
    int objtype, flags;
{
    struct tbtree *left, *middle, *right = NULL;

	/* determines if npg has a log base of 2 */
	u_long tmp = LOG2((long) size);

    if(isPowerOfTwo(size)) {
        left = tbtree_left(ktp, size);
        ktp->tb_addr = (caddr_t) ovl_malloc(ovl_map, right->tb_size, objtype, flags);

	} else if(isPowerOfTwo(size - 2)) {
		middle = tbtree_middle(ktp, size);
        ktp->tb_addr = (caddr_t) ovl_malloc(ovl_map, middle->tb_size, objtype, flags);

	} else if (isPowerOfTwo(size - 3)) {
		right = tbtree_right(ktp, size);
        ktp->tb_addr = (caddr_t) ovl_malloc(ovl_map, right->tb_size, objtype, flags);

	} else {
		/* allocates size (tmp) if it has a log base of 2 */
		if(isPowerOfTwo(tmp)) {
			left = tbtree_left(ktp, size);
            ktp->tb_addr = (caddr_t) ovl_malloc(ovl_map, left->tb_size, objtype, flags);

		} else if(isPowerOfTwo(tmp - 2)) {
			middle = tbtree_middle(ktp, size);
            ktp->tb_addr = (caddr_t) ovl_malloc(ovl_map, middle->tb_size, objtype, flags);

		} else if (isPowerOfTwo(tmp - 3)) {
			right = tbtree_right(ktp, size);
            ktp->tb_addr = (caddr_t) ovl_malloc(ovl_map, right->tb_size, objtype, flags);
		}
    }
    return (ktp->tb_addr);
}

/* free block of memory from kmem_malloc: update asl, bucket freelist
 * Todo: Must match addr: No way of knowing what block of memory is being freed otherwise!
 * Sol 1 = Add addr ptr to asl &/ or kmemtree to log/ track addr.
 *  - Add to asl: Add addr ptr to functions (insert, search, remove, etc), then trealloc_free can run a check on addr
 */
void
tbtree_free(ktp, size)
	struct tbtree *ktp;
	u_long size;
{
	struct tbtree *toFind = NULL;
	struct asl *free = NULL;

	if(tbtree_find(ktp, size)->tb_size == size) {
		toFind = tbtree_find(ktp, size);
	}
	if(toFind->tb_type == TYPE_11) {
		free = ktp->tb_freelist1;
		if(free->asl_size == toFind->tb_size) {
			free = asl_remove(ktp->tb_freelist1, size);
		}
	}
	if(toFind->tb_type == TYPE_01 || toFind->tb_type == TYPE_10) {
		free = ktp->tb_freelist2;
		if(free->asl_size == toFind->tb_size) {
			free = asl_remove(ktp->tb_freelist2, size);
		}
	}
	ktp->tb_entries--;
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

caddr_t
asl_get_addr(free)
	struct asl *free;
{
	return (free->asl_addr);
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
