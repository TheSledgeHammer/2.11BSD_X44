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

/* To setup correctly: pages may need to me remapped after segments have been initialized */

#include <sys/tree.h>
#include <sys/fnv_hash.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_segment.h>

struct seglist  	*vm_segment_buckets;
int					vm_segment_bucket_count = 0;	/* How big is array? */
int					vm_segment_hash_mask;			/* Mask for hash function */
simple_lock_data_t	segment_bucket_lock;			/* lock for all segment buckets XXX */

bool_t 				vm_segment_startup_initialized;

struct seglist		vm_segment_list;
struct seglist		vm_segment_list_active;
struct seglist		vm_segment_list_inactive;
simple_lock_data_t	vm_segment_list_lock;
simple_lock_data_t	vm_segment_list_activity_lock;

vm_segment_t 		vm_segment_array;
long				vm_segment_array_size;
long				first_segment;
long				last_segment;
vm_offset_t			first_logical_addr;
vm_offset_t			last_logical_addr;
vm_size_t			segment_mask;
int					segment_shift;

void
vm_set_segment_size()
{
	if (cnt.v_segment_size == 0)
		cnt.v_segment_size = DEFAULT_SEGMENT_SIZE;
	segment_mask = cnt.v_segment_size - 1;
	if ((segment_mask & cnt.v_segment_size) != 0)
		panic("vm_set_segment_size: segment size not a power of two");
	for (segment_shift = 0;; segment_shift++)
		if ((1 << segment_shift) == cnt.v_segment_size)
			break;
}

void
vm_segment_startup(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	register vm_segment_t		seg;
	register struct seglist		*bucket;
	vm_size_t					nsegments;
	int							i;
	vm_offset_t					la;

	simple_lock_init(&vm_segment_list_lock, "vm_segment_list_lock");
	simple_lock_init(&vm_segment_list_activity_lock, "vm_segment_list_activity_lock");

	CIRCLEQ_INIT(&vm_segment_list);
	CIRCLEQ_INIT(&vm_segment_list_active);
	CIRCLEQ_INIT(&vm_segment_list_inactive);

	if (vm_segment_bucket_count == 0) {
		vm_segment_bucket_count = 1;
		while (vm_segment_bucket_count < atos(*end - *start))
			vm_segment_bucket_count <<= 1;
	}

	vm_segment_hash_mask = vm_segment_bucket_count - 1;

	vm_segment_buckets = (struct seglist *)pmap_bootstrap_alloc(vm_segment_bucket_count * sizeof(struct seglist));
	bucket = vm_segment_buckets;

	for(i = vm_segment_bucket_count; i--;) {
		CIRCLEQ_INIT(bucket);
		bucket++;
	}

	simple_lock_init(&segment_bucket_lock, "vm_segment_bucket_lock");

	/*
	 *	Truncate the remainder of memory to our segment size.
	 */
	*end = trunc_segment(*end);

	/*
 	 *	Compute the number of segments.
	 */
	cnt.v_segment_count = nsegments = (*end - *start + sizeof(struct vm_segment)) / (SEGMENT_SIZE + sizeof(struct vm_segment));
	vm_segment_array_size = nsegments;

	/*
	 *	Record the extent of physical memory that the
	 *	virtual memory system manages. taking into account the overhead
	 *	of a segment structure per segment.
	 */
	first_segment = *start;
	first_segment += nsegments * sizeof(struct vm_segment);
	first_segment = atos(round_segment(first_segment));
	last_segment = first_segment + nsegments - 1;

	first_logical_addr = stoa(first_segment);
	last_logical_addr  = stoa(last_segment) + SEGMENT_MASK;

	seg = vm_segment_array = (vm_segment_t)pmap_bootstrap_alloc(nsegments * sizeof(struct vm_segment));

	/* Allocate and initialize pseudo-segments */
	vm_psegment_init(seg, start, end);

	/*
	 *	Allocate and clear the mem entry structures.
	 */

	la = first_logical_addr;
	while (nsegments--) {
		seg->sg_flags = 0;
		seg->sg_object = NULL;
		seg->sg_log_addr = la;
		seg++;
		la += SEGMENT_SIZE;
	}

	seg->sg_resident_page_count = 0;
	vm_segment_startup_initialized = TRUE;
}

