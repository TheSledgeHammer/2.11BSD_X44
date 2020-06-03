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
#include "include/vm_extent2.h"

void
vm_extent_init(vm_ext, name, start, end, mtype, storage, storagesize)
	struct vm_extent *vm_ext;
	char *name;
    vm_offset_t start, end;
    int mtype;
    caddr_t	storage;
    vm_size_t storagesize;
{
    vm_ext->name = name;
    vm_ext = extent_create(name, start, end, mtype, storage, storagesize, EX_NOWAIT | EX_MALLOCOK);
}

void
vm_extent_mallocok(void)
{
    vm_extent_malloc_safe = 1;
}

int
vm_extent_alloc(vm_ext, size)
    struct vm_extent *vm_ext;
    vm_size_t size;
{
    struct extent *ex = vm_ext->vm_extent;
    int error;

    if (ex == NULL) {
        return (0);
    }
    vm_ext->addr += ex->ex_start;

    error = extent_alloc_region(ex, vm_ext->addr, size, EX_NOWAIT | EX_MALLOCOK);

    if(error) {
        return (error);
    }

    return (0);
}

int
vm_extent_suballoc(vm_ext, start, end, size, malloctypes, mallocflags, alignment, boundary, result)
	struct vm_extent *vm_ext;
    vm_offset_t start, end;
    vm_size_t size;
    u_long *result;
	int malloctypes, mallocflags;
	u_long *result, alignment, boundary;
{
    struct extent *ex = vm_ext->vm_extent;
    int error;

    if (start > ex->ex_start && start < ex->ex_end) {
        if (end > start) {
            error = extent_alloc_subregion(ex, start, end, size, alignment, boundary, EX_FAST | EX_NOWAIT | EX_MALLOCOK, result);
        }
    }

    if(error) {
        return (error);
    }
    vm_ext->malloctypes = malloctypes;
    vm_ext->mallocflags = mallocflags;
    return (0);
}

void
vm_extent_destroy(vm_ext)
    struct vm_extent *vm_ext;
{
    struct extent *ex = vm_ext->vm_extent;

    if(ex != NULL) {
        extent_destroy(ex);
    }

    if(VMEXTENT_ALLOCATED) {
        free(vm_ext, M_VMEXTENT);
    }
}

static void
vm_extent_unmap(vm_ext, start, size, type)
    struct vm_extent *vm_ext;
    vm_offset_t start;
    vm_size_t size;
    int type;
{
    struct extent *ex = vm_ext->vm_extent;
    int error;

    if(ex == NULL) {
        return;
    }

    if(vm_ext->malloctypes != type) {
    	 error = extent_free(ex, start, size, EX_NOWAIT);
    }

    if (error) {
        printf("%#lx-%#lx of %s space lost\n", start, start + size, ex->ex_name);
    }
}

void
vm_extent_free(vm_ext, size, type)
    struct vm_extent *vm_ext;
	vm_size_t size;
	int type;
{
    struct extent *ex = vm_ext->vm_extent;

    if (ex != NULL) {
        vm_extent_unmap(vm_ext, ex->ex_start, size, type);
    }
}
