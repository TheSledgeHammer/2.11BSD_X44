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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/null.h>
#include <sys/tree.h>

#include <devel/vm/include/vm.h>

static struct extent 	*kmap_extent, *kentry_extent;
vm_map_t 				kmap;
vm_map_entry_t 			kentry;
vm_size_t 				kmap_size, kentry_size;

long kmap_storage[EXTENT_FIXED_STORAGE_SIZE(sizeof(vm_map_t))];
long kentry_storage[EXTENT_FIXED_STORAGE_SIZE(sizeof(vm_map_entry_t))];

vm_offset_t
vm_page_bootstrap(size, nitems)
	vm_size_t size;
	int nitems;
{
	vm_offset_t data;
	vm_size_t  	data_size;

	data_size = (nitems * size);
	data = (vm_offset_t)pmap_bootstrap_alloc(data_size);
	return (data);
}

void
vm_page_startup()
{
	kmap_size = vm_page_bootstrap(sizeof(struct vm_map), MAX_KMAP);
	kentry_size = vm_page_bootstrap(sizeof(struct vm_map_entry), MAX_KMAPENT);
}

void
vm_map_startup()
{
	kmap_extent = extent_init("KMAP", kmap_size, M_VMMAP, kmap_storage, sizeof(kmap_storage), EX_NOWAIT | EX_MALLOCOK);
	kentry_extent = extent_init("KENTRY", kentry_size, M_VMMAPENT, kentry_storage, sizeof(kentry_storage), EX_NOWAIT | EX_MALLOCOK);
}

/*
 * create and allocate an extent map
 */
struct extent *
extent_init(name, size, mtype, storage, storagesize, flags)
	char    *name;
	u_long  size;
	int     mtype;
	caddr_t storage;
	size_t 	storagesize;
	int     flags;
{
	struct extent *ex;
    u_long start, end;
    int error;

    start = trunc_page(size);
    end = round_page(size);

    ex = extent_create(name, start, end, mtype, storage, storagesize, flags);
    if (ex == NULL) {
        return (NULL);
    }
    error = extent_alloc_region(ex, start, size, flags);
    if (error != 0) {
    	error = extent_free(ex, start, size, flags);
    	if (error != 0) {
    		extent_destroy(ex);
    		return (NULL);
    	}
    	return (NULL);
    }
	return (ex);
}