unsigned long
vm_segment_hash(object, offset)
	vm_object_t	object;
	vm_offset_t offset;
{
	Fnv32_t hash1 = fnv_32_buf(&object, (sizeof(&object)+offset)&vm_segment_hash_mask, FNV1_32_INIT)%vm_segment_hash_mask;
	Fnv32_t hash2 = (((unsigned long)object+(unsigned long)offset)&vm_segment_hash_mask);
    return (hash1^hash2);
}

void
vm_segment_insert(segment, object, offset)
	vm_segment_t 	segment;
	vm_object_t		object;
	vm_offset_t 	offset;
{
	register struct seglist *bucket;

	if (segment->sg_flags & SEG_ALLOCATED) {
		panic("vm_segment_insert: already inserted");
	}

	segment->sg_object = object;
	segment->sg_offset = offset;

	bucket = &vm_segment_buckets[vm_segment_hash(object, offset)];

	simple_lock(&segment_bucket_lock);
	CIRCLEQ_INSERT_TAIL(bucket, segment, sg_hlist);
	simple_unlock(&segment_bucket_lock);

	CIRCLEQ_INSERT_TAIL(&object->seglist, segment, sg_list);
	segment->sg_flags |= SEG_ALLOCATED;
}

void
vm_segment_remove(segment)
	register vm_segment_t 	segment;
{
	register struct seglist *bucket;

	VM_SEGMENT_CHECK(segment);

	if (!(segment->sg_flags & SEG_ALLOCATED)) {
		return;
	}

	bucket = &vm_segment_buckets[vm_segment_hash(segment->sg_object, segment->sg_offset)];

	simple_lock(&segment_bucket_lock);
	CIRCLEQ_REMOVE(bucket, segment, sg_hlist);
	simple_unlock(&segment_bucket_lock);

	CIRCLEQ_REMOVE(&segment->sg_object->seglist, segment, sg_list);

	segment->sg_flags &= ~SEG_ALLOCATED;
}

vm_segment_t
vm_segment_lookup(object, offset)
	vm_object_t	object;
	vm_offset_t offset;
{
	register struct seglist 	*bucket;
	vm_segment_t 				segment, first, last;

	bucket = &vm_segment_buckets[vm_segment_hash(object, offset)];
	first = CIRCLEQ_FIRST(bucket);
	last = CIRCLEQ_LAST(bucket);

	simple_lock(&segment_bucket_lock);
	if(first->sg_offset != last->sg_offset) {
		if(first->sg_offset >= offset) {
			segment = vm_segment_search_next(object, offset);
			simple_unlock(&segment_bucket_lock);
			return (segment);
		}
		if(last->sg_offset >= offset) {
			segment = vm_segment_search_prev(object, offset);
			simple_unlock(&segment_bucket_lock);
			return (segment);
		}
	}
	if (first->sg_offset == last->sg_offset && first->sg_object == object) {
		simple_unlock(&segment_bucket_lock);
		return (first);
	}
	simple_unlock(&segment_bucket_lock);
	return (NULL);
}

vm_segment_t
vm_segment_search_next(object, offset)
	vm_object_t	object;
	vm_offset_t offset;
{
	register struct seglist 	*bucket;
	vm_segment_t 				segment;

	bucket = &vm_segment_buckets[vm_segment_hash(object, offset)];

	simple_lock(&segment_bucket_lock);
	for(segment = CIRCLEQ_FIRST(bucket); segment != NULL; segment = CIRCLEQ_NEXT(segment, sg_hlist)) {
		if (segment->sg_object == object && segment->sg_offset == offset) {
			simple_unlock(&segment_bucket_lock);
			return (segment);
		}
	}
	simple_unlock(&segment_bucket_lock);
	return (NULL);
}

vm_segment_t
vm_segment_search_prev(object, offset)
	vm_object_t	object;
	vm_offset_t offset;
{
	register struct seglist *bucket;
	vm_segment_t segment;

	bucket = &vm_segment_buckets[vm_segment_hash(object, offset)];

	simple_lock(&segment_bucket_lock);
	for (segment = CIRCLEQ_FIRST(bucket); segment != NULL; segment = CIRCLEQ_PREV(segment, sg_hlist)) {
		if (segment->sg_object == object && segment->sg_offset == offset) {
			simple_unlock(&segment_bucket_lock);
			return (segment);
		}
	}
	simple_unlock(&segment_bucket_lock);
	return (NULL);
}

