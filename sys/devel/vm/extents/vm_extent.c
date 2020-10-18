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

#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <vm_extent.h>

struct extent *
vm_extent_create(ex, name, start, end, mtype, storage, storagesize, flags)
	struct extent *ex;
	char *name;
	vm_offset_t start, end;
	int mtype;
	caddr_t storage;
	size_t storagesize;
	int flags;
{
	return (extent_create(name, start, end, mtype, storage, storagesize, flags));
}

int
vm_extent_alloc_region(ex, start, size, flags)
	struct extent *ex;
	vm_offset_t start, size;
	int flags;
{
	return (extent_alloc_region(ex, start, size, flags));
}

int
vm_extent_alloc_subregion(ex, substart, subend, size, alignment, boundary, flags, result)
	struct extent *ex;
	vm_offset_t substart, subend;
	vm_size_t size;
	u_long alignment, boundary;
	int flags;
	u_long *result;
{
	return (extent_alloc_subregion(ex, substart, subend, size, alignment, boundary, flags, result));
}

int
vm_extent_free(ex, start, size, flags)
	struct extent *ex;
	vm_offset_t start, size;
	int flags;
{
	return (extent_free(ex, start, size, flags));
}

void
vm_extent_destroy(ex)
	struct extent *ex;
{
	extent_destroy(ex);
}

void
vm_extent_print(ex)
	struct extent *ex;
{
	extent_print(ex);
}



struct extent *slab_extent;
long slab_ex_storage[MINBUCKET + 16];
struct slablist vm_slab_hashtable[MINBUCKET + 16];

void
slab_extent_create(start, end, mtype, storage, storagesize, flags)
	vm_offset_t start, end;
	int mtype;
	caddr_t storage;
	size_t storagesize;
	int flags;
{
	register long indx;
	for(indx = 0; indx < MINBUCKET +16; indx++) {
		CIRCLEQ_INIT(&vm_slab_hashtable[indx]);
	}
	&slab_extent = extent_create("vm_slab_extent", start, end, mtype, storage, storagesize, flags);

}

void
slab_extent_check(start, size)
	vm_offset_t start, size;
{
	if(extent_alloc_region(&slab_extent, start, size, EX_NOWAIT)) {
		printf("WARNING: CAN'T ALLOCATE MEMORY FROM SLAB EXTENT MAP!\n");
	}
}

void
slab_cache_insert(cache, addr, size, align, flags)
	register struct vm_slab_cache 	*cache;
	vm_offset_t 					addr;
	vm_size_t						size, align;
	int 							flags;
{
	struct vm_slab	*slabs;

	long indx = BUCKETINDX(size);
	slabs = &vm_slab_hashtable[indx];

	cache->vsc_addr = addr;
	cache->vsc_size = size;
	cache->vsc_stride = align * ((size - 1) / align + 1);
	cache->vsc_slabmax = (SLOTS(size) - sizeof(struct vm_slab)) / cache->vsc_stride;
	cache->vsc_flags = flags;

	CIRCLEQ_INSERT_HEAD(&slabs->vs_header, cache, vsc_list);
	slabs->vs_count++;
}






