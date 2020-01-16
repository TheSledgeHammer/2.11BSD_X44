
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <test/mmu/malloc.h>
#include <test/mmu/log.h>

#include <vm/vm.h>
#include <vm/include/vm_kern.h>

/* Bucket List Search (kmembuckets) */
struct kmembuckets *
bucket_list(struct kmembuckets *kbp, char next)
{
    kbp->kb_last = kbp->kb_next;
    kbp->kb_next = next;
    return (kbp);
}

struct kmembuckets *
bucket_insert(struct kmembuckets *kbp, char next)
{
    kbp->kb_back = kbp->kb_front;
    kbp->kb_front = bucket_list(kbp, next);
    return (kbp);
}

struct kmembuckets *
bucket_search(struct kmembuckets *kbp, char next)
{
    if(kbp != NULL) {
        if(next == kbp->kb_next) {
            return kbp;
        }
    }
    return (NULL);
}

/* Tertiary Tree: Available Space List (asl) */
struct asl *
asl_list(struct asl *free, unsigned long size)
{
    free->asl_size = size;
    return (free);
}

struct asl *
asl_insert(struct asl *free, unsigned long size)
{
    free->asl_prev = free->asl_next;
    free->asl_next = asl_list(free, size);
    return (free);
}

struct asl *
asl_remove(struct asl *free, unsigned long size)
{
    if(size == free->asl_size) {
        int empty = 0;
        free = asl_list(free, empty);
    }
    return (free);
}

struct asl *
asl_search(struct asl *free, unsigned long size)
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
insert(size, type, ktp)
    register struct kmemtree *ktp;
	int type;
    unsigned long size;
{
    ktp->kt_size = size;
    ktp->kt_type = type;
    ktp->kt_entries++;
    return (ktp);
}

struct kmemtree *
push_left(size, dsize, ktp)
	unsigned long size, dsize;  /*dsize = difference (if any) */
	struct kmemtree *ktp;
{
	struct asl* free = ktp->kt_freelist1;
	if(dsize > 0) {
		free->asl_next = asl_insert(free, dsize);
	} else {
		free->asl_next = asl_insert(free, size);
	}
	ktp->kt_left = insert(size, TYPE_11, ktp);
	return(ktp);
}

struct kmemtree *
push_middle(size, dsize, ktp)
	unsigned long size, dsize;  /*dsize = difference (if any) */
	struct kmemtree *ktp;
{
	struct asl* free = ktp->kt_freelist2;
	if(dsize > 0) {
		free->asl_next = asl_insert(free, dsize);
	} else {
		free->asl_next = asl_insert(free, size);
	}
	ktp->kt_middle = insert(size, TYPE_01, ktp);
	return(ktp);
}

struct kmemtree *
push_right(size, dsize, ktp)
	unsigned long size, dsize; /*dsize = difference (if any) */
	struct kmemtree *ktp;
{
	struct asl* free = ktp->kt_freelist2;
	if(dsize > 0) {
		free->asl_next = asl_insert(free, dsize);
	} else {
		free->asl_next = asl_insert(free, size);
	}
	ktp->kt_right = insert(size, TYPE_10, ktp);
	return(ktp);
}

struct kmemtree_entry *
kmembucket_cqinit(kbp, indx)
    struct kmembuckets *kbp;
	long indx;
{
    kbp->kb_tstree = &tree_bucket_entry[indx];
    register struct kmemtree_entry *ktep = kbp->kb_tstree;

    /* kmembucket Circular Queue setup */
    CIRCLEQ_INIT(&kbp->kb_list);
    CIRCLEQ_INSERT_HEAD(&kbp->kb_list, kbp, kb_front);
    CIRCLEQ_INSERT_TAIL(&kbp->kb_list, kbp, kb_back);

    /* kmemtree_entry Circular Queue setup */
    CIRCLEQ_INIT(&ktep->kte_list);
    CIRCLEQ_INSERT_HEAD(&ktep->kte_list, kbp, kb_tstree->kte_head);
    CIRCLEQ_INSERT_TAIL(&ktep->kte_list, kbp, kb_tstree->kte_tail);

    return (ktep);
}

