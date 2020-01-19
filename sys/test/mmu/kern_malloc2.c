
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <vm/vm.h>
#include <vm/include/vm_kern.h>

struct kmemtree_entry tree_bucket_entry[MINBUCKET + 16];
static int isPowerOfTwo(long n); 	/* 0 = true, 1 = false */

struct kmemtree_entry *
kmembucket_cqinit(kbp, indx)
    struct kmembuckets *kbp;
	long indx;
{
    kbp->kb_tstree = &tree_bucket_entry[indx];
    register struct kmemtree_entry *ktep = kbp->kb_tstree;

    /* kmembucket Circular Queue setup */
    CIRCLEQ_INIT(&kbp->kb_cqlist);
    CIRCLEQ_INSERT_HEAD(&kbp->kb_cqlist, kbp, kb_tstree->kte_head);
    CIRCLEQ_INSERT_TAIL(&kbp->kb_cqlist, kbp, kb_tstree->kte_tail);

    /* kmemtree_entry setup */
    ktep->kte_head = kbp->kb_tstree->kte_head;
    ktep->kte_tail = kbp->kb_tstree->kte_tail;

    return (ktep);
}

struct kmemtree *
kmemtree_init(ktep, indx)
    struct kmemtree_entry *ktep;
    long indx;
{
    struct kmemtree *ktp = (struct kmemtree *) &ktep[indx];
    ktp->kt_parent = ktep;
    ktp->kt_left = NULL;
    ktp->kt_middle = NULL;
    ktp->kt_right = NULL;

    ktp->kt_space = FALSE;

    ktp->kt_freelist1 = (struct asl *)ktp;
    ktp->kt_freelist2 = (struct asl *)ktp;
    ktp->kt_freelist1->asl_next = NULL;
    ktp->kt_freelist1->asl_prev = NULL;
    ktp->kt_freelist2->asl_next = NULL;
    ktp->kt_freelist2->asl_prev = NULL;
    return (ktp);
}

/* Bucket List Search (kmembuckets) */
struct kmembuckets *
bucket_search_next(struct kmembuckets *kbp, caddr_t next)
{
    CIRCLEQ_FOREACH(kbp, &kbp->kb_cqlist, kb_tstree->kte_head) {
        if(CIRCLEQ_FIRST(&kbp->kb_cqlist)->kb_next == next) {
            return kbp; /* return virtual address? */
        }
    }
    return (NULL);
}

struct kmembuckets *
bucket_search_last(struct kmembuckets *kbp, caddr_t last)
{
    CIRCLEQ_FOREACH(kbp, &kbp->kb_cqlist, kb_tstree->kte_tail) {
        if(CIRCLEQ_LAST(&kbp->kb_cqlist)->kb_last == last) {
            return kbp; /* return virtual address? */
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

struct kmemtree *
trealloc_left(ktp, size)
    struct kmemtree *ktp;
    unsigned long size;
{
    long indx = BUCKETINDX(size);
    unsigned long bsize = BUCKETSIZE(indx);
    ktp->kt_bindx = indx;
    ktp->kt_bsize = bsize;

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
trealloc_middle(ktp, size)
    struct kmemtree *ktp;
    unsigned long size;
{
    long indx = BUCKETINDX(size);
    unsigned long bsize = BUCKETSIZE(indx);
    ktp->kt_bindx = indx;
    ktp->kt_bsize = bsize;

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
trealloc_right(ktp, size)
    struct kmemtree *ktp;
    unsigned long size;
{
    long indx = BUCKETINDX(size);
    unsigned long bsize = BUCKETSIZE(indx);
    ktp->kt_bindx = indx;
    ktp->kt_bsize = bsize;

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
kmemtree_find(ktp, size)
    struct kmemtree *ktp;
    unsigned long size;
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
trealloc_va(ktp, size, flags)
	struct kmemtree *ktp;
	unsigned long size;
    int flags;
{
    struct kmemtree *left, *middle, *right = NULL;

	/* determines if npg has a log base of 2 */
	unsigned long tmp = LOG2((long) size);

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

/* free memory update asl lists & bucket freelist */
void
trealloc_free()
{

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