/*
struct vextops vextops;

int
vm_extent_create(vext, name, start, end, mtype, storage, storagesize, flags)
	struct vm_extent *vext;
	char *name;
	vm_offset_t start, end;
	int mtype, flags;
	caddr_t	storage;
	vm_size_t storagesize;
{
    struct vextops_create_args vap;
    int error;

    vap.a_head.a_ops = &vextops;
    vap.a_vext = vext;
    vap.a_name = name;
    vap.a_start = start;
    vap.a_end = end;
    vap.a_mtype = mtype;
    vap.a_storage = storage;
    vap.a_storagesize = storagesize;
    vap.a_flags = flags;

    if (vextops.vm_extent_create == NULL) {
    	return (EOPNOTSUPP);
    }

    error = vextops.vm_extent_create(vext, vext->vext_ext, name, start, end, mtype, storage, storagesize, flags);

    return (error);
}

int
vm_extent_alloc(vext, start, size, flags)
	struct vm_extent *vext;
	vm_offset_t start;
	vm_size_t	size;
	int flags;
{
	struct vextops_alloc_args vap;
	int error;

	vap.a_head.a_ops = &vextops;
	vap.a_vext = vext;
	vap.a_start = start;
	vap.a_addr += vap.a_start;
	vap.a_size = size;
	vap.a_flags = flags;

    if (vextops.vm_extent_alloc == NULL) {
    	return (EOPNOTSUPP);
    }

	error = vextops.vm_extent_alloc(vext, start, size, flags);

	return (error);
}

int
vm_extent_suballoc(vext, start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result)
	struct vm_extent *vext;
	vm_offset_t start, end;
	vm_size_t	size;
	int flags, malloctypes, mallocflags;
	u_long *result, alignment, boundary;
{
	struct vextops_suballoc_args vap;
	int error;

	vap.a_head.a_ops = &vextops;
	vap.a_vext = vext;
	vap.a_start = start;
	vap.a_end += vap.a_start;
	vap.a_size = size;
	vap.a_malloctypes = malloctypes;
	vap.a_mallocflags = mallocflags;
	vap.a_alignment = alignment;
	vap.a_boundary = boundary;
	vap.a_flags = flags;
	vap.a_result = result;

    if (vextops.vm_extent_suballoc == NULL) {
    	return (EOPNOTSUPP);
    }

	error = vextops.vm_extent_suballoc(vext, start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result);

	return (error);
}

int
vm_extent_free(vext, start, size, malloctypes, flags)
	struct vm_extent *vext;
	vm_offset_t start;
	vm_size_t	size;
	int malloctypes, flags;
{
	struct vextops_free_args vap;
	int error;

	vap.a_head.a_ops = &vextops;
	vap.a_vext = vext;
	vap.a_start = start;
	vap.a_malloctypes = malloctypes;
	vap.a_flags = flags;

    if (vextops.vm_extent_free == NULL) {
    	return (EOPNOTSUPP);
    }

	error = vextops.vm_extent_free(vext, start, size, malloctypes, flags);

	return (error);
}

int
vm_extent_destroy(vext)
	struct vm_extent *vext;
{
	struct vextops_destroy_args vap;
	int error;

	vap.a_head.a_ops = &vextops;
	vap.a_vext = vext;

    if (vextops.vm_extent_destroy == NULL) {
    	return (EOPNOTSUPP);
    }

	error = vextops.vm_extent_destroy(vext);

	return (error);
}
*/

/* initilize vextops */
/*
void
vextops_init()
{
	vextops_malloc(&vextops);
}
*/
/* allocate vextops */
/*
void
vextops_malloc(vextops)
	struct vextops *vextops;
{
	MALLOC(vextops, struct vextops *, sizeof(struct vextops *), M_VEXTOPS, M_WAITOK);
}

struct vextops vextops = {
		.vm_extent_create 	= vextops_create,
		.vm_extent_alloc 	= vextops_alloc,
		.vm_extent_suballoc = vextops_suballoc,
		.vm_extent_free 	= vextops_free,
		.vm_extent_destroy 	= vextops_destroy
};

int
vextops_create(vext, name, start, end, mallocok, mtype, storage, storagesize, flags)
	struct vm_extent *vext;
	char *name;
	vm_offset_t start, end;
	int mallocok, mtype, flags;
	caddr_t storage;
	vm_size_t storagesize;
{
	int error;

	if(mallocok == 1) {
		mallocok = vm_extent_mallocok(vext, 1);
		error = vm_extent_create(vext, name, start, end, mtype, storage, storagesize, EX_MALLOCOK | flags);
	} else {
		mallocok = vm_extent_mallocok(vext, 0);
		error = vm_extent_create(vext, name, start, end, mtype, storage, storagesize, flags);
	}
	return (error);
}

int
vextops_alloc(start, size, flags)
	vm_offset_t start;
	vm_size_t size;
	int flags;
{
	struct vm_extent *vext;
	int error;
	error = vm_extent_alloc(vext, start, size, flags);

	return (error);
}

int
vextops_suballoc(start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result)
	vm_offset_t start, end;
	vm_size_t size;
	u_long alignment, boundary, *result;
	int malloctypes, mallocflags, flags;
{
	struct vm_extent *vext;
	int error;

	error = vm_extent_suballoc(vext, start, end, size, malloctypes, mallocflags, alignment, boundary, flags, result);

	return (error);
}

int
vextops_free(vext, start, size, malloctypes, flags)
	struct vm_extent *vext;
	vm_offset_t start;
	vm_size_t size;
	int malloctypes, flags;
{
	if(malloctypes & flags) {
		return (vm_extent_free(vext, start, size, malloctypes, flags));
	}
	return (0);
}

int
vextops_destroy(vext)
	struct vm_extent *vext;
{
	return (vm_extent_destroy(vext));
}
*/
