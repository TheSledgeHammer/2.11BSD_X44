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

#ifndef _VM_SLAB_H_
#define _VM_SLAB_H_

#include <sys/malloc.h>
#include <sys/queue.h>
#include <devel/vm/include/vm.h>

/* Hold metadata on a slab */
struct slab_metadata {
    int                         sm_bslots;           							/* bucket slots available */
    int							sm_aslots;										/* bucket slots to be allocated */
    int                      	sm_fslots;       								/* bucket slots free */
    u_long 						sm_bsize; 										/* bucket size */
    u_long 						sm_bindx;	 									/* bucket index */
    u_long 						sm_min;											/* bucket min */
    u_long 						sm_max;											/* bucket max */
};
typedef struct slab_metadata    *slab_metadata_t;

struct slab {
    CIRCLEQ_ENTRY(slab)         s_list;                                         /* slab list entry */
    struct kmembuckets			*s_bucket;										/* slab kmembucket */

    slab_metadata_t             s_meta;                                         /* slab metadata */
    void						*s_metaaddr;

    u_long                      s_size;											/* slab size */
    int							s_mtype;                                        /* malloc type */
    int                         s_stype;            							/* slab type: see below */

    int							s_flags;										/* slab flags */
    int                         s_refcount;
    int                         s_usecount;                                     /* usage counter for slab caching */
};
typedef struct slab             *slab_t;

struct slab_cache {
	CIRCLEQ_HEAD(, slab)		sc_head;
	struct slab					sc_link;
};
typedef struct slab_cache      *slab_cache_t;

struct slab			        	slab_list;
int			                    slab_count;                                     /* number of items in slablist */

#define slab_lock(lock)			simple_lock(lock)
#define slab_unlock(lock)		simple_unlock(lock)

/* slab flags */
#define SLAB_FULL               0x01        									/* slab full */
#define SLAB_PARTIAL            0x02        									/* slab partially full */
#define SLAB_EMPTY              0x04        									/* slab empty */

/* slab object types */
#define SLAB_SMALL              0x08        									/* slab contains small objects */
#define SLAB_LARGE              0x16        									/* slab contains large objects */

/* slab object macros */
#define SMALL_OBJECT(s)        	(BUCKETINDX((s)) < 10)
#define LARGE_OBJECT(s)        	(BUCKETINDX((s)) >= 10)

/* slot macros */
#define BUCKET_SLOTS(bsize)     ((bsize)/BUCKETINDX(bsize))       				/* Number of slots in a bucket */
#define ALLOCATED_SLOTS(size)	(BUCKET_SLOTS(size)/BUCKETINDX(size))			/* Number slots taken by space to be allocated */
#define SLOTSFREE(bsize, size)  (BUCKET_SLOTS(bsize) - ALLOCATED_SLOTS(size)) 	/* free slots in bucket (s = size) */

/* proto types */
slab_t				slab_object(slab_cache_t, long);
slab_t  			slab_lookup(slab_cache_t, long, int);
void				slab_insert(slab_cache_t, long, int);
void				slab_remove(slab_cache_t, long);
struct kmembuckets 	*slab_kmembucket(slab_t);
struct kmembuckets 	*kmembucket_search(slab_cache_t, slab_metadata_t, long, int, int);

/*
#define	MALLOC(space, cast, size, type, flags) { 					\
	register struct kmembuckets *kbp = &slab_list[BUCKETINDX(size)].s_bucket; 	\
	long s = splimp(); 												\
	if (kbp->kb_next == NULL) { 									\
		(space) = (cast)malloc((u_long)(size), type, flags); 		\
	} else { 														\
		(space) = (cast)kbp->kb_next; 								\
		kbp->kb_next = *(caddr_t *)(space); 						\
	} 																\
	splx(s); 														\
}

#define FREE(addr, type) { 											\
	register struct kmembuckets *kbp; 								\
	register struct kmemusage *kup = btokup(addr); 					\
	long s = splimp(); 												\
	if (1 << kup->ku_indx > MAXALLOCSAVE) { 						\
		free((caddr_t)(addr), type); 								\
	} else { 														\
		kbp = &slab_list[kup->ku_indx].s_bucket;					\
		if (kbp->kb_next == NULL) 									\
			kbp->kb_next = (caddr_t)(addr); 						\
		else 														\
			*(caddr_t *)(kbp->kb_last) = (caddr_t)(addr); 			\
		*(caddr_t *)(addr) = NULL; 									\
		kbp->kb_last = (caddr_t)(addr); 							\
	} 																\
	splx(s); 														\
}
*/
#endif /* _VM_SLAB_H_ */
