/*
 * malloc2.h
 *
 *  Created on: 23 Jan 2020
 *      Author: marti
 */

#ifndef SYS_MALLOC2_H_
#define SYS_MALLOC2_H_
#include <sys/malloc.h>
#include <sys/queue.h>

/* Two-bit Type field to distinguish between different splits of sized blocks */
#define TYPE_11     11      /* split from 2k sized block */
#define TYPE_01     01      /* left split from a 3 2k-3 block */
#define TYPE_10     10      /* right split from a 3 2k-3 block  */
/*
struct kmembuckets {
    struct table_zones      *kb_kmemtable;
};
*/

CIRCLEQ_HEAD(table_zones, kmemtable) table_head;
struct kmemtable {
	CIRCLEQ_ENTRY(kmemtable) tbl_entry;
	struct kmembucket        *tbl_bucket;
	struct kmemtree          *tbl_ztree;
	boolean_t                tbl_bspace;		/* Bucket contains a tree: Default = False or 0 */
	unsigned long            tbl_bsize;			/* bucket size */
	long                     tbl_bindx;			/* bucket indx */
};

/* Tertiary Tree within each bucket, for each size of memory block that is retained */
struct kmemtree {
	struct table_zones      *kt_kmemtable;
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
    struct asl 				*asl_next;
    struct asl 				*asl_prev;
    unsigned long 			asl_size;
    //Total space allocated & free: Can provide a secondary validation to kmemstats
};


#define BUCKETSIZE(indx)	(powertwo(indx))

#define SplitLeft(n)    (n / 2)
#define SplitMiddle(n)  (((SplitLeft(n)) * 2) / 3)
#define SplitRight(n)   (((SplitMiddle(n) / 2)) + (((SplitLeft(n) + SplitMiddle(n) + n) % 2) + 1))
#define SplitTotal(n)   (SplitLeft(n) + SplitMiddle(n) + SplitRight(n))

#define LOG2(n)         (n >> 2)

extern struct kmemtable table_zone[];

/* All methods below are for internal use only for kern_malloc */
extern void 				kmemtable_init();
extern void 				setup_kmembuckets(long indx);
extern struct kmembuckets 	*create_kmembucket(struct kmemtable *tble, long indx);
extern void 				allocate_kmembucket_head(struct kmemtable *tble, u_long size);
extern void 				allocate_kmembucket_tail(struct kmemtable *tble, u_long size);
extern struct kmemtree 		*allocate_kmemtree(struct kmemtable *tble);
extern struct kmembuckets 	*kmembucket_search_next(struct kmemtable *tble, struct kmembucket *kbp, caddr_t next);
extern struct kmembuckets 	*kmembucket_search_last(struct kmemtable *tble, struct kmembucket *kbp, caddr_t last);

extern void 				kmemtree_trealloc(struct kmemtree *ktp, u_long size, int flags);
extern void 				trealloc_free(struct kmemtree *ktp, u_long size);

/* Tertiary Tree: Available Space List */
extern struct asl 			*asl_list(struct asl *free, u_long size);
extern struct asl 			*asl_insert(struct asl *free, u_long size);
extern struct asl 			*asl_remove(struct asl *free, u_long size);
extern struct asl 			*asl_search(struct asl *free, u_long size);

#endif /* SYS_MALLOC2_H_ */

/*
 * 		if(!ktp->kt_space) {
			ktp->kt_space = TRUE;
		    ktp->kt_size = 0;
		    ktp->kt_entries = 0;
		}

		va = (caddr_t) kmemtree_trealloc(ktp, (vm_size_t)ctob(npg), !(flags & M_NOWAIT));
 */
