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
 * vm extents provides a simple extent interface, for initializing and allocating
 * extent maps.
 * As extents can be mostly malloc independent, it provides an easy initialization before
 * the vm is up and running.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/null.h>
#include <sys/tree.h>

#include <devel/vm/include/vm.h>
#include <devel/vm_extent.h>

/* example vm_map startup using extents */
vm_extent_t		kmap_extent, kentry_extent, vmspace_extent;
vm_map_t 		kmapex;
vm_map_entry_t 	kentryex;

long 			vmspace_storage[EXTENT_FIXED_STORAGE_SIZE(sizeof(struct vmspace *))];
long 			kmap_storage[EXTENT_FIXED_STORAGE_SIZE(sizeof(vm_map_t))];
long			kentry_storage[EXTENT_FIXED_STORAGE_SIZE(sizeof(vm_map_entry_t))];

void
vm_map_startup1()
{
	vm_exbootinit(kmap_extent, "KMAP", &kmapex[0], &kmapex[MAX_KMAP], M_VMMAP, kmap_storage, sizeof(kmap_storage), EX_FAST);
	vm_exbootinit(kentry_extent, "KENTRY", &kentryex[0], &kentryex[MAX_KMAPENT], M_VMMAPENT, kentry_storage, sizeof(kentry_storage), EX_FAST);

	vm_exbootinit(vmspace_extent, "VMSPACE", min, max, M_VMMAP, vmspace_storage, sizeof(vmspace_storage), EX_FAST);

	struct vmspace *vm = (struct vmspace *)vm_exalloc(vmspace_extent);
}

LIST_HEAD(exlist, vm_extent) exlist = LIST_HEAD_INITIALIZER(exlist);

vm_extent_t
vm_exinit(name, start, end, mtype, storage, storagesize, flags)
	char 	*name;
	u_long 	start, end;
	int 	mtype;
	caddr_t storage;
	size_t 	storagesize;
	int 	flags;
{
	register vm_extent_t ex;

	ex = (vm_extent_t)malloc(sizeof(vm_extent_t), M_VMEXTENT, M_WAITOK);

	if(ex == NULL) {
		return (NULL);
	}

	ex->ve_extent = extent_create(name, start, end, mtype, storage, storagesize, flags);
	if(vm_exalloc_region(ex->ve_extent, start, (end - start), flags) != 0) {
		vm_exfree(ex, start, ex->ve_size1, flags);
		return (NULL);
	}
	vm_extent_lock_init(ex);
	return (ex);
}

/* create an extent and allocate to the extent map */
void
vm_exbootinit(ex, name, start, end, mtype, storage, storagesize, flags)
	vm_extent_t	ex;
	char 	*name;
	u_long 	start, end;
	int 	mtype;
	caddr_t storage;
	size_t 	storagesize;
	int 	flags;
{
	ex = vm_exinit(name, start, end, mtype, storage, storagesize, flags);
}

void *
vm_exalloc(ex)
	vm_extent_t	ex;
{
	void *item;

	vm_extent_lock(ex);
	item = (void *)vm_exget(ex->ve_extent->ex_name, ex->ve_extent->ex_start, ex->ve_extent->ex_end);
	vm_extent_unlock(ex);

	return (item);
}

vm_extent_t
vm_exget(name, start, end)
	char 		*name;
	u_long 		start, end;
{
	register vm_extent_t ex;
	register struct extent *ext;

	vm_extent_lock(ex);
	for(ex = LIST_FIRST(&exlist); ex != NULL; ex = LIST_NEXT(ex, ve_exnode)) {
		ext = ex->ve_extent;
		if((ext->ex_name == name) && (ext->ex_start == start) && (ext->ex_end == end)) {
			vm_extent_unlock(ex);
			return (ex);
		}
	}
	vm_extent_unlock(ex);
	return (NULL);
}

int
vm_exalloc_region(ex, start, size, flags)
	vm_extent_t ex;
	u_long 		start, size;
	int 		flags;
{
	register struct extent *ext;

	if (ex == NULL) {
		return (1);
	}

	ext = ex->ve_extent;
	vm_extent_lock(ex);
	if (extent_alloc_region(ext, start, size, flags)) {
		ex->ve_size1 = size;
		LIST_INSERT_HEAD(&exlist, ex, ve_exnode);
		vm_extent_unlock(ex);
		return (0);
	}
	vm_exfree(ex, start, size, flags);
	vm_extent_unlock(ex);
	return (1);
}

int
vm_exalloc_subregion(ex, size, alignment, boundary, flags, result)
	vm_extent_t ex;
	u_long 		size, alignment, boundary;
	int 		flags;
	u_long 		*result;
{
	register struct extent *ext;

	if (ex == NULL) {
		return (1);
	}

	ext = ex->ve_extent;
	vm_extent_lock(ex);
	if(extent_alloc(ex, size, alignment, boundary, flags, result)) {
		ex->ve_size2 = size;
		ex->ve_result = result;
		LIST_INSERT_HEAD(&exlist, ex, ve_exnode);
		vm_extent_unlock(ex);
		return (0);
	}
	vm_exfree(ex, ext->ex_start, size, flags);
	vm_extent_unlock(ex);
	return (1);
}

void
vm_exfree(ex, start, size, flags)
	vm_extent_t ex;
	u_long 		start, size;
	int 		flags;
{
	register struct extent *ext;

	ext = ex->ve_extent;
	vm_extent_lock(ex);
	if (ext) {
		error = extent_free(ext, start, size, flags);
		if (error != 0) {
			panic("vm_exfree: extent not freed");
		}
		LIST_REMOVE(ex, ve_exnode);
		vm_extent_unlock(ex);
	}
}

void
vm_exdestroy(ex)
	vm_extent_t ex;
{
	register struct extent *ext;

	ext = ex->ve_extent;
	vm_extent_lock(ex);
	if (ext) {
		extent_destroy(ext);
		LIST_REMOVE(ex, ve_exnode);
		vm_extent_unlock(ex);
	}
}