void
vm_segment_page_insert(object, offset1, page, offset2)
	vm_object_t	object;
	vm_page_t 	page;
	vm_offset_t offset1, offset2;
{
	register vm_segment_t segment;

	segment = vm_segment_lookup(object, offset1);
	if (segment) {
		vm_page_insert(page, segment, offset2);
	}
}

vm_page_t
vm_segment_page_lookup(object, offset1, offset2)
    vm_object_t	object;
    vm_offset_t offset1, offset2;
{
    register vm_page_t 	 page;
    register vm_segment_t segment;

    segment = vm_segment_lookup(object, offset1);
    if (segment) {
        page = vm_page_lookup(segment, offset2);
        return (page);
    }
    return (NULL);
}

void
vm_segment_page_remove(object, offset1, offset2)
	vm_object_t	object;
	vm_offset_t offset1, offset2;
{
	register vm_page_t 	 page;
	register vm_segment_t segment;

    segment = vm_segment_lookup(object, offset1);
    if(segment) {
        page = vm_page_lookup(segment, offset2);
        if(page) {
        	vm_page_remove(page);
        } else {
        	panic("page doesn't exist");
        }
    } else {
    	panic("segment not found");
    }
}

vm_segment_t
vm_segment_alloc(object, offset)
	vm_object_t	object;
	vm_offset_t	offset;
{
	register vm_segment_t seg;

	simple_lock(&vm_segment_list_activity_lock);
	if(CIRCLEQ_FIRST(&vm_segment_list) == NULL) {
		simple_unlock(&vm_segment_list_activity_lock);
		return (NULL);
	}
	seg = CIRCLEQ_FIRST(&vm_segment_list);
	CIRCLEQ_REMOVE(&vm_segment_list, seg, sg_list);

	cnt.v_segment_count--;
	simple_unlock(&vm_segment_list_activity_lock);

	VM_SEGMENT_INIT(seg, object, offset)

	return (seg);
}

void
vm_segment_free(segment)
	register vm_segment_t segment;
{
	KASSERT(TAILQ_EMPTY(segment->sg_memq));

	vm_segment_remove(segment);
	if(segment->sg_flags & SEG_ACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_active, segment, sg_list);
		segment->sg_flags &= SEG_ACTIVE;
		cnt.v_segment_active_count--;
	}
	if(segment->sg_flags & SEG_INACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_inactive, segment, sg_list);
		segment->sg_flags &= SEG_INACTIVE;
		cnt.v_segment_inactive_count--;
	}
}

void
vm_segment_wire(segment)
	register vm_segment_t segment;
{
	VM_SEGMENT_CHECK(segment);

	if(segment->sg_flags & SEG_ACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_active, segment, sg_list);
		segment->sg_flags &= SEG_ACTIVE;
		cnt.v_segment_active_count--;
	}
	if(segment->sg_flags & SEG_INACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_inactive, segment, sg_list);
		segment->sg_flags &= SEG_INACTIVE;
		cnt.v_segment_inactive_count--;
	}
}

void
vm_segment_unwire(segment)
	register vm_segment_t segment;
{
	VM_SEGMENT_CHECK(segment);

	CIRCLEQ_INSERT_TAIL(&vm_segment_list_active, segment, sg_list);
	cnt.v_segment_active_count++;
	segment->sg_flags |= SEG_ACTIVE;
}

void
vm_segment_deactivate(segment)
	register vm_segment_t segment;
{
	VM_SEGMENT_CHECK(segment);

	if(segment->sg_flags & SEG_ACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_active, segment, sg_list);
		CIRCLEQ_INSERT_TAIL(&vm_segment_list_inactive, segment, sg_list);
		segment->sg_flags &= SEG_ACTIVE;
		segment->sg_flags |= SEG_INACTIVE;
		cnt.v_segment_active_count--;
		cnt.v_segment_inactive_count++;
	}
}

void
vm_segment_activate(segment)
	register vm_segment_t segment;
{
	VM_SEGMENT_CHECK(segment);

	if(segment->sg_flags & SEG_INACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_inactive, segment, sg_list);
		cnt.v_segment_inactive_count--;
		segment->sg_flags &= SEG_INACTIVE;
	}
	if(segment->sg_flags & SEG_ACTIVE) {
		panic("vm_segment_activate: already active");
	}
	CIRCLEQ_INSERT_TAIL(&vm_segment_list_active, segment, sg_list);
	segment->sg_flags |= SEG_ACTIVE;
	cnt.v_segment_active_count++;
}

