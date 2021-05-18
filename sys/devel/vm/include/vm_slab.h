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

/* TODO:
 *	- slab caching
 *	- use of vm_pages and vm_segments
 *	- provide ability to use extents?
 *	- size: may need to change to a void *addr/size
 */
#ifndef _VM_SLAB_H_
#define _VM_SLAB_H_

#include <sys/malloc.h>
#include <devel/vm/include/vm.h>

/* Hold metadata on a slab */
struct slab_metadata {
    int                         sm_bslots;           							/* bucket slots available */
    int							sm_aslots;										/* bucket slots to be allocated */
    int                      	sm_fslots;       								/* bucket slots free */

    u_long 						sm_bsize; 										/* bucket size */
    u_long 						sm_bindx;	 									/* bucket index */
};
typedef struct slab_metadata    *slab_metadata_t;

struct slablist;
CIRCLEQ_HEAD(slablist, slab);
struct slab {
    CIRCLEQ_ENTRY(slab)         s_list;                                         /* slab list entry */
    CIRCLEQ_ENTRY(slab)         s_cache;                                        /* cache list entry */

    struct kmembuckets			*s_bucket;

    slab_metadata_t             s_meta;                                         /* slab metadata */
    u_long                      s_size;											/* slab size */
    int							s_mtype;                                        /* malloc type */
    int                         s_stype;            							/* slab type: see below */

    int							s_flags;										/* slab flags */
    int                         s_refcount;
    int                         s_usecount;                                     /* usage counter for slab caching */

    struct extent				*s_extent;										/* slab extent */

    /* alt list: see end of vm_slab.c */
    struct slab					*s_next;
    struct slab					*s_prev;
};
typedef struct slab             *slab_t;

struct slablist			        slab_cache_list;
int			                    slab_cache_count;                               /* number of items in cache */

struct slablist			        slab_list;
int			                    slab_count;                                     /* number of items in slablist */

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
#define BUCKET_SLOTS(bsize)     ((bsize)/(BUCKETINDX(bsize)))       			/* Number of slots in a bucket */
#define ALLOCATED_SLOTS(size)	(BUCKET_SLOTS(size))							/* Number slots taken by space to be allocated */
#define SLOTSFREE(bsize, size)  (BUCKET_SLOTS(bsize) - ALLOCATED_SLOTS(size)) 	/* free slots in bucket (s = size) */

/* proto types */
void	slab_create(slab_t);
void 	slab_malloc(u_long, int, int);
void 	slab_free(void *, int);

slab_t  slab_small_lookup(u_long, int);
slab_t  slab_large_lookup(u_long, int);
void	slab_insert(slab_t, u_long, int, int);
void	slab_remove(u_long);
#endif /* _VM_SLAB_H_ */