/* Replaced by above!!
void
kmemtree_entry(ktep, next, last)
    struct kmemtree_entry *ktep;
    char next, last;
{
    ktep->kte_head.kb_front = ktep->kte_head.kb_back = &ktep->kte_head;
    ktep->kte_tail.kb_front = ktep->kte_tail.kb_back = &ktep->kte_tail;
    ktep->kteb_next = next;
    ktep->kteb_last = last;
}
*/

struct kmemtree *
kmemtree_init(ktep, size)
    struct kmemtree_entry *ktep;
    unsigned long size;
{
    long indx = BUCKETINDX(size);
    struct kmemtree *ktp = (struct kmemtree *) &ktep[indx];
    ktp->kt_parent = ktep[indx];
    ktp->kt_left = NULL;
    ktp->kt_middle = NULL;
    ktp->kt_right = NULL;
    return ktp;
}

void
kmemtree_create(ktp, space)
    register struct kmemtree *ktp;
    boolean_t space;
{
    ktp->kt_space = space;
    ktp->kt_entries = 0;
    ktp->kt_size = 0;

    ktp->kt_freelist1 = (struct asl *)ktp;
    ktp->kt_freelist2 = (struct asl *)ktp;
    ktp->kt_freelist1->asl_next = NULL;
    ktp->kt_freelist1->asl_prev = NULL;
    ktp->kt_freelist2->asl_next = NULL;
    ktp->kt_freelist2->asl_prev = NULL;
}

/* Search for the bucket which best-fits the block size to be allocated */
static long
search_buckets(unsigned long size)
{
    if(size <= bucketmap[0].bucket_size) {
        return bucketmap[0].bucket_index;
    } else if(size >= bucketmap[11].bucket_size) {
        return bucketmap[11].bucket_index;
    } else {
        for(int i = 0; i < 12; i++) {
            int j = i + 1;
            if(size == bucketmap[i].bucket_size) {
                return bucketmap[i].bucket_index;
            }
            if(size > bucketmap[i].bucket_size && size < bucketmap[j].bucket_index) {
                return bucketmap[j].bucket_index;
            } else {
                return bucketmap[i].bucket_index;
            }
        }
    }
    panic("no bucket size found, returning 0");
    return (0); /* Should never reach this... */
}

/* Searches for the bucket that matches the indx value. Returning the buckets allocation size */
static unsigned long
align_to_bucket(long indx)
{
    if(indx <= 0) {
        return bucketmap[0].bucket_size;
    } else if(indx >= 11) {
        return bucketmap[11].bucket_size;
    } else {
        for(int i = 1; i < 11; i++) {
            if(indx == i) {
                return bucketmap[i].bucket_size;
            }
        }
    }
    panic("no bucket found at indx: returning 0");
    return (0); /* Should never reach this... */
}

/* Function to check if x is a power of 2 (Internal use only) */
static int
isPowerOfTwo(long n)
{
    if (n == 0)
        return 0;
    while (n != 1)
    {
        if (n%2 != 0)
            return 0;
        n = n/2;
    }
    return (1);
}

struct kmemtree *
trealloc_left(ktp, size, bsize)
    struct kmemtree *ktp;
    unsigned long size, bsize; /* bsize is bucket size*/
{
    unsigned long left;
    unsigned long diff = 0;

    left = SplitLeft(bsize);
    if(size < left) {
        ktp->kt_left = push_left(left, diff, ktp);
        while(size <= left && left > 0) {
            left = SplitLeft(left);
            ktp->kt_left = push_left(left, diff, ktp);
            if(size <= left) {
                if(size < left) {
                    diff = left - size;
                    ktp->kt_left = push_left(left, diff, ktp);
                } else {
                    ktp->kt_left = push_left(left, diff, ktp);
                }
                break;
            }
        }
    } else {
        if(size < bsize && size >= left) {
            diff = bsize - size;
            ktp->kt_left = push_left(size, diff, ktp);
        } else {
            ktp->kt_left = push_left(size, diff, ktp);
        }
    }
    return (ktp);
}

