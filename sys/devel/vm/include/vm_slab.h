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
    int							sm_aslots;										/* slots to be allocated */
    int                      	sm_freeslots;       							/* slots free */

    int                         sm_type;            							/* slab type: see below */
/*
  	u_long						sm_pool;
    long                        sm_min;
    long                        sm_max;
    caddr_t                     sm_addr;

    vm_segment_t                sm_segment;
    vm_page_t                   sm_page;
*/
};
typedef struct slab_metadata    *slab_metadata_t;

struct slablist;
CIRCLEQ_HEAD(slablist, slab);
struct slab {
    CIRCLEQ_ENTRY(slab)         s_list;
    CIRCLEQ_ENTRY(slab)         s_cache;

    slab_metadata_t             s_meta;
    u_long                      s_size;
    int							s_mtype;

    int							s_flags;
    int                         s_refcount;
};
typedef struct slab             *slab_t;

struct slablist			        slab_cache_list;
int			                    slab_cache_count;

struct slablist			        slab_list;
int			                    slab_count;

/* slab flags */
#define SLAB_FULL               0x01        									/* slab full */
#define SLAB_EMPTY              0x02        									/* slab empty */
#define SLAB_PARTIAL            0x04        									/* slab partially full */

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
void	slab_create();
void 	slab_malloc(u_long, int, int);
void 	slab_free(void *, int);

slab_t  slab_lookup(u_long, int);
void	slab_insert(slab_t, u_long, int, int);
void	slab_remove(u_long);
#endif /* _VM_SLAB_H_ */
