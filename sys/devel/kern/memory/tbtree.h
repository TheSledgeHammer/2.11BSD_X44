/*
 * tbtree.h
 *
 *  Created on: 3 Oct 2020
 *      Author: marti
 */

#ifndef SYS_TBTREE_H_
#define SYS_TBTREE_H_

#include <sys/malloc.h>

/* Two-bit Type field to distinguish between different splits of sized blocks */
#define TYPE_11     11      /* split from 2k sized block */
#define TYPE_01     01      /* left split from a 3 2k-3 block */
#define TYPE_10     10      /* right split from a 3 2k-3 block  */

struct tbtree {
    struct tbtree 					*tb_left;		    /* free blocks on left child */
    struct tbtree 					*tb_middle;		    /* free blocks on middle child */
    struct tbtree 					*tb_right;		    /* free blocks on right child */

    struct asl      				*tb_freelist1;      /* ptr to asl for 2k blocks */
    struct asl      				*tb_freelist2;      /* ptr to asl for 3(2k-3) blocks */

    caddr_t 						tb_addr;			/* address */

    long 							tb_size;			/* size of memory */
    int 							tb_entries;			/* # of child nodes in tree */

    int 							tb_type;			/* Two-bit Type field for different block sizes */

   unsigned long 					tb_bsize;			/* bucket size this tree belongs too */
   long 							tb_bindx;			/* bucket indx this tree belongs too */
};

struct asl {
	struct asl 						*asl_next;
	struct asl 						*asl_prev;
	unsigned long  					asl_size;
	caddr_t 						asl_addr;

	long							asl_spare0;
	long							asl_spare1;
	short							asl_type;
};

#define BUCKETSIZE(indx)		(isPowerOfTwo(indx))

#define SplitLeft(n)    		(n / 2)
#define SplitMiddle(n)  		(((SplitLeft(n)) * 2) / 3)
#define SplitRight(n)   		(((SplitMiddle(n) / 2)) + (((SplitLeft(n) + SplitMiddle(n) + n) % 2) + 1))
#define SplitTotal(n)   		(SplitLeft(n) + SplitMiddle(n) + SplitRight(n))

#define LOG2(n)         		(n >> 2)

struct tbtree 					*tbtree_allocate(struct tbtree *);
extern void 					tbtree_malloc(struct tbtree *, u_long, int, int);
extern void 					tbtree_free(struct tbtree *, u_long);


/*  Available Space List */
extern struct asl 				*asl_list(struct asl *, u_long);
extern struct asl 				*asl_insert(struct asl *, u_long);
extern struct asl 				*asl_remove(struct asl *, u_long);
extern struct asl 				*asl_search(struct asl *, u_long);

caddr_t							asl_set_addr(struct asl *, caddr_t);

extern struct tbtree treebucket[];

#endif /* SYS_TBTREE_H_ */
