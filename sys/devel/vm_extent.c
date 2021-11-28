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
vm_extent_t		kmap_extent, kentry_extent;
vm_map_t 		kmapex;
vm_map_entry_t 	kentryex;
long 			kmap_storage[EXTENT_FIXED_STORAGE_SIZE(sizeof(vm_map_t))];
long			kentry_storage[EXTENT_FIXED_STORAGE_SIZE(sizeof(vm_map_entry_t))];

void
vm_map_startup1()
{
	vm_exbootinit(kmap_extent, "KMAP", &kmapex[0], &kmapex[MAX_KMAP], M_VMMAP, kmap_storage, sizeof(kmap_storage), EX_FAST);
	vm_exbootinit(kentry_extent, "KENTRY", &kentryex[0], &kentryex[MAX_KMAPENT], M_VMMAPENT, kentry_storage, sizeof(kentry_storage), EX_FAST);
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
	vm_exalloc_region(ex, start, (end - start), flags);
}

/* create an extent without allocating to the extent map */
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

	ex = extent_create(name, start, end, mtype, storage, storagesize, flags);
	if (ex == NULL) {
		return (NULL);
	}

	return (ex);
}

void
vm_exalloc_region(ex, start, size, flags)
	vm_extent_t	ex;
	u_long 		start, size;
	int 		flags;
{
	int error = extent_alloc_region(ex, start, size, flags);
	if (error != 0) {
		vm_exdestroy(ex);
		panic("vm_extent_alloc_region: failed to allocate extent region");
	}
}

void
vm_exalloc_subregion(ex, size, alignment, boundary, flags, result)
	vm_extent_t ex;
	u_long 		size, alignment, boundary;
	int 		flags;
	u_long 		*result;
{
	int error = extent_alloc(ex, size, alignment, boundary, flags, result);
	if (error != 0) {
		vm_exfree(ex, ex->ex_start, size, flags);
		panic("vm_extent_alloc_subregion: failed to allocate extent subregion");
	}
}

void
vm_exfree(ex, start, size, flags)
	vm_extent_t ex;
	u_long 		start, size;
	int 		flags;
{
	int error;
	if (ex) {
		error = extent_free(ex, start, size, flags);
		if (error != 0) {
			panic("vm_extent_free: extent not freed");
		}
	}
}

void
vm_exdestroy(ex)
	vm_extent_t ex;
{
	extent_destroy(ex);
}
