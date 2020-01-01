
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
push_left(size, ktp)
	unsigned long size;
	struct kmemtree *ktp;
{
	ktp->kt_left = insert(size, TYPE_11, ktp);
	return(ktp);
}

struct kmemtree *
push_middle(size, ktp)
	unsigned long size;
	struct kmemtree *ktp;
{
	ktp->kt_middle = insert(size, TYPE_01, ktp);
	return(ktp);
}

struct kmemtree *
push_right(size, ktp)
	unsigned long size;
	struct kmemtree *ktp;
{
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

/* likely to need freelist2 */
void
trealloc_left(ktp, size)
    struct kmemtree *ktp;
    unsigned long size;
{
    unsigned long bsize = 0; /* bucket size*/
    unsigned long left;
    unsigned long diff;

    if(ktp->kt_left == NULL) {
        left = SplitLeft(bsize);
        /* add other half to freelist2 */
        while(left != size && left > 0) {
            left = SplitLeft(left);
            //left_free = left_free + left;
            /* add other half to freelist2 */
            if(left <= size) {
                if(left < size) {
                    diff = size - left;
                    //add diff to freelist2
                }
                left = size;
                break;
            }
        }
    } else {
        if(isPowerOfTwo(size)) {  /* Could check this in trealloc */
            left = SplitLeft(bsize);
            while(left != size && left > 0) {
                left = SplitLeft(left);
                if (left <= size) {
                    if (left < size) {
                        diff = size - left;
                    }
                    left = size;
                    break;
                }
            }
        }
    }
}

/* likely to need freelist1 */
void
trealloc_middle(ktp, size)
    struct kmemtree *ktp;
    unsigned long size;
{
    unsigned long bsize = 0; /* bucket size*/
    unsigned long middle;
    unsigned long diff;

    if(ktp->kt_middle == NULL) {
        middle = SplitMiddle(bsize);
        /* add other half to freelist1 */
        while(middle != size && middle > 0) {
            middle = SplitMiddle(middle);
            /* add other half to freelist1 */
            if(middle <= size) {
                if(middle < size) {
                    diff = size - middle;
                    //add diff to freelist1
                }
                middle = size;
                break;
            }
        }
    } else {
        if(isPowerOfThree(size)) { /* Could check this in trealloc */
            middle = SplitMiddle(bsize);
            while(middle != size && middle > 0) {
                middle = SplitMiddle(middle);
                if (middle <= size) {
                    if (middle < size) {
                        diff = size - middle;
                    }
                    middle = size;
                    break;
                }
            }
        }
    }
}

/* likely to need freelist1 */
void
trealloc_right(ktp, size)
    struct kmemtree *ktp;
    unsigned long size;
{
    unsigned long bsize = 0; /* bucket size*/
    unsigned long right;
    unsigned long diff;

    if(ktp->kt_right == NULL) {
        right = SplitRight(bsize);
        /* add other half to freelist1 */
        while(right != size && right > 0) {
            right = SplitRight(right);
            /* add other half to freelist1 */
            if(right <= size) {
                if(right < size) {
                    diff = size - right;
                    //add diff to freelist1
                }
                right = size;
                break;
            }
        }
    } else {
        if(isPowerOfThree(size)) {  /* Could check this in trealloc */
            right = SplitRight(bsize);
            while(right != size && right > 0) {
                right = SplitRight(right);
                if (right <= size) {
                    if (right < size) {
                        diff = size - right;
                    }
                    right = size;
                    break;
                }
            }
        }
    }
}

/* Likely need to reference freelist1 & 2, Will allocate to va */
void
trealloc()
{

}