struct kmemtree *
trealloc_middle(ktp, size, bsize)
    struct kmemtree *ktp;
    unsigned long size, bsize; /* bsize is bucket size*/
{
    unsigned long middle;
    unsigned long diff = 0;

    middle = SplitMiddle(bsize);
    if(size < middle) {
        ktp->kt_middle = push_middle(middle, diff, ktp);
        while(size <= middle && middle > 0) {
        	middle = SplitMiddle(middle);
            ktp->kt_middle = push_middle(middle, diff, ktp);
            if(size <= middle) {
                if(size < middle) {
                    diff = middle - size;
                    ktp->kt_middle = push_middle(middle, diff, ktp);
                } else {
                    ktp->kt_middle = push_middle(middle, diff, ktp);
                }
                break;
            }
        }
    } else {
        if(size < bsize && size >= middle) {
            diff = bsize - size;
            ktp->kt_middle = push_middle(size, diff, ktp);
        } else {
            ktp->kt_middle = push_middle(size, diff, ktp);
        }
    }
    return (ktp);
}

struct kmemtree *
trealloc_right(ktp, size, bsize)
    struct kmemtree *ktp;
    unsigned long size, bsize; /* bsize is bucket size*/
{
    unsigned long right;
    unsigned long diff = 0;

    right = SplitRight(bsize);
    if(size < right) {
        ktp->kt_right = push_right(right, diff, ktp);
        while(size <= right && right > 0) {
        	right = SplitRight(right);
            ktp->kt_right = push_right(right, diff, ktp);
            if(size <= right) {
                if(size < right) {
                    diff = right - size;
                    ktp->kt_right = push_right(right, diff, ktp);
                } else {
                    ktp->kt_right = push_right(right, diff, ktp);
                }
                break;
            }
        }
    } else {
        if(size < bsize && size >= right) {
            diff = bsize - size;
            ktp->kt_right = push_right(size, diff, ktp);
        } else {
            ktp->kt_right = push_right(size, diff, ktp);
        }
    }
    return (ktp);
}

struct kmemtree *
kmemtree_find(ktp, size, bsize)
    struct kmemtree *ktp;
    unsigned long size, bsize;
{
    if(ktp == trealloc_left(ktp, size, bsize)) {
        return ktp;
    } else if(ktp == trealloc_middle(ktp, size, bsize)) {
        return ktp;
    } else if(ktp == trealloc_right(ktp, size, bsize)) {
        return ktp;
    } else {
    	panic("Couldn't find block of memory in tree");
        return (NULL);
    }
}

/* Assumes that the current address of kmembucket is null */
void
trealloc(ktp, size)
	struct kmemtree *ktp;
	unsigned long size;
{
	struct kmemtree *left, *middle, *right = NULL;
	long indx, npg, allocsize;
	unsigned long bsize = align_to_bucket(BUCKETINDX(size));

	if (size > MAXALLOCSAVE) {
		allocsize = roundup(size, CLBYTES);
	} else {
		allocsize = 1 << indx;
	}
	npg = clrnd(btoc(allocsize));
	unsigned long tmp = LOG(npg); /* does log(npg) fit */

	if(isPowerOfTwo(npg - 1)) {
		left = trealloc_left(ktp, npg, bsize);
	} else if(isPowerOfTwo(npg - 2)) {
		middle = trealloc_middle(ktp, npg, bsize);
	} else if (isPowerOfTwo(npg - 3)) {
		right = trealloc_right(ktp, npg, bsize);
	} else {
		if(isPowerOfTwo(tmp - 1)) {
			left = trealloc_left(ktp, npg, bsize);
		} else if(isPowerOfTwo(tmp - 2)) {
			middle = trealloc_middle(ktp, npg, bsize);
		} else if (isPowerOfTwo(tmp - 3)) {
			right = trealloc_right(ktp, npg, bsize);
		}
		/* add else find best fit (rmalloc?) */
	}
}

/* free memory update asl lists & bucket freelist */
void
trealloc_free()
{

}
