/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SYS_TBTREE_H_
#define SYS_TBTREE_H_

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

#ifdef DIAGNOSTIC

/* Available Space List */
struct asl {
	struct asl 		*asl_next;
	struct asl 		*asl_prev;
	unsigned long  	asl_size;

	/* Malloc Freelist Compat */
	long			asl_spare0;
	short			asl_type;
	long			asl_spare1;
	caddr_t 		asl_addr;
};
#else /* !DIAGNOSTIC */
struct asl {
	struct asl 		*asl_next;
	struct asl 		*asl_prev;
	unsigned long  	asl_size;

	/* Malloc Freelist Compat */
	caddr_t 		asl_addr;
};
#endif /* DIAGNOSTIC */

#define powertwo(x) 			(1 << (x))	/* returns 2 ^ x */

#define BUCKETSIZE(indx)		(powertwo(indx))

#define SplitLeft(n)    		(n / 2)
#define SplitMiddle(n)  		(((SplitLeft(n)) * 2) / 3)
#define SplitRight(n)   		(((SplitMiddle(n) / 2)) + (((SplitLeft(n) + SplitMiddle(n) + n) % 2) + 1))
#define SplitTotal(n)   		(SplitLeft(n) + SplitMiddle(n) + SplitRight(n))

#define LOG2(n)         		(n >> 2)

extern struct tbtree			*tbtree_get(struct kmembuckets *);
extern void  					tbtree_allocate(struct kmembuckets *, struct tbtree *);
extern void 					tbtree_malloc(struct tbtree *, u_long, int, int);
extern void 					tbtree_free(struct tbtree *, caddr_t, u_long);

/*  Available Space List */
extern struct asl 				*asl_list(struct asl *, u_long);
extern struct asl 				*asl_insert(struct asl *, u_long);
extern struct asl 				*asl_remove(struct asl *, u_long);
extern struct asl 				*asl_search(struct asl *, u_long);

#endif /* SYS_TBTREE_H_ */
