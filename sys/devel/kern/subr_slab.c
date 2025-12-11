/*
 * The 3-Clause BSD License:
 * Copyright (c) 2025 Martin Kelly
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
 *
 * @(#)subr_slab.c	1.0 (Martin Kelly) 12/8/25
 */
/*
 * Splitting of slab allocator from kern_malloc, with slab subroutines to
 * interface between kern_malloc and the vm memory management and vmem (future implementation).
 */
/*
 * TODO: Slab Magazines:
 * - pop/push routines
 * - slots counter on each magazine list
 * - rotating magazines when full
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/kernel.h>

/* magazine flags */
#define MAGAZINE_FULL			0x10	/* magazine full ((i.e. space left) rounds is greater than 0) */
#define MAGAZINE_EMPTY			0x12	/* magazine empty ((i.e. no space left) rounds equals 0)*/

#define MAGAZINE_CAPACITY       64      /* magazine capacity (number of rounds in each magazine) */
#define MAGAZINE_DEPOT_MAX		2 		/* number of active depots in use at once per cpu */

/* magazines */
struct kmemmagazine {
    LIST_ENTRY(kmemmagazine) ksm_list;   	/* magazine list */
	void					 *ksm_object; 	/* magazine object */
	u_long					 ksm_size;	 	/* magazine size */
	u_long					 ksm_index;   	/* magazine index */
	int 					 ksm_flags;	 	/* magazine flags */
    int 					 ksm_rounds; 	/* magazine rounds */
};

struct kmemmagazine_depot {
	LIST_HEAD(, kmemmagazine) ksmd_magazinelist_empty;	/* magazine empty list */
	LIST_HEAD(, kmemmagazine) ksmd_magazinelist_full;	/* magazine full list */
	int 					ksmd_capacity;				/* capacity per magazine */
	int 				    ksmd_magcount; 		        /* number of magazines counter */
	int 					ksmd_rounds_magazine;		/* current rounds in magazine */
};

struct kmemcache_percpu {
	struct kmemmagazine 	*kscp_magazine_current;
	struct kmemmagazine 	*kscp_magazine_previous;
	int						kscp_rounds_current;
	int						kscp_rounds_previous;
};

struct kmemcache {
	CIRCLEQ_HEAD(, kmemslabs) ksc_slablist_empty;	/* slab empty list */
	CIRCLEQ_HEAD(, kmemslabs) ksc_slablist_partial;	/* slab partial list */
	CIRCLEQ_HEAD(, kmemslabs) ksc_slablist_full;	/* slab full list */
	struct kmemslabs 		 ksc_slab;				/* slab back-pointer */
	int 					 ksc_slabcount; 		/* number of slabs counter */
	//struct kmemmagazine ksc_magazine;	/* magazine back-pointer */

	struct kmemmagazine_depot ksc_depot; 			/* magazine depot */
	int 					 ksc_depotcount; 		/* number of magazine depot counter */

};

struct kmemmagazine_depot magazinedepot[MAGAZINE_DEPOT_MAX];
struct kmemcache_percpu slabcachemp[NCPUS];

static int magazine_state(int);


/* magazine functions */
void
magazine_insert(depot, mag, object, size, index)
	struct kmemmagazine_depot *depot;
	struct kmemmagazine *mag;
	void *object;
	u_long size, index;
{
	int rval;

	rval = magazine_state(depot->ksmd_rounds_magazine);
	switch (rval) {
	case MAGAZINE_FULL:
		if (mag->ksm_rounds == 0) {
			/* full -> empty */
			depot_insert(depot, mag, object, size, index, MAGAZINE_EMPTY);
		} else {
			depot_insert(depot, mag, object, size, index, MAGAZINE_FULL);
		}
		break;
	case MAGAZINE_EMPTY:
		if (mag->ksm_rounds > 0) {
			/* empty -> full */
			depot_insert(depot, mag, object, size, index, MAGAZINE_FULL);
		} else {
			depot_insert(depot, mag, object, size, index, MAGAZINE_EMPTY);
		}
		break;
	}
}

void
magazine_remove(depot, mag, object, size, index)
	struct kmemmagazine_depot *depot;
	struct kmemmagazine *mag;
	void *object;
	u_long size, index;
{
	int rval;

	rval = magazine_state(depot->ksmd_rounds_magazine);
	switch (rval) {
	case MAGAZINE_FULL:
		if (mag->ksm_rounds == 0) {
			/* full -> empty */
			depot_remove(depot, mag, object, size, index, MAGAZINE_EMPTY);
		} else {
			depot_remove(depot, mag, object, size, index, MAGAZINE_FULL);
		}
		break;
	case MAGAZINE_EMPTY:
		if (mag->ksm_rounds > 0) {
			/* empty -> full */
			depot_remove(depot, mag, object, size, index, MAGAZINE_FULL);
		} else {
			depot_remove(depot, mag, object, size, index, MAGAZINE_EMPTY);
		}
		break;
	}
}

