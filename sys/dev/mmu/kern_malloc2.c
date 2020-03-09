
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <dev/mmu/malloc2.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>

struct kmemtable table_zone[MINBUCKET + 16];
static int isPowerOfTwo(long n); 	/* 0 = true, 1 = false */

void
kmemtable_init()
{
    CIRCLEQ_INIT(&table_head);
    &table_zone = bucket;
}

void
setup_kmembuckets(indx)
    long indx;
{
    register struct kmemtable *tble = (struct kmemtable *) &table_head;
    tble->tbl_bucket[indx] = bucket[indx];
    tble->tbl_bindx = indx;
    tble->tbl_bspace = FALSE;
}

struct kmembuckets *
create_kmembucket(tble, indx)
    struct kmemtable *tble;
    long indx;
{
    return (&tble->tbl_bucket[indx]);
}

/* Allocate a bucket at the head of the Table */
void
allocate_kmembucket_head(tble, size)
    struct kmemtable *tble;
    u_long size;
{
    long indx = BUCKETINDX(size);
    register struct kmembuckets *kbp = create_kmembucket(tble, indx);
    tble->tbl_ztree = (struct kmemtrees *) &tble->tbl_bucket[indx];
    CIRCLEQ_INSERT_HEAD(&table_head, tble, tbl_entry);
}

/* Allocate a bucket at the Tail of the Table */
void
allocate_kmembucket_tail(tble, size)
    struct kmemtable *tble;
    u_long size;
{
    long indx = BUCKETINDX(size);
    register struct kmembuckets *kbp = create_kmembucket(tble, indx);
    tble->tbl_ztree = (struct kmemtrees *) &tble->tbl_bucket[indx];
    CIRCLEQ_INSERT_TAIL(&table_head, tble, tbl_entry);
}

/* Allocate kmemtrees from Table */
struct kmemtree *
allocate_kmemtree(tble)
    struct kmemtable *tble;
{
    register struct kmemtree *ktp;
    ktp = tble->tbl_ztree;
    ktp->kt_left = NULL;
    ktp->kt_middle = NULL;
    ktp->kt_right = NULL;
    ktp->kt_space = tble->tbl_bspace;
    if(ktp->kt_space == FALSE) {
        ktp->kt_bindx = tble->tbl_bindx;
        ktp->kt_bsize = tble->tbl_bsize;
        ktp->kt_space = TRUE;
        tble->tbl_bspace = ktp->kt_space;
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
struct kmembuckets *
kmembucket_search_next(tble, kbp, next)
    struct kmemtable *tble;
    struct kmembuckets *kbp;
    caddr_t next;
{
    CIRCLEQ_FOREACH(tble, &table_head, tbl_entry) {
        if(CIRCLEQ_FIRST(&table_head)->tbl_bucket == kbp) {
            if(kbp->kb_next == next) {
                return kbp;
            }
        }
    }
    return (NULL);
}

struct kmembuckets *
kmembucket_search_last(tble, kbp, last)
	struct kmemtable *tble;
	struct kmembuckets *kbp;
	caddr_t last;
{
    CIRCLEQ_FOREACH(tble, &table_head, tbl_entry) {
        if(CIRCLEQ_LAST(&table_head)->tbl_bucket == kbp) {
        	if(kbp->kb_last == last) {
        		return kbp;
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
insert(size, type, ktp)
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
	ktp->kt_left = insert(size, TYPE_11, ktp);
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
	ktp->kt_middle = insert(size, TYPE_01, ktp);
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
	ktp->kt_right = insert(size, TYPE_10, ktp);
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
trealloc_free(ktp, size)
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
