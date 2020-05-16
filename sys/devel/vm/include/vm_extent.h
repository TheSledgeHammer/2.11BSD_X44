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

/* Virtual Memory Extent Allocator Management */

#ifndef _VM_EXTENT_H
#define _VM_EXTENT_H

#include <machine/param.h>

int vm_extent_malloc_safe;

struct vm_extent {
    struct extent          	*vm_extent;
    struct vm_extops		*extops;
    char					*name;
    vm_offset_t             addr;
    int                     malloctypes;
    int                     mallocflags;
};

#define VMEXTENT_ALLOCATED  (1 < 0)

#define VMEXTENT_ALIGNMENT	(NBPG/NBPDE)
#define VMEXTENT_BOUNDARY	0x0					/* Temporary:  */

void vm_extent_init(struct vm_extent *, char*, vm_offset_t, vm_offset_t, caddr_t, vm_size_t); 	/* Allocates an extent with storage */
void vm_extent_mallocok(void);
int vm_extent_alloc(struct vm_extent *, vm_size_t);
int vm_extent_suballoc(struct vm_extent *, vm_offset_t, vm_offset_t, vm_size_t, int, int, u_long *);
void vm_extent_free(struct vm_extent *, vm_size_t, int);

#endif //_VM_EXTENT_H
