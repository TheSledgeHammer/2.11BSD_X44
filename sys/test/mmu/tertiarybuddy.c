
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <test/mmu/malloc.h>

#include <vm/vm.h>
#include <vm/include/vm_kern.h>

static int isPowerOfTwo(long n); 	/* 0 = true, 1 = false */
static int isPowerOfThree(long n);  /* 0 = true, 1 = false */
static long bucket_search(unsigned long size);
static unsigned long bucket_align(long indx);

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
	unsigned long size, dsize;
	struct kmemtree *ktp;
{
	if(dsize > 0) {
		ktp->kt_freelist1->asl_size += dsize;
	} else {
	   ktp->kt_freelist1->asl_size += size;
	}
	ktp->kt_left = insert(size, TYPE_11, ktp);
	return(ktp);
}

struct kmemtree *
push_middle(size, dsize, ktp)
	unsigned long size, dsize;
	struct kmemtree *ktp;
{
	if(dsize > 0) {
		ktp->kt_freelist2->asl_size += dsize;
	} else {
		ktp->kt_freelist2->asl_size += size;
	}
	ktp->kt_middle = insert(size, TYPE_01, ktp);
	return(ktp);
}

struct kmemtree *
push_right(size, dsize, ktp)
	unsigned long size, dsize;
	struct kmemtree *ktp;
{
	if(dsize > 0) {
		ktp->kt_freelist2->asl_size += dsize;
	} else {
		ktp->kt_freelist2->asl_size += size;
	}
	ktp->kt_right = insert(size, TYPE_10, ktp);
	return(ktp);
}

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
bucket_search(unsigned long size)
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
bucket_align(long indx)
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

/* Function to check if x is a power of 3 (Internal use only) */
static int
isPowerOfThree(long n)
{
    if (n == 0)
        return 0;
    while (n != 1)
    {
        if (n%3 != 0)
            return 0;
        n = n/3;
    }
    return (1);
}

struct kmemtree *
trealloc_left(ktp, size, bsize)
        struct kmemtree *ktp;
        unsigned long size, bsize; /* bsize is bucket size*/
{
    //long indx;
    //unsigned long bsize = bucket_align(indx);
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
        if(size < bsize) {
            diff = bsize - size;
            ktp->kt_left = push_left(left, diff, ktp);
        } else {
            ktp->kt_left = push_left(left, diff, ktp);
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
        if(size < bsize) {
            diff = bsize - size;
            ktp->kt_middle = push_middle(middle, diff, ktp);
        } else {
            ktp->kt_middle = push_middle(middle, diff, ktp);
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
        if(size < bsize) {
            diff = bsize - size;
            ktp->kt_right = push_right(right, diff, ktp);
        } else {
            ktp->kt_right = push_right(right, diff, ktp);
        }
    }
    return (ktp);
}

void
trealloc(ktp, size)
	struct kmemtree *ktp;
	unsigned long size;
{
	struct kmemtree *left, *middle, *right = NULL;
	char va;

	if(isPowerOfTwo(size)) {

	}
	if(isPowerOfThree(size)) {

	}
}
