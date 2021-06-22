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

/*
 * A Simple Slab Allocator.
 * Slabs treats each bucket in kmembucket as pool with "x" slots available.
 *
 * Slots are determined by the bucket index and the object size being allocated.
 * Slab_metadata is responsible for retaining all slot related data and some slab information
 * (i.e. slab state (partial, full or empty) and slab size (large or small))
 *
 * Slab allows for small and large objects by splitting kmembuckets into two.
 * Any objects with a bucket index less than 10 are flagged as small, while objects
 * greater than or equal to 10 are flagged as large.
 */
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/fnv_hash.h>
#include <devel/sys/malloctypes.h>
#include <devel/vm/include/vm_slab.h>

void
slab_startup(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	slab_t 			slab;
	int				error;
	u_long 			indx;
	size_t 			size;
	u_long			bsize;

    simple_lock_init(&slab_list_lock, "slab_list_lock");

    CIRCLEQ_INIT(slab_list);

    slab_count = 0;

    size = end - start;

	for(indx = 0; indx < MINBUCKET + 16; indx++) {
		bsize = BUCKETSIZE(indx);
		slab_insert(slab, bsize, M_VMSLAB, M_WAITOK);
	}
	slab->s_extent = extent_create("slab extent", start, end, slab->s_mtype, NULL, NULL, EX_WAITOK | EX_MALLOCOK);
	error = extent_alloc_region(slab->s_extent, start, bsize, EX_WAITOK | EX_MALLOCOK);
	if (error) {
		printf("slab_startup: slab allocator initialized successful");
	} else {
		panic("slab_startup: slab allocator couldn't be initialized");
		slab_destroy(slab);
	}
}

void
slab_malloc(size, alignment, boundary, mtype, flags)
	u_long 	size, alignment, boundary;
	int		mtype, flags;
{
	slab_t 	slab;
	int 	error;
	u_long 	*result;

	slab = slab_create(CIRCLEQ_FIRST(&slab_list));
	slab = slab_lookup(size, mtype);

	error = extent_alloc(slab->s_extent, size, alignment, boundary, flags, &result);
	if(error) {
		printf("slab_malloc: successful");
	} else {
		printf("slab_malloc: unsuccessful");
	}
}

void
slab_free(addr, mtype)
	void 	*addr;
	int		mtype;
{
	slab_t slab;
	size_t start;
	int   error;

	slab = slab_lookup(addr, mtype);
	if (slab) {
		start = slab->s_extent->ex_start;
		error = extent_free(slab->s_extent, start, addr, flags);
		slab_remove(addr);
	}
}

void
slab_destroy(slab)
	slab_t slab;
{
	if(slab->s_extent != NULL) {
		extent_destroy(slab->s_extent);
	}
}

void
vm_extent_init(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	struct vm_extent *ve;
	//rminit(coremap, end - start, addr, "vm_extent", end);
	ve = (struct vm_extent *) rmalloc(coremap, sizeof(struct vm_extent *));
}

void
vm_extent_create(ve, name, start, end, mtype, flags)
	struct vm_extent *ve;
	size_t start, end;
	int mtype, flags;
{
	ve->ve_extent = extent_create(name, start, end, mtype, NULL, 0, flags);
}

int
vm_extent_alloc_region(ve, start, size, flags)
	struct vm_extent *ve;
	size_t start, size;
{
	int error;
	error = extent_alloc_region(ve->ve_extent, start, size, flags);
	return (error);
}

vm_extent_subregion(ve, size, alignment, boundary, flags, result)
	struct vm_extent *ve;
{
	int error;
	error = extent_alloc(ve->ve_extent, size, alignment, boundary, flags, result);
}