struct kmemmagazine *
magazine_lookup(depot, object, size, index)
	struct kmemmagazine_depot *depot;
	void *object;
	unsigned long size, index;
{
	struct kmemmagazine *mag;
	int rval;

	rval = magazine_state(depot->ksmd_rounds_magazine);
	switch (rval) {
	case MAGAZINE_FULL:
		mag = depot_lookup(depot, object, size, index, MAGAZINE_FULL);
		break;
	case MAGAZINE_EMPTY:
		mag = depot_lookup(depot, object, size, index, MAGAZINE_EMPTY);
		break;
	}
	return (depot);
}

/* depot functions */
struct kmemmagazine_depot *
depot_create(capacity)
	int capacity;
{
    struct kmemmagazine_depot *depot = NULL;

    depot = &magazinedepot[0];
    LIST_INIT(&depot->ksmd_magazinelist_empty);
    LIST_INIT(&depot->ksmd_magazinelist_full);
    depot->ksmd_capacity = capacity;
    depot->ksmd_magcount = 0;
    return (depot);
}

void
depot_insert(depot, mag, object, size, index, flags)
	struct kmemmagazine_depot *depot;
	struct kmemmagazine *mag;
	void *object;
	unsigned long size, index;
	int flags;
{
	mag->ksm_object = object;
	mag->ksm_size = size;
	mag->ksm_index = index;
	mag->ksm_flags = flags;
	simple_lock(&malloc_slock);
	switch (flags) {
	case MAGAZINE_FULL:
		LIST_INSERT_HEAD(&depot->ksmd_magazinelist_full, mag, ksm_list);
		break;
	case MAGAZINE_EMPTY:
		LIST_INSERT_HEAD(&depot->ksmd_magazinelist_empty, mag, ksm_list);
		break;
	}
	simple_unlock(&malloc_slock);
	mag->ksm_rounds--;
	/* depot update */
	depot->ksmd_rounds_magazine = mag->ksm_rounds;
	depot->ksmd_magcount++;
}

struct kmemmagazine *
depot_lookup(depot, object, size, index, flags)
	struct kmemmagazine_depot *depot;
	void *object;
	unsigned long size, index;
	int flags;
{
	struct kmemmagazine *mag;

	simple_lock(&malloc_slock);
	switch (flags) {
	case MAGAZINE_FULL:
		LIST_FOREACH(mag, &depot->ksmd_magazinelist_full, ksm_list) {
			if (mag == &magazinebucket[index]) {
				if ((mag->ksm_object == object) && (mag->ksm_size == size)
						&& (mag->ksm_index == index)) {
					simple_unlock(&malloc_slock);
					return (mag);
				}
			}
		}
		break;
	case MAGAZINE_EMPTY:
		LIST_FOREACH(mag, &depot->ksmd_magazinelist_empty, ksm_list) {
			if (mag == &magazinebucket[index]) {
				if ((mag->ksm_object == object) && (mag->ksm_size == size)
						&& (mag->ksm_index == index)) {
					simple_unlock(&malloc_slock);
					return (mag);
				}
			}
		}
		break;
	}
	simple_unlock(&malloc_slock);
	return (NULL);
}

void
depot_remove(depot, object, size, index, flags)
	struct kmemmagazine_depot *depot;
	void *object;
	unsigned long size, index;
	int flags;
{
	struct kmemmagazine *mag;

	simple_lock(&malloc_slock);
	if ((mag->ksm_object == object) && (mag->ksm_size == size)
			&& (mag->ksm_index == index)) {
		switch (flags) {
		case MAGAZINE_FULL:
			LIST_REMOVE(mag, ksm_list);
			break;
		case MAGAZINE_EMPTY:
			LIST_REMOVE(mag, ksm_list);
			break;
		}
	}
	simple_unlock(&malloc_slock);
	mag->ksm_rounds++;

	/* depot update */
	depot->ksmd_rounds_magazine = mag->ksm_rounds;
	depot->ksmd_magcount--;
}

static int
magazine_state(nrounds)
	int nrounds;
{
    int rval;

    rval = 0;
	if (nrounds > 0) {
		rval = MAGAZINE_FULL;
	} else {
		rval = MAGAZINE_EMPTY;
	}
	return (rval);
}


struct kmem_ops {
	void (*kmops_alloc)(struct kmemcache *, u_long, u_long, int);
	void (*kmops_free)(struct kmemcache *, u_long, u_long, int);
	void (*kmops_destroy)(struct kmemcache *, void *, u_long, u_long, int);
};

struct kmem_ops kmembucketops = {
	.kmops_alloc = kmembucket_alloc,

};

void
slab_alloc(cache, size, index, mtype)
	struct kmemcache *cache;
{
	register struct kmemslabs *slab;
	register struct kmembuckets *kbp;

	slab = kmemslab_create(cache, size, index, mtype);
	if (slab != NULL) {
		kbp = slab->ksl_bucket;
	}
	if (kbp != NULL) {
		kmemslab_insert(cache, slab, size, index, mtype);
	}
}

void *
slab_get()
{
	register struct kmemslabs *slab;

	slab = kmemslab_lookup(cache, size, index, mtype);
	if (slab != NULL) {
		return (slab);
	}
}
