
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/kernel.h>
#include <test/mmu/malloc.h>

#include <vm/vm.h>
#include <vm/include/vm_kern.h>

struct kmemtree_entry tree_bucket_entry[MINBUCKET + 16];
struct kmembuckets bucket[MINBUCKET + 16];
struct kmemstats kmemstats[M_LAST];
struct kmemusage *kmemusage;
char *kmembase, *kmemlimit;

static int isPowerOfTwo(long n); 	/* 0 = true, 1 = false */
static int isPowerOfThree(long n);  /* 0 = true, 1 = false */
static long bucket_search(unsigned long size);
static unsigned long bucket_align(long indx);

struct asl {
	caddr_t	*next;
    long	spare0;
    short	type;
    long	spare1;
};
typedef struct asl freelist;

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

void
kmemtree_init(ktp, next, last)
    register struct kmemtree *ktp;
    caddr_t next, last;
{
    	ktp->kt_entries = 0;
    	ktp->kt_size = 0;
    	ktp->kt_next = next;
    	ktp->kt_last = last;
    	ktp->kt_left = NULL;
    	ktp->kt_middle = NULL;
    	ktp->kt_right = NULL;
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
    return 0; /* Should never reach this... (kernel panic) */
}

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
    return 0;
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
    return 1;
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
    return 1;
}

void *
malloc(size, type, flags)
    unsigned long size;
    int type, flags;
{
    	register struct kmemtree_entry *ktep;
    	register struct kmemtree *ktp;
        register struct kmembuckets *kbp;
        register struct kmemusage *kup;
        register freelist *freep1;      /* 3.2k-3 sizes */
        register freelist *freep2;      /* 2k sizes */
        long indx, npg, allocsize;
        int s;
        char va, cp, savedlist;

        indx = BUCKETINDX(size);
        kbp = &bucket[indx];
        ktep = &tree_bucket_entry[indx];
        s = splimp();

        if (kbp->kb_next == NULL) {
        	kbp->kb_last = NULL;
        	kmemtree_entry(ktep, kbp->kb_next, kbp->kb_last);
        	ktp->kt_parent = ktep;

            if (size > MAXALLOCSAVE) {
                allocsize = roundup(size, CLBYTES);
            } else {
                allocsize = 1 << indx;
            }
            npg = clrnd(btoc(allocsize));

        	goto out;
        }

        out:
        return ((void *) va);
}

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
		kmemusage = (struct kmemusage *) kmem_alloc(kernel_map, (vm_size_t)(npg * sizeof(struct kmemusage)));
		kmem_map = kmem_suballoc(kernel_map, (vm_offset_t *)&kmembase, (vm_offset_t *)&kmemlimit, (vm_size_t)(npg * NBPG), FALSE);
		rminit(coremap, (long)npg, (long)CLSIZE, "malloc map", npg);
#ifdef KMEMSTATS
		for(indx = 0; indx < MINBUCKET + 16; indx++) {
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
