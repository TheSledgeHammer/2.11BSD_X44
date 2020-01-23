/*
 * malloc2.h
 *
 *  Created on: 23 Jan 2020
 *      Author: marti
 */

#ifndef SYS_MALLOC2_H_
#define SYS_MALLOC2_H_

#include <sys/queue.h>
/* Two-bit Type field to distinguish between different splits of sized blocks */
#define TYPE_11     11      /* split from 2k sized block */
#define TYPE_01     01      /* left split from a 3 2k-3 block */
#define TYPE_10     10      /* right split from a 3 2k-3 block  */
/*
struct kmembuckets {
    CIRCLEQ_HEAD( , kmembuckets) kb_cqlist;
    struct kmemtree_entry *kb_tstree;
};
*/
/* Tertiary Tree Entry for Each Bucket */
struct kmemtree_entry {
	CIRCLEQ_ENTRY(kmembuckets) 		kte_head;
	CIRCLEQ_ENTRY(kmembuckets) 		kte_tail;

#define kteb_next  kte_head.cqe_next->kb_next 	/* list of free blocks */
#define kteb_last  kte_tail.cqe_next->kb_last 	/* last free block */
};

/* Tertiary Tree within each bucket, for each size of memory block that is retained */
struct kmemtree {
    struct kmemtree_entry 	kt_parent;         	/* parent tree_entry */
    struct kmemtree 		*kt_left;		    /* free blocks on left child */
    struct kmemtree 		*kt_middle;		    /* free blocks on middle child */
    struct kmemtree 		*kt_right;		    /* free blocks on right child */

    struct asl      		*kt_freelist1;      /* ptr to asl for 2k blocks */
    struct asl      		*kt_freelist2;      /* ptr to asl for 3(2k-3) blocks */

    int 					kt_type;			/* Two-bit Type field for different block sizes */
    boolean_t				kt_space;			/* Determines if tertiary tree needs to be created/used */

    long 					kt_size;			/* size of memory */
    int 					kt_entries;			/* # of child nodes in tree */

    caddr_t 				kt_va;				/* virtual address */
    unsigned long 			kt_bsize;			/* bucket size this tree belongs too */
    long 					kt_bindx;			/* bucket indx this tree belongs too */
};

/* Tertiary Tree: Available Space List */
struct asl {
    struct asl 		*asl_next;
    struct asl 		*asl_prev;
    unsigned long 	asl_size;
    //Total space allocated & free: Can provide a secondary validation to kmemstats
};

#define BUCKETSIZE(indx)	(powertwo(indx))

#define SplitLeft(n)    (n / 2)
#define SplitMiddle(n)  (((SplitLeft(n)) * 2) / 3)
#define SplitRight(n)   (((SplitMiddle(n) / 2)) + (((SplitLeft(n) + SplitMiddle(n) + n) % 2) + 1))
#define SplitTotal(n)   (SplitLeft(n) + SplitMiddle(n) + SplitRight(n))

#define LOG2(n)         (n >> 2)

extern struct kmemtree_entry tree_bucket_entry[];

/* All methods below are for internal use only for kern_malloc */
extern struct kmemtree_entry *kmembucket_cqinit __P((struct kmembuckets *kbp, long indx));
extern struct kmemtree *kmemtree_init __P((struct kmemtree_entry *ktep, long indx));
extern void kmemtree_trealloc __P((struct kmemtree *ktp, unsigned long size, int flags));
extern void trealloc_free __P((struct kmemtree *ktp, unsigned long size));

/* Tertiary Tree: Available Space List */
extern struct asl *asl_list(struct asl *free, unsigned long size);
extern struct asl *asl_insert(struct asl *free, unsigned long size);
extern struct asl *asl_remove(struct asl *free, unsigned long size);
extern struct asl *asl_search(struct asl *free, unsigned long size);

#endif /* SYS_MALLOC2_H_ */

/*
 * 		if(!ktp->kt_space) {
			ktp->kt_space = TRUE;
		    ktp->kt_size = 0;
		    ktp->kt_entries = 0;
		}

		va = (caddr_t) kmemtree_trealloc(ktp, (vm_size_t)ctob(npg), !(flags & M_NOWAIT));
 */
