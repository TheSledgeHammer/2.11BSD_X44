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
 *	- slab locks
 */
#ifndef _VM_SLAB_H_
#define _VM_SLAB_H_

#include <sys/malloc.h>
#include <devel/vm/include/vm.h>

/* Hold metadata on slabs */
struct slab_metadata {
    u_long                      sm_size;           		 	/* size of address to be allocated */
    int                         sm_slots;           		/* slots available */
    u_long                      sm_freeslots;       		/* slots free */

    int                         sm_type;            		/* slab type: see below */
    /*
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
    long                        s_size;

    int                         s_offset;
    int							s_flags;
    int                         s_refcount;
};
typedef struct slab             *slab_t;

struct slab_head;
CIRCLEQ_HEAD(slab_head, slab_entry);						/* XXX: Is placed in malloc.h struct kmembuckets */
struct slab_entry {
    CIRCLEQ_ENTRY(slab_entry)   se_link;
    slab_t                      se_slab;
};
typedef struct slab_entry       *slab_entry_t;

struct slablist			        slab_cache_list;
int			                    slab_cache_count;

struct slablist			        slab_list;
int			                    slab_count;

/* slots: should be managed by a freelist */
#define SLAB_FULL               0x01        				/* slab full */
#define SLAB_EMPTY              0x02        				/* slab empty */
#define SLAB_PARTIAL            0x04        				/* slab partially full */

/* slab object flags */
#define SLAB_SMALL              0x08        				/* slab contains small objects */
#define SLAB_LARGE              0x16        				/* slab contains large objects */

/* slab object macros */
#define SMALL_OBJECT(s)        (BUCKETINDX((s)) < 10)
#define LARGE_OBJECT(s)        (BUCKETINDX((s)) >= 10)

/* pool macros */
#define SLOTS(s)                ((s)/(BUCKETINDX(s)))       /* 4kb blocks */
#define SLOTSFREE(s)            (BUCKETINDX(s) - SLOTS(s))  /* free slots in bucket (s = size) */

#endif /* _VM_SLAB_H_ */