/*
 *	vm_segment_zero_fill:
 *	Will perform a segment check.
 *	if pages are empty, will zero fill the segment.
 *	Otherwise will zero fill all pages in the segment.
 */
bool_t
vm_segment_zero_fill(s)
    vm_segment_t 	s;
{
	register vm_page_t 	p;

    VM_SEGMENT_CHECK(s);
    s->sg_flags &= ~SEG_CLEAN;
    if (!TAILQ_EMPTY(&s->sg_memq)) {
    	TAILQ_FOREACH(p, &s->sg_memq, listq) {
    		if (p->segment == s) {
    			return (vm_page_zero_fill(p));
    		}
    	}
    }
	pmap_zero_page(VM_SEGMENT_TO_PHYS(s));
	return (TRUE);
}

/*
 *	vm_segment_sanity_check:
 *
 *	Checks the number of pages align to the number of segments.
 *	This will always occur in the last segment. Due to how they are
 *	currently allocated & initialized.
 *
 *	Return:
 *	True: The segment contains empty pages.
 *	False: The segment is full (no empty pages found).
 */

/* check alignment of segment size to page size */
static int npages_per_segment = (SEGMENT_SIZE/ PAGE_SIZE);

bool_t
vm_segment_sanity_check(pgs, segs)
    vm_size_t pgs, segs;
{
    vm_size_t nsegment;

    /* check per single segment */
    if(segs > 1) {
        nsegment = 1;
    } else {
        nsegment = segs;
    }

    /* get the difference between a single segment & the default npages per segment (default pages: 1024) */
    vm_size_t diff = nsegment * npages_per_segment;
    vm_size_t emptyspace;
    if(pgs > npages_per_segment) {
        emptyspace = pgs - npages_per_segment;
    }

    /* sanity check above */
    int count = 0;
    while(diff < pgs) {
        diff++;
        count++;
        if(diff == pgs) {
            break;
        }
    }

    if((diff == pgs) && (count == emptyspace)) {
    	 printf("this segment contains empty pages %lu\n", emptyspace);
        return (TRUE);
    } else {
        return (FALSE);
    }
}

/*
 *	vm_segment_copy:
 *
 *	Copy one segment to another. Checks if destination segment isn't null and
 *	contains no pages. if pages do exist will run vm_page_copy on all the source segment pages instead.
 */
void
vm_segment_copy(src_seg, dest_seg)
	vm_segment_t src_seg, dest_seg;
{
	vm_page_t src_m;
	vm_page_t dest_m;

	VM_SEGMENT_CHECK(src_seg);
	VM_SEGMENT_CHECK(dest_seg);

	if (dest_seg != NULL && TAILQ_EMTPY(dest_seg->sg_memq)) {
		TAILQ_FOREACH(src_m, src_seg->sg_memq, listq) {
			if(src_m->segment == src_seg) {
				if (dest_m == NULL) {
					dest_m = vm_page_alloc(src_m->segment, src_m->offset);
				}
				vm_copy_page(src_m, dest_m);
			}
		}
	} else {
		dest_seg->sg_flags &= ~SEG_CLEAN;
		pmap_copy_page(VM_SEGMENT_TO_PHYS(src_seg), VM_SEGMENT_TO_PHYS(dest_seg));
	}
}

/* segment anon management */
vm_segment_t
vm_segment_anon_alloc(object, offset, anon)
	vm_object_t 	object;
	vm_offset_t		offset;
	vm_anon_t 		anon;
{
	vm_segment_t 	seg;

	seg = vm_segment_alloc(object, offset);

	seg->sg_object = object;
	seg->sg_offset = offset;
	seg->sg_anon = anon;
	seg->sg_flags = SEG_BUSY | SEG_CLEAN;
	if (anon) {
		anon->u.an_segment = seg;
	} else {
		if (object) {
			vm_segment_insert(seg, object, offset);
		}
	}
	return (seg);
}

void
vm_segment_anon_free(segment)
	vm_segment_t 	segment;
{
	vm_page_t anpage;

	anpage = segment->sg_anon->u.an_page;
	if (anpage != NULL) {
		return;
	} else {
		segment->sg_anon = NULL;
		return;
	}
	vm_segment_free(segment);
}
